#ifndef PBRT_UTIL_LSYSTEM_H
#define PBRT_UTIL_LSYSTEM_H

#include <pbrt/pbrt.h>
#include <pbrt/shapes.h>

#include <vector>
#include <bitset>

namespace pbrt {

struct Tube {
  public:
    Tube(Point3f start, Point3f end) : start(start), end(end) {};
    Point3f start, end;
};

struct Trie {
  struct Node {
    Node *nxt[128];
    bool terminal = false;
    std::string value;
  };

  Node *root = new Node;
  void Insert(const std::string &key, const std::string &value);
  std::string Match(std::string::iterator &it, std::string::iterator end);
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
    
    std::string axiom;
    Trie rules;
    std::string sequence;

    const char ruleDelimiter = '=';
    std::vector<Tube> tubes;

    // Lsystem Private Methods
    std::string GenerateSequence();
};


}   // namespace pbrt

#endif  // PBRT_UTIL_LSYSTEM_H