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
  std::vector<int> primitives; // index in GridAggregate::primitives
};


GridAggregate::GridAggregate(std::vector<Primitive> p) : primitives(std::move(p)) {
  CHECK(!primitives.empty());

  for (const auto &prim : primitives)
    bounds = Union(bounds, prim.Bounds());

  Vector3f diag = bounds.pMax - bounds.pMin;
  int maxAxis = bounds.MaxDimension();
  Float invMaxWidth = 1 / diag[maxAxis];
  CHECK_GT(invMaxWidth, 0);
  Float cubeRoot = 3 * std::pow(Float(primitives.size()), 1 / 3.f);
  Float voxelsPerUnitDist = cubeRoot * invMaxWidth;
  for (int axis = 0; axis < 3; ++axis) {
    nVoxels[axis] = pstd::round(diag[axis] * voxelsPerUnitDist);
    nVoxels[axis] = Clamp(nVoxels[axis], 1, 64);
  }

  for (int axis = 0; axis < 3; ++axis) {
    width[axis] = diag[axis] / nVoxels[axis];
    invWidth[axis] = (width[axis] == 0.f) ? 0.f : 1.f / width[axis];
  }
  int nv = nVoxels[0] * nVoxels[1] * nVoxels[2];
  voxels.assign(nv, Voxel());

  for (uint32_t i = 0; i < primitives.size(); ++i) {
    Bounds3f pb = primitives[i].Bounds();
    int vmin[3], vmax[3];
    for (int axis = 0; axis < 3; ++axis) {
      vmin[axis] = posToVoxel(pb.pMin, axis);
      vmax[axis] = posToVoxel(pb.pMax, axis);
    }

    for (int z = vmin[2]; z <= vmax[2]; ++z)
      for (int y = vmin[1]; y <= vmax[1]; ++y)
        for (int x = vmin[0]; x <= vmax[0]; ++x) {
          int o = offset(x, y, z);
          voxels[o].AddPrimitive(primitives[i]);
        }
  }



  
}


GridAggregate *GridAggregate::Create(std::vector<Primitive> prims,
                                     const ParameterDictionary &parameters) {
  return new GridAggregate(std::move(prims));
}

} // namespace pbrt