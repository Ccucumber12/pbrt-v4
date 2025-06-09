#include <pbrt/util/lsystem.h>
#include <pbrt/paramdict.h>

#include <pbrt/util/string.h>
#include <pbrt/util/log.h>
#include <set>

namespace pbrt {

void Trie::Insert(const std::string &key, const std::string &value) {
  Node *cur = root;
  for (char c : key) {
    if (!cur -> nxt[c])
      cur -> nxt[c] = new Node();
    cur = cur -> nxt[c];
  }
  if (cur -> terminal)
    ErrorExit("Duplicate key \"%s\"", key);
  cur -> terminal = true;
  cur -> value = value;
}

std::string Trie::Match(std::string::iterator &it, std::string::iterator end) {
  Node *cur = root;
  std::string ret(1, *it);
  std::string::iterator matchedIt = it;
  while (it != end) {
    if (cur -> nxt[*it]) {
      cur = cur -> nxt[*it];
      if (cur -> terminal) {
        ret = cur -> value;
        matchedIt = it;
      }
      ++it;
    } else {
      break;
    }
  }
  it = ++matchedIt;
  return ret;
}

struct State {
  Float stepSize;
  Float radius;
  Point3f currentPos;
  Vector3f frontVec;
  Vector3f rightVec;

  State(Float stepSize, Float radius, const Point3f& currentPos,
        const Vector3f& frontVec, const Vector3f& rightVec)
      : stepSize(stepSize),
        radius(radius),
        currentPos(currentPos),
        frontVec(frontVec),
        rightVec(rightVec) {}
};

void Polygon::CreateShapes(Allocator alloc, const Transform *renderFromObject, 
                          const Transform *objectFromRender, pstd::vector<Shape> &shapes, int &shapeIdx) {
  if (!indicies.empty()) {
    TriangleMesh *mesh = alloc.new_object<TriangleMesh>(
        *renderFromObject, false, indicies, points,
        std::vector<Vector3f>(), std::vector<Normal3f>(),
        std::vector<Point2f>(), std::vector<int>(), alloc);
    auto triShapes = Triangle::CreateTriangles(mesh, alloc);
    for (const auto& triangle : triShapes)
      shapes[shapeIdx++] = triangle;
  }
}

void Polygon::AddPoint(Point3f p) {
  points.emplace_back(p);
  ++nPoints;
  if (nPoints >= 3)
    indicies.insert(indicies.end(), {0, nPoints-2, nPoints-1});
}

Lsystem::Lsystem( const Transform *renderFromObject, 
                  const Transform *objectFromRender,
                  const ParameterDictionary &parameters,
                  const FileLoc *loc, Allocator alloc) : 
                  renderFromObject(renderFromObject), objectFromRender(objectFromRender) {
  radius = parameters.GetOneFloat("radius", 0.05);
  stepSize = parameters.GetOneFloat("stepsize", 1.0);
  angle = parameters.GetOneFloat("angle", 28.0);
  radiusScale = parameters.GetOneFloat("radiusscale", 0.9);

  nGenerations = parameters.GetOneInt("n", 3);
  axiom = parameters.GetOneString("axiom", "");

  std::vector<std::string> raw_rules = parameters.GetStringArray("rules");
  for (std::string rule : raw_rules) {
    rule.erase(std::remove(rule.begin(), rule.end(), ' '), rule.end());
    auto parts = SplitString(rule, '=');
    if (parts.size() != 2)
      ErrorExit(loc, "Invalid rule %s.", rule);
    std::string key = parts[0];
    std::string value = parts[1];
    rules.Insert(key, value);
  }

  Point3f currentPos(0, 0, 0);
  Vector3f frontVec(0, 0, 1); // starting upwards
  Vector3f rightVec(1, 0, 0);

  sequence = GenerateSequence();
  pstd::vector<State> stack;
  Polygon *polygon = nullptr;
  for (char c : sequence) {
    if (c == '>') {
      Point3f newPos = currentPos + frontVec * stepSize;
      tubes.emplace_back(Tube(currentPos, newPos, radius));
      currentPos = newPos;
    } else if (c == '~') {
      currentPos = currentPos + frontVec * stepSize;
    } else if (c == '+') {
      frontVec = Rotate(angle, rightVec)(frontVec);
    } else if (c == '-') {
      frontVec = Rotate(-angle, rightVec)(frontVec);
    } else if (c == '&') {
      Vector3f upVec = Cross(frontVec, rightVec);
      frontVec = Rotate(angle, upVec)(frontVec);
      rightVec = Rotate(angle, upVec)(rightVec);
    } else if (c == '^') {
      Vector3f upVec = Cross(frontVec, rightVec);
      frontVec = Rotate(-angle, upVec)(frontVec);
      rightVec = Rotate(-angle, upVec)(rightVec);
    } else if (c == '`') {
      rightVec = Rotate(angle, frontVec)(rightVec);
    } else if (c == '/') {
      rightVec = Rotate(-angle, frontVec)(rightVec);
    } else if (c == '|') {
      frontVec = Rotate(180, rightVec)(frontVec);
    } else if (c == '(') {
      stack.emplace_back(stepSize, radius, currentPos, frontVec, rightVec);
    } else if (c == ')') {
      if (stack.empty())
        ErrorExit("Invalid sequence (pop when stack empty): %s", sequence);
      stepSize = stack.back().stepSize;
      radius = stack.back().radius;
      currentPos = stack.back().currentPos;
      frontVec = stack.back().frontVec;
      rightVec = stack.back().rightVec;
      stack.pop_back();
    } else if (c == '\'') {
      radius *= radiusScale;
    } else if (c == '{') {
      if (polygon) 
        ErrorExit("Invalid sequence (double polygon construction): %s", sequence);
      polygon = new Polygon();
    } else if (c == '}') {
      if (!polygon)
        ErrorExit("Invalid sequence (end polygon when not in construction): %s", sequence);
      polygons.emplace_back(*polygon);
      polygon = nullptr;
    } else if (c == '@') {
      if (!polygon)
        Error("Invalid sequence (add point when not in polygon).");
      else
        polygon -> AddPoint(currentPos);
    }
  }
  nTubes = tubes.size();
}

pstd::vector<Shape> Lsystem::CreateShapes(Allocator alloc) {
  Float zmin = 0;
  Float phimax = 360;

  int nShape = nTubes * 3; // 1 tube + 2 hemispheres
  for (Polygon &poly : polygons)
    nShape += poly.indicies.size() / 3;
  pstd::vector<Shape> shapes(nShape, alloc);

  Cylinder *cy = alloc.allocate_object<Cylinder>(nTubes);
  Sphere *sp = alloc.allocate_object<Sphere>(nTubes * 2);
  for (int i = 0; i < nTubes; ++i) {
    Point3f p0 = tubes[i].start;
    Point3f p1 = tubes[i].end;
    Float r = tubes[i].radius;
    
    Vector3f dir = p1 - p0;
    Float height = Length(dir);
    dir /= height;

    Transform objectFromStart = Translate(Vector3f(p0)) * RotateFromTo(Vector3f(0,0,1), dir);
    Transform *renderFromStart = alloc.new_object<Transform>(*renderFromObject * objectFromStart);
    Transform *startFromRender = alloc.new_object<Transform>(Inverse(objectFromStart) * *objectFromRender);

    Transform objectFromEnd = Translate(Vector3f(p1)) * RotateFromTo(Vector3f(0,0,1), dir);
    Transform *renderFromEnd = alloc.new_object<Transform>(*renderFromObject * objectFromEnd);
    Transform *endFromRender = alloc.new_object<Transform>(Inverse(objectFromEnd) * *objectFromRender);

    alloc.construct(&cy[i], renderFromStart, startFromRender, false,
                    r, zmin, height, phimax);
    shapes[3*i] = &cy[i];
    
    alloc.construct(&sp[2*i], renderFromStart, startFromRender, false, r, -r, 0, 360);
    alloc.construct(&sp[2*i+1], renderFromEnd, endFromRender, false, r, 0, r, 360);
    shapes[3*i+1] = &sp[2*i];
    shapes[3*i+2] = &sp[2*i+1];
  }

  int shapeIdx = nTubes * 3;
  for (Polygon poly : polygons)
    poly.CreateShapes(alloc, renderFromObject, objectFromRender, shapes, shapeIdx);
  return shapes;
}

std::string Lsystem::GenerateSequence() {
  std::string seq = axiom;
  for (int gen=0; gen<nGenerations; ++gen) {
    std::string nextSeq = "";
    auto it = seq.begin();
    while (it != seq.end())
      nextSeq += rules.Match(it, seq.end());
    seq = std::move(nextSeq);
    // LOG_VERBOSE("seq: %s", seq);
  }
  return seq;
}

}   // namespace pbrt