#ifndef PBRT_CPU_GRIDAGGREGATE_H
#define PBRT_CPU_GRIDAGGREGATE_H

#include <pbrt/pbrt.h>

#include <pbrt/cpu/primitive.h>

#include <vector>

namespace pbrt {

inline int Float2Int(Float f) { return static_cast<int>(f); }

struct Voxel;

class GridAggregate {
  public:
    GridAggregate(std::vector<Primitive> p);
    static GridAggregate *Create(std::vector<Primitive> prims,
                                 const ParameterDictionary &parameters);

    Bounds3f Bounds() const;
    pstd::optional<ShapeIntersection> Intersect(const Ray &ray, Float tMax) const;
    bool IntersectP(const Ray &ray, Float tMax) const;

  private:
    int posToVoxel(const Point3f &P, int axis) const {
        int v = Float2Int((P[axis] - bounds.pMin[axis]) * invWidth[axis]);
        return Clamp(v, 0, nVoxels[axis] - 1);
    }
    float voxelToPos(int p, int axis) const {
        return bounds.pMin[axis] + p * width[axis];
    }
    inline int offset(int x, int y, int z) const {
        return z * nVoxels[0] * nVoxels[1] + y * nVoxels[0] + x;
    }

    std::vector<Primitive> primitives;
    std::vector<Voxel> voxels;
    int nVoxels[3];
    Bounds3f bounds;
    float width[3], invWidth[3];
};


} // namespace pbrt

#endif // PBRT_CPU_GRIDAGGREGATE_H