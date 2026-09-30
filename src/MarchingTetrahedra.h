#ifndef SRC_MARCHINGTETRAHEDRA_H_
#define SRC_MARCHINGTETRAHEDRA_H_

#include "MeshBuilder.h"

class VoxelGrid;

class MarchingTetrahedra
{
public:
    static TriangleMesh build(const VoxelGrid& grid,
                              float isoLevel = 0.0f);
};

#endif // SRC_MARCHINGTETRAHEDRA_H_
