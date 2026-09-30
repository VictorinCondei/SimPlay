#ifndef SRC_VOXELGRID_H_
#define SRC_VOXELGRID_H_

#include <vector>
#include <cstdint>
#include <cstddef>

struct Voxel
{
    float value;

    Voxel()
        : value(1.0f)   // default = outside
    {
    }
};

class VoxelGrid
{
public:
    int   nx, ny, nz;
    float cell;
    float origin[3];

    std::vector<Voxel> voxels;

    VoxelGrid();
    void init(int nx, int ny, int nz, float cell,
              float ox, float oy, float oz);

    void clearSolid();   // all inside
    void clearEmpty();   // all outside

    inline size_t idx(int ix, int iy, int iz) const {
        return (size_t)ix * ny * nz + (size_t)iy * nz + (size_t)iz;
    }

    inline bool inBounds(int ix, int iy, int iz) const {
        return ix >= 0 && ix < nx &&
               iy >= 0 && iy < ny &&
               iz >= 0 && iz < nz;
    }

    inline float value(int ix, int iy, int iz) const
    {
        if (!inBounds(ix, iy, iz))
            return 1.0f; // outside

        return voxels[idx(ix, iy, iz)].value;
    }

    inline void setValue(int ix, int iy, int iz, float v)
    {
        if (!inBounds(ix, iy, iz))
            return;

        voxels[idx(ix, iy, iz)].value = v;
    }

    inline bool isSolid(int ix, int iy, int iz) const
    {
        return value(ix, iy, iz) < 0.0f;
    }

    inline void setSolid(int ix, int iy, int iz, bool solid)
    {
        setValue(ix, iy, iz, solid ? -0.5f : 0.5f);
    }

    bool worldToVoxel(float x, float y, float z,
                      int& ix, int& iy, int& iz) const;

    void voxelCenter(int ix, int iy, int iz,
                     float& x, float& y, float& z) const;

    float cornerValue(int ix, int iy, int iz) const;

    void buildDistanceFieldFromBinary();
};

#endif
