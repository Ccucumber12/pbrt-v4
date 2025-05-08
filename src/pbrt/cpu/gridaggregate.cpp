#include <pbrt/cpu/gridaggregate.h>

#include <pbrt/shapes.h>
#include <pbrt/util/math.h>

namespace pbrt {


struct GridPrimitive {
  GridPrimitive() {}
  GridPrimitive(Primitive p) : prim(std::move(p)) {}

  Bounds3f Bounds() const { return prim.Bounds(); }

  pstd::optional<ShapeIntersection> Intersect(const Ray &ray, Float tMax) {
    Float checkHash = HashFloat(ray.o, ray.d);
    if (checkHash == prevCheckHash)
      return {};
    prevCheckHash = checkHash;
    return prim.Intersect(ray, tMax);
  }

  bool IntersectP(const Ray &ray, Float tMax) {
    Float checkHash = HashFloat(ray.o, ray.d);
    if (checkHash == prevCheckHash)
      return false;
    prevCheckHash = checkHash;
    return prim.IntersectP(ray, tMax);
  }

private:
  Primitive prim;
  Float prevCheckHash = 0;
};

struct Voxel {
  uint32_t size() const { return primitives.size(); }
  Voxel() {}
  Voxel(GridPrimitive *prim) {
    primitives.push_back(prim);
  }
  void AddPrimitive(GridPrimitive *prim) {
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
    for (const auto &prim : primitives)
      if (prim->IntersectP(ray, tMax))
        return true;
    return false;
  }

private:
  std::vector<GridPrimitive*> primitives;
};


GridAggregate::GridAggregate(std::vector<Primitive> p) {
  CHECK(!p.empty());
  primitives.resize(p.size());
  for (uint32_t i = 0; i < p.size(); ++i)
    primitives[i] = GridPrimitive(std::move(p[i]));

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
          voxels[o].AddPrimitive(&primitives[i]);
        }
  }
}


pstd::optional<ShapeIntersection> GridAggregate::Intersect(const Ray &ray, Float tMax) const {
  if (!bounds.IntersectP(ray.o, ray.d, tMax))
    return {};
    
  Float nextT[3], deltaT[3];
  int step[3], out[3], pos[3];
  for (int axis = 0; axis < 3; ++axis) {
    pos[axis] = posToVoxel(ray.o, axis);
    if (ray.d[axis] >= 0) {
      nextT[axis] = (voxelToPos(pos[axis] + 1, axis) - ray.o[axis]) / ray.d[axis];
      deltaT[axis] = width[axis] / ray.d[axis];
      step[axis] = 1;
      out[axis] = nVoxels[axis];
    } else {
      nextT[axis] = (voxelToPos(pos[axis], axis) - ray.o[axis]) / ray.d[axis];
      deltaT[axis] = -width[axis] / ray.d[axis];
      step[axis] = -1;
      out[axis] = -1;
    }
  }

  pstd::optional<ShapeIntersection> closestIsect;
  for (;;) {
    const Voxel& voxel = voxels[offset(pos[0], pos[1], pos[2])];
    auto isect = voxel.Intersect(ray, tMax);
    if (isect && (!closestIsect || isect->tHit < closestIsect->tHit))
      closestIsect = isect;
    
    int bits = ((nextT[0] < nextT[1]) << 2) +
               ((nextT[0] < nextT[2]) << 1) +
               ((nextT[1] < nextT[2]));
    const int cmpToAxis[8] = { 2, 1, 2, 1, 2, 2, 0, 0 };
    int stepAxis = cmpToAxis[bits];
    if ((closestIsect && closestIsect->tHit < nextT[stepAxis]) || 
        tMax < nextT[stepAxis])
      break;
    pos[stepAxis] += step[stepAxis];
    if (pos[stepAxis] == out[stepAxis])
      break;
    nextT[stepAxis] += deltaT[stepAxis];
  }
  return closestIsect;
}


bool GridAggregate::IntersectP(const Ray &ray, Float tMax) const {
 if (!bounds.IntersectP(ray.o, ray.d, tMax))
    return false;
    
  Float nextT[3], deltaT[3];
  int step[3], out[3], pos[3];
  for (int axis = 0; axis < 3; ++axis) {
    pos[axis] = posToVoxel(ray.o, axis);
    if (ray.d[axis] >= 0) {
      nextT[axis] = (voxelToPos(pos[axis] + 1, axis) - ray.o[axis]) / ray.d[axis];
      deltaT[axis] = width[axis] / ray.d[axis];
      step[axis] = 1;
      out[axis] = nVoxels[axis];
    } else {
      nextT[axis] = (voxelToPos(pos[axis], axis) - ray.o[axis]) / ray.d[axis];
      deltaT[axis] = -width[axis] / ray.d[axis];
      step[axis] = -1;
      out[axis] = -1;
    }
  }

  for (;;) {
    const Voxel& voxel = voxels[offset(pos[0], pos[1], pos[2])];
    if (voxel.IntersectP(ray, tMax))
      return true;
    
    int bits = ((nextT[0] < nextT[1]) << 2) +
               ((nextT[0] < nextT[2]) << 1) +
               ((nextT[1] < nextT[2]));
    const int cmpToAxis[8] = { 2, 1, 2, 1, 2, 2, 0, 0 };
    int stepAxis = cmpToAxis[bits];
    if (tMax < nextT[stepAxis])
      break;
    pos[stepAxis] += step[stepAxis];
    if (pos[stepAxis] == out[stepAxis])
      break;
    nextT[stepAxis] += deltaT[stepAxis];
  }
  return false;
}


GridAggregate *GridAggregate::Create(std::vector<Primitive> prims,
                                     const ParameterDictionary &parameters) {
  return new GridAggregate(std::move(prims));
}

} // namespace pbrt