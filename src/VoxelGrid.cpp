#include "VoxelGrid.h"
#include <cmath>
#include <algorithm>
#include <limits>

VoxelGrid::VoxelGrid() : nx(0), ny(0), nz(0), cell(1.0f)
{
    origin[0] = origin[1] = origin[2] = 0.0f;
}

void VoxelGrid::init(int _nx, int _ny, int _nz, float _cell,
                     float ox, float oy, float oz)
{
    nx = _nx;
    ny = _ny;
    nz = _nz;
    cell = _cell;
    origin[0] = ox;
    origin[1] = oy;
    origin[2] = oz;
    voxels.resize((size_t)nx * ny * nz);
}

void VoxelGrid::clearSolid()
{
    for (auto& v : voxels)
        v.value = -1.0f;
}

void VoxelGrid::clearEmpty()
{
    for (auto& v : voxels)
        v.value = 1.0f;
}

bool VoxelGrid::worldToVoxel(float x, float y, float z,
                             int& ix, int& iy, int& iz) const
{
    ix = (int)std::floor((x - origin[0]) / cell + 0.5f);
    iy = (int)std::floor((y - origin[1]) / cell + 0.5f);
    iz = (int)std::floor((z - origin[2]) / cell + 0.5f);
    return inBounds(ix, iy, iz);
}

void VoxelGrid::voxelCenter(int ix, int iy, int iz,
                            float& x, float& y, float& z) const
{
    x = origin[0] + ix * cell;
    y = origin[1] + iy * cell;
    z = origin[2] + iz * cell;
}

float VoxelGrid::cornerValue(int ix, int iy, int iz) const
{
    if (ix < 0 || iy < 0 || iz < 0) return 1.0f;
    if (ix >= nx || iy >= ny || iz >= nz) return 1.0f;
    return voxels[idx(ix, iy, iz)].value;
}
void VoxelGrid::buildDistanceFieldFromBinary()
{
    if (voxels.empty())
        return;

    struct CellPos
    {
        int x, y, z;
    };

    std::vector<unsigned char> wasSolid(voxels.size(), 0);
    std::vector<CellPos> solidCells;
    std::vector<CellPos> emptyCells;

    solidCells.reserve(voxels.size());
    emptyCells.reserve(voxels.size());

    for (int ix = 0; ix < nx; ++ix) {
        for (int iy = 0; iy < ny; ++iy) {
            for (int iz = 0; iz < nz; ++iz) {
                const size_t i = idx(ix, iy, iz);
                const bool solid = voxels[i].value < 0.0f;
                wasSolid[i] = solid ? 1 : 0;

                if (solid)
                    solidCells.push_back({ix, iy, iz});
                else
                    emptyCells.push_back({ix, iy, iz});
            }
        }
    }

    // Degenerate cases: fully solid or fully empty.
    if (solidCells.empty()) {
        for (auto& v : voxels)
            v.value = 1.0f;
        return;
    }

    if (emptyCells.empty()) {
        for (auto& v : voxels)
            v.value = -1.0f;
        return;
    }

    for (int ix = 0; ix < nx; ++ix) {
        for (int iy = 0; iy < ny; ++iy) {
            for (int iz = 0; iz < nz; ++iz) {
                const size_t i = idx(ix, iy, iz);
                const bool solid = wasSolid[i] != 0;

                const std::vector<CellPos>& targets =
                    solid ? emptyCells : solidCells;

                float bestDist2 = std::numeric_limits<float>::max();

                for (const CellPos& p : targets) {
                    const float dx = float(ix - p.x);
                    const float dy = float(iy - p.y);
                    const float dz = float(iz - p.z);
                    const float d2 = dx * dx + dy * dy + dz * dz;

                    if (d2 < bestDist2)
                        bestDist2 = d2;
                }

                const float dist = std::sqrt(bestDist2) * cell;

                // Negative inside, positive outside.
                voxels[i].value = solid ? -dist : dist;
            }
        }
    }
}
