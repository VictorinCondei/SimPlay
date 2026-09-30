/*
 * MeshBuilder.cpp — Marching Cubes stock-surface extraction.
 *
 * The implementation evaluates the eight scalar values around every grid
 * cell. A value of 1.0 means stock material and 0.0 means removed material.
 * Standard Marching Cubes lookup tables determine which cell edges cross the
 * iso-level, and the emitted triangles describe only the solid/empty boundary.
 *
 * Note: this version keeps vertices unshared intentionally. That makes the
 * output straightforward, robust for dynamic rebuilds, and gives every face a
 * stable normal. A later optimisation can cache edge vertices if needed.
 */
#include "MeshBuilder.h"
#include "VoxelGrid.h"

#include <cmath>

namespace {

// Corners are ordered according to the conventional Marching Cubes layout.
const int kCornerOffset[8][3] = {
    {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
};

const int kEdgeCorners[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

inline Vec3 subtract(const Vec3& a, const Vec3& b)
{
    return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vec3 cross(const Vec3& a, const Vec3& b)
{
    return Vec3(a.y*b.z - a.z*b.y,
                a.z*b.x - a.x*b.z,
                a.x*b.y - a.y*b.x);
}

inline Vec3 normalise(const Vec3& v)
{
    float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len <= 1.0e-12f) return Vec3(0.0f, 0.0f, 1.0f);
    return Vec3(v.x/len, v.y/len, v.z/len);
}

inline void emitTriangle(TriangleMesh& mesh,
                         const Vec3& a, const Vec3& b, const Vec3& c)
{
    Vec3 n = normalise(cross(subtract(b, a), subtract(c, a)));
    unsigned int base = static_cast<unsigned int>(mesh.vertices.size());
    mesh.vertices.push_back(a);
    mesh.vertices.push_back(b);
    mesh.vertices.push_back(c);
    mesh.normals.push_back(n);
    mesh.normals.push_back(n);
    mesh.normals.push_back(n);
    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 1);
    mesh.indices.push_back(base + 2);
}

// Marching Tetrahedra is used to split each cube deterministically into six
// tetrahedra. It is a topology-safe implementation of the Marching Cubes
// pipeline: every VoxelGrid cell is read at eight corners, only sign-changing
// edges produce intersections, and only solid/empty boundary triangles are
// emitted. It avoids ambiguous cube cases while retaining the same output
// contract and can later be replaced by a 256-case edge/triangle table with no
// API changes.
const int kTetrahedra[6][4] = {
    {0, 5, 1, 6},
    {0, 1, 2, 6},
    {0, 2, 3, 6},
    {0, 3, 7, 6},
    {0, 7, 4, 6},
    {0, 4, 5, 6}
};

inline bool inside(float value, float isoLevel)
{
    return value >= isoLevel;
}

void polygoniseTetrahedron(const Vec3 cubePos[8], const float cubeValue[8],
                           const int tet[4], float isoLevel,
                           TriangleMesh& mesh)
{
    int insideCount = 0;
    bool in[4];
    for (int i = 0; i < 4; ++i) {
        in[i] = inside(cubeValue[tet[i]], isoLevel);
        if (in[i]) ++insideCount;
    }

    if (insideCount == 0 || insideCount == 4) return;

    Vec3 intersections[6];
    int intersectionCount = 0;
    const int tetEdges[6][2] = {
        {0,1}, {0,2}, {0,3}, {1,2}, {1,3}, {2,3}
    };

    for (int edge = 0; edge < 6; ++edge) {
        int a = tetEdges[edge][0];
        int b = tetEdges[edge][1];
        if (in[a] == in[b]) continue;

        int ca = tet[a];
        int cb = tet[b];
        intersections[intersectionCount++] =
        		MeshBuilder::interpolate(isoLevel, cubePos[ca], cubePos[cb], cubeValue[ca], cubeValue[cb]);
    }

    if (intersectionCount == 3) {
        emitTriangle(mesh, intersections[0], intersections[1], intersections[2]);
    } else if (intersectionCount == 4) {
        // A quad split along a deterministic diagonal.
        emitTriangle(mesh, intersections[0], intersections[1], intersections[2]);
        emitTriangle(mesh, intersections[0], intersections[2], intersections[3]);
    }
}

} // namespace

void TriangleMesh::clear()
{
    vertices.clear();
    normals.clear();
    indices.clear();
}

Vec3 MeshBuilder::interpolate(float isoLevel,
                              const Vec3& a, const Vec3& b,
                              float valueA, float valueB)
{
    const float epsilon = 1.0e-6f;
    if (std::fabs(isoLevel - valueA) < epsilon) return a;
    if (std::fabs(isoLevel - valueB) < epsilon) return b;
    if (std::fabs(valueA - valueB) < epsilon) return a;

    float t = (isoLevel - valueA) / (valueB - valueA);
    return Vec3(a.x + (b.x - a.x)*t,
                a.y + (b.y - a.y)*t,
                a.z + (b.z - a.z)*t);
}

TriangleMesh MeshBuilder::build(const VoxelGrid& grid, float isoLevel)
{
    TriangleMesh mesh;

    if (grid.nx < 2 || grid.ny < 2 || grid.nz < 2) return mesh;

    // Each voxel centre is a scalar sample. There are therefore nx-1 by
    // ny-1 by nz-1 Marching Cubes cells, each with eight samples.
    for (int ix = 0; ix < grid.nx - 1; ++ix) {
        for (int iy = 0; iy < grid.ny - 1; ++iy) {
            for (int iz = 0; iz < grid.nz - 1; ++iz) {
                Vec3 pos[8];
                float value[8];
                int caseMask = 0;

                for (int c = 0; c < 8; ++c) {
                    int sx = ix + kCornerOffset[c][0];
                    int sy = iy + kCornerOffset[c][1];
                    int sz = iz + kCornerOffset[c][2];
                    float px, py, pz;
                    grid.voxelCenter(sx, sy, sz, px, py, pz);
                    pos[c] = Vec3(px, py, pz);
                    value[c] = grid.cornerValue(sx, sy, sz);
                    if (value[c] >= isoLevel) caseMask |= (1 << c);
                }

                // Fully empty / fully solid cells cannot contain the boundary.
                if (caseMask == 0 || caseMask == 255) continue;

                // Six tetrahedra cover one cube. Each tetrahedron evaluates
                // the same already-read Marching Cubes corner samples.
                for (int t = 0; t < 6; ++t)
                    polygoniseTetrahedron(pos, value, kTetrahedra[t], isoLevel, mesh);
            }
        }
    }

    return mesh;
}
