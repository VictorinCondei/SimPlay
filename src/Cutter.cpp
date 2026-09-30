#include "Cutter.h"

#include <algorithm>
#include <cmath>

namespace
{

inline float clampPositive(float v)
{
    return (v > 0.0f) ? v : 0.0f;
}

inline float length2(float x, float y)
{
    return std::sqrt(x * x + y * y);
}

inline float length3(float x, float y, float z)
{
    return std::sqrt(x * x + y * y + z * z);
}

float sdfCappedCylinder(float px, float py, float pz,
                        float cx, float cy,
                        float zBottom, float zTop,
                        float r)
{
    const float dx = px - cx;
    const float dy = py - cy;
    const float radial = length2(dx, dy) - r;

    const float below = zBottom - pz;
    const float above = pz - zTop;
    const float axial = std::max(below, above);

    if (radial <= 0.0f && axial <= 0.0f)
        return std::max(radial, axial);

    const float ox = clampPositive(radial);
    const float oz = clampPositive(axial);
    return std::sqrt(ox * ox + oz * oz);
}

} // namespace

Cutter::Cutter(CutterType t, float r, float len)
    : type(t), radius(r), length(len), x(0.0f), y(0.0f), z(0.0f)
{
}

float Cutter::signedDistance(float px, float py, float pz) const
{
    if (type == CutterType::FLAT_END) {
        return sdfCappedCylinder(px, py, pz, x, y, z - length, z, radius);
    }

    const float sphereCenterZ = z - radius;

    const float dx = px - x;
    const float dy = py - y;
    const float dz = pz - sphereCenterZ;
    const float sphereSdf = length3(dx, dy, dz) - radius;

    const float cylinderSdf =
        sdfCappedCylinder(px, py, pz, x, y, sphereCenterZ, z, radius);

    // Lower hemisphere only.
    float tipSdf;
    {
        const float clip = pz - sphereCenterZ;
        if (sphereSdf <= 0.0f && clip <= 0.0f)
            tipSdf = std::max(sphereSdf, clip);
        else {
            const float ox = clampPositive(sphereSdf);
            const float oz = clampPositive(clip);
            tipSdf = std::sqrt(ox * ox + oz * oz);
        }
    }

    return std::min(tipSdf, cylinderSdf);
}

void Cutter::bounds(float& minX, float& minY, float& minZ,
                    float& maxX, float& maxY, float& maxZ) const
{
    minX = x - radius;
    maxX = x + radius;
    minY = y - radius;
    maxY = y + radius;
    minZ = z - length;
    maxZ = z;
}

void Cutter::carveAt(VoxelGrid& grid, float px, float py, float pz) const
{
    Cutter pose = *this;
    pose.x = px;
    pose.y = py;
    pose.z = pz;

    float minX, minY, minZ, maxX, maxY, maxZ;
    pose.bounds(minX, minY, minZ, maxX, maxY, maxZ);

    const float pad = grid.cell;
    minX -= pad; minY -= pad; minZ -= pad;
    maxX += pad; maxY += pad; maxZ += pad;

    int ixMin, iyMin, izMin;
    int ixMax, iyMax, izMax;

    grid.worldToVoxel(minX, minY, minZ, ixMin, iyMin, izMin);
    grid.worldToVoxel(maxX, maxY, maxZ, ixMax, iyMax, izMax);

    ixMin = std::max(0, ixMin);
    iyMin = std::max(0, iyMin);
    izMin = std::max(0, izMin);

    ixMax = std::min(grid.nx - 1, ixMax);
    iyMax = std::min(grid.ny - 1, iyMax);
    izMax = std::min(grid.nz - 1, izMax);

    for (int ix = ixMin; ix <= ixMax; ++ix) {
        for (int iy = iyMin; iy <= iyMax; ++iy) {
            for (int iz = izMin; iz <= izMax; ++iz) {
                float cx, cy, cz;
                grid.voxelCenter(ix, iy, iz, cx, cy, cz);

                const float toolSdf = pose.signedDistance(cx, cy, cz);
                //const float newValue = std::max(grid.value(ix, iy, iz), -toolSdf);
                const float newValue = std::max(grid.value(ix, iy, iz), toolSdf);

                grid.setValue(ix, iy, iz, newValue);
            }
        }
    }
}

void Cutter::carve(VoxelGrid& grid) const
{
    carveAt(grid, x, y, z);
}

void Cutter::carveSegment(VoxelGrid& grid,
                          float x0, float y0, float z0,
                          float x1, float y1, float z1) const
{
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float dz = z1 - z0;

    const float segmentLength = std::sqrt(dx * dx + dy * dy + dz * dz);

    // Sample densely enough that neighboring cutter stamps overlap.
    const float step = std::max(0.25f * radius, 0.5f * grid.cell);
    const int samples = std::max(1, static_cast<int>(std::ceil(segmentLength / step)));

    for (int i = 0; i <= samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(samples);
        const float px = x0 + dx * t;
        const float py = y0 + dy * t;
        const float pz = z0 + dz * t;
        carveAt(grid, px, py, pz);
    }
}
