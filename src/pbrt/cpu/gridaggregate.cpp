#include <pbrt/cpu/gridaggregate.h>

#include <pbrt/shapes.h>
#include <pbrt/util/math.h>

namespace pbrt {

struct Voxel {
  uint32_t size() const { return primitives.size(); }
  Voxel() {}
  Voxel(std::shared_ptr<Primitive> prim) {
    primitives.push_back(prim);
  }
  void AddPrimitive(std::shared_ptr<Primitive> prim) {
    primitives.push_back(prim);
  }
  
  pstd::optional<ShapeIntersection> Intersect(const Ray &ray, Float tMax) const {
    pstd::optional<ShapeIntersection> closestIsect;
    for (const auto &prim : primitives) {
      auto isect = prim->Intersect(ray, tMax);
      if (isect && (!closestIsect || isect->tHit < closestIsect->tHit))
        closestIsect = isect;
    }
    return closestIsect;
  }

  bool IntersectP(const Ray &ray, Float tMax) const {
    for (const auto &prim : primitives) {
      if (prim->IntersectP(ray))
        return true;
    }
    return false;
  }

private:
  std::vector<std::shared_ptr<Primitive>> primitives;
};


GridAggregate::GridAggregate(std::vector<Primitive> p) : primitives(std::move(p)) {
  CHECK(!primitives.empty());

  for (const auto &prim : primitives) {
    bounds = Union(bounds, prim.Bounds());

  Vector3f diag = bounds.pMax - bounds.pMin;
  int maxAxis = bounds.MaxDimension();
  Float invMaxWidth = 1 / diag[maxAxis];
  CHECK(invMaxWidth > 0);
  // TODO: Continue Here
}


GridAggregate *GridAggregate::Create(std::vector<Primitive> prims,
                                     const ParameterDictionary &parameters) {
  return new GridAggregate(std::move(prims));
}

} // namespace pbrt