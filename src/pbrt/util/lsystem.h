#ifndef PBRT_UTIL_LSYSTEM_H
#define PBRT_UTIL_LSYSTEM_H

#include <pbrt/pbrt.h>
#include <pbrt/shapes.h>

#include <vector>
#include <bitset>

namespace pbrt {

struct Tube {
  public:
    Tube(Point3f start, Point3f end, Float radius) 
      : start(start), end(end), radius(radius){};
    Point3f start, end;
    Float radius;
};

struct Trie {
  struct Node {
    Node *nxt[128] = {};
    bool terminal = false;
    std::string value;
  };

  Node *root = new Node;
  void Insert(const std::string &key, const std::string &value);
  std::string Match(std::string::iterator &it, std::string::iterator end);
};

struct Polygon {
  std::vector<Point3f> points;
  std::vector<int> indicies;
  int nPoints = 0;

  void CreateShapes(Allocator alloc, const Transform *renderFromObject, 
                      const Transform *objectFromRender, pstd::vector<Shape> &shapes, int &shapeIdx);
  void AddPoint(Point3f p);
};

class Lsystem {
  public:
    // Lsystem Public Methods
    Lsystem(const Transform *renderFromObject, const Transform *objectFromRender,
            const ParameterDictionary &parameters,
            const FileLoc *loc, Allocator alloc);

    pstd::vector<Shape> CreateShapes(Allocator alloc);

  private:
    // Lsystem Private Members
    const Transform *renderFromObject, *objectFromRender;
    bool reverseOrientation;
    Float radius;
    Float stepSize;
    Float angle;
    int nGenerations;
    int nTubes;

    Float radiusScale;
    Float stepSizeScale;
    
    std::string axiom;
    Trie rules;
    std::string sequence;

    const char ruleDelimiter = '=';
    pstd::vector<Tube> tubes;
    pstd::vector<Polygon> polygons;

    // Lsystem Private Methods
    std::string GenerateSequence();
};


}   // namespace pbrt

#endif  // PBRT_UTIL_LSYSTEM_H