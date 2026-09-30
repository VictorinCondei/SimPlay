#ifndef SRC_CUTTER_H_
#define SRC_CUTTER_H_

#include "VoxelGrid.h"

enum class CutterType
{
    FLAT_END,
    BALL_END
};

class Cutter
{
public:
    CutterType type;
    float radius;
    float length;
    float x, y, z;

    Cutter(CutterType t = CutterType::FLAT_END,
           float r = 1.0f,
           float len = 5.0f);

    float signedDistance(float px, float py, float pz) const;

    void bounds(float& minX, float& minY, float& minZ,
                float& maxX, float& maxY, float& maxZ) const;

    void carve(VoxelGrid& grid) const;
    void carveAt(VoxelGrid& grid, float px, float py, float pz) const;
    void carveSegment(VoxelGrid& grid,
                      float x0, float y0, float z0,
                      float x1, float y1, float z1) const;
};

#endif
