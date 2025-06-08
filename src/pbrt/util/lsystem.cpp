#include <pbrt/util/lsystem.h>
#include <pbrt/paramdict.h>

#include <pbrt/util/string.h>
#include <pbrt/util/log.h>

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

Lsystem::Lsystem( const Transform *renderFromObject, 
                  const Transform *objectFromRender,
                  const ParameterDictionary &parameters,
                  const FileLoc *loc, Allocator alloc) : 
                  renderFromObject(renderFromObject), objectFromRender(objectFromRender) {
  radius = parameters.GetOneFloat("radius", 0.05);
  stepSize = parameters.GetOneFloat("stepsize", 1.0);
  angle = parameters.GetOneFloat("angle", 28.0);

  nGenerations = parameters.GetOneInt("n", 3);
  axiom = parameters.GetOneString("axiom", "");

  std::vector<std::string> raw_rules = parameters.GetStringArray("rules");
  for (std::string rule : raw_rules) {
    auto parts = SplitString(rule, '=');
    if (parts.size() != 2)
      ErrorExit(loc, "Invalid rule %s.", rule);
    std::string key = parts[0];
    std::string value = parts[1];
    if (!IsAllAlpha(key))
      ErrorExit(loc, "Key in rule contain non-alphabet: %s.", rule);
    rules.Insert(key, value);
  }

  Point3f currentPos(0, 0, 0);
  Vector3f direction(0, 0, 1); // up

  sequence = GenerateSequence();
  pstd::vector<Point3f> posStack;
  pstd::vector<Vector3f> dirStack;
  for (char c : sequence) {
    if (c == 'F') {
      Point3f newPos = currentPos + direction * stepSize;
      tubes.emplace_back(Tube(currentPos, newPos));
      currentPos = newPos;
    } else if (c == '+') {
      direction = RotateX(angle)(direction);
    } else if (c == '-') {
      direction = RotateX(-angle)(direction);
    } else if (c == '&') {
      direction = RotateY(angle)(direction);
    } else if (c == '^') {
      direction = RotateY(-angle)(direction);
    } else if (c == '\\') {
      direction = RotateZ(angle)(direction);
    } else if (c == '/') {
      direction = RotateZ(-angle)(direction);
    } else if (c == '[') {
      posStack.emplace_back(currentPos);
      dirStack.emplace_back(direction);
    } else if (c == ']') {
      if (posStack.empty())
        ErrorExit("Invalid Sequence: %s", sequence);
      currentPos = posStack.back();
      direction = dirStack.back();
      posStack.pop_back();
      dirStack.pop_back();
    }
  }
  nTubes = tubes.size();
}

pstd::vector<Shape> Lsystem::CreateShapes(Allocator alloc) {
  Float zmin = 0;
  Float phimax = 360;

  pstd::vector<Shape> cylinders(nTubes, alloc);
  Cylinder *cy = alloc.allocate_object<Cylinder>(nTubes);
  for (int i = 0; i < nTubes; ++i) {
    Point3f p0 = tubes[i].start;
    Point3f p1 = tubes[i].end;
    
    Vector3f dir = p1 - p0;
    Float height = Length(dir);
    dir /= height;

    Transform objectFromTube = Translate(Vector3f(p0)) * RotateFromTo(Vector3f(0,0,1), dir);
    Transform *renderFromTube = alloc.new_object<Transform>(*renderFromObject * objectFromTube);
    Transform *tubeFromRender = alloc.new_object<Transform>(Inverse(objectFromTube) * *objectFromRender);

    // Then use these transforms in construct:
    alloc.construct(&cy[i], renderFromTube, tubeFromRender, false,
                    radius, zmin, height, phimax);
    cylinders[i] = &cy[i];
    // LOG_ERROR("p0: %f, %f, %f", p0.x, p0.y, p0.z);
    // LOG_ERROR("p1: %f, %f, %f", p1.x, p1.y, p1.z);
  }
  return cylinders;
}

std::string Lsystem::GenerateSequence() {
  std::string seq = axiom;
  for (int gen=0; gen<nGenerations; ++gen) {
    std::string nextSeq = "";
    auto it = seq.begin();
    while (it != seq.end())
      nextSeq += rules.Match(it, seq.end());
    seq = std::move(nextSeq);
    // LOG_ERROR("Sequence: %s", seq);
  }
  return seq;
}

}   // namespace pbrt