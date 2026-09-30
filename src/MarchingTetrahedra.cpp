#include "MarchingTetrahedra.h"
#include "VoxelGrid.h"

#include <cmath>

namespace
{

const int kCornerOffset[8][3] = {
    {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
};

const int kTetrahedra[6][4] = {
    {0, 5, 1, 6},
    {0, 1, 2, 6},
    {0, 2, 3, 6},
    {0, 3, 7, 6},
    {0, 7, 4, 6},
    {0, 4, 5, 6}
};

inline Vec3 add(const Vec3& a, const Vec3& b)
{
    return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vec3 subtract(const Vec3& a, const Vec3& b)
{
    return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vec3 scale(const Vec3& v, float s)
{
    return Vec3(v.x * s, v.y * s, v.z * s);
}

inline float dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b)
{
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

inline Vec3 normalise(const Vec3& v)
{
    const float len2 = dot(v, v);
    if (len2 <= 1.0e-12f)
        return Vec3(0.0f, 0.0f, 1.0f);

    const float invLen = 1.0f / std::sqrt(len2);
    return scale(v, invLen);
}

inline bool isInside(float value, float isoLevel)
{
    return value <= isoLevel;
}

float sampleField(const VoxelGrid& grid, float x, float y, float z)
{
    const float gx = (x - grid.origin[0]) / grid.cell;
    const float gy = (y - grid.origin[1]) / grid.cell;
    const float gz = (z - grid.origin[2]) / grid.cell;

    const int x0 = static_cast<int>(std::floor(gx));
    const int y0 = static_cast<int>(std::floor(gy));
    const int z0 = static_cast<int>(std::floor(gz));

    const int x1 = x0 + 1;
    const int y1 = y0 + 1;
    const int z1 = z0 + 1;

    const float tx = gx - static_cast<float>(x0);
    const float ty = gy - static_cast<float>(y0);
    const float tz = gz - static_cast<float>(z0);

    const float c000 = grid.cornerValue(x0, y0, z0);
    const float c100 = grid.cornerValue(x1, y0, z0);
    const float c010 = grid.cornerValue(x0, y1, z0);
    const float c110 = grid.cornerValue(x1, y1, z0);
    const float c001 = grid.cornerValue(x0, y0, z1);
    const float c101 = grid.cornerValue(x1, y0, z1);
    const float c011 = grid.cornerValue(x0, y1, z1);
    const float c111 = grid.cornerValue(x1, y1, z1);

    const float c00 = c000 + (c100 - c000) * tx;
    const float c10 = c010 + (c110 - c010) * tx;
    const float c01 = c001 + (c101 - c001) * tx;
    const float c11 = c011 + (c111 - c011) * tx;

    const float c0 = c00 + (c10 - c00) * ty;
    const float c1 = c01 + (c11 - c01) * ty;

    return c0 + (c1 - c0) * tz;
}

Vec3 sampleGradient(const VoxelGrid& grid, const Vec3& p)
{
    const float h = 0.5f * grid.cell;

    const float dx =
        sampleField(grid, p.x + h, p.y, p.z) -
        sampleField(grid, p.x - h, p.y, p.z);

    const float dy =
        sampleField(grid, p.x, p.y + h, p.z) -
        sampleField(grid, p.x, p.y - h, p.z);

    const float dz =
        sampleField(grid, p.x, p.y, p.z + h) -
        sampleField(grid, p.x, p.y, p.z - h);

    // Gradient points toward increasing values.
    return normalise(Vec3(dx, dy, dz));
}

void emitTriangle(TriangleMesh& mesh,
                  const VoxelGrid& grid,
                  Vec3 a, Vec3 b, Vec3 c,
                  const Vec3& outwardHint)
{
    Vec3 faceNormal = cross(subtract(b, a), subtract(c, a));

    if (dot(faceNormal, outwardHint) < 0.0f) {
        const Vec3 tmp = b;
        b = c;
        c = tmp;
        faceNormal = cross(subtract(b, a), subtract(c, a));
    }

    Vec3 na = sampleGradient(grid, a);
    Vec3 nb = sampleGradient(grid, b);
    Vec3 nc = sampleGradient(grid, c);

    // If gradient sampling becomes degenerate, fall back to face normal.
    const Vec3 fallback = normalise(faceNormal);

    if (dot(na, na) <= 1.0e-12f) na = fallback;
    if (dot(nb, nb) <= 1.0e-12f) nb = fallback;
    if (dot(nc, nc) <= 1.0e-12f) nc = fallback;

    // Keep normals consistently outward.
    if (dot(na, outwardHint) < 0.0f) na = scale(na, -1.0f);
    if (dot(nb, outwardHint) < 0.0f) nb = scale(nb, -1.0f);
    if (dot(nc, outwardHint) < 0.0f) nc = scale(nc, -1.0f);

    const unsigned int base =
        static_cast<unsigned int>(mesh.vertices.size());

    mesh.vertices.push_back(a);
    mesh.vertices.push_back(b);
    mesh.vertices.push_back(c);

    mesh.normals.push_back(na);
    mesh.normals.push_back(nb);
    mesh.normals.push_back(nc);

    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 1);
    mesh.indices.push_back(base + 2);
}

void polygoniseTetrahedron(const Vec3 p[4],
                           const float value[4],
                           float isoLevel,
                           const VoxelGrid& grid,
                           TriangleMesh& mesh)
{
    int inside[4];
    int outside[4];
    int insideCount = 0;
    int outsideCount = 0;

    Vec3 insideCenter(0.0f, 0.0f, 0.0f);
    Vec3 outsideCenter(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 4; ++i) {
        if (isInside(value[i], isoLevel)) {
            inside[insideCount++] = i;
            insideCenter = add(insideCenter, p[i]);
        } else {
            outside[outsideCount++] = i;
            outsideCenter = add(outsideCenter, p[i]);
        }
    }

    if (insideCount == 0 || insideCount == 4)
        return;

    insideCenter = scale(insideCenter, 1.0f / static_cast<float>(insideCount));
    outsideCenter = scale(outsideCenter, 1.0f / static_cast<float>(outsideCount));

    const Vec3 outwardHint = subtract(insideCenter, outsideCenter);

    if (insideCount == 1 || insideCount == 3) {
        const bool oneInside = (insideCount == 1);
        const int lone = oneInside ? inside[0] : outside[0];
        const int* other = oneInside ? outside : inside;

        const Vec3 a = MeshBuilder::interpolate(
            isoLevel, p[lone], p[other[0]], value[lone], value[other[0]]);
        const Vec3 b = MeshBuilder::interpolate(
            isoLevel, p[lone], p[other[1]], value[lone], value[other[1]]);
        const Vec3 c = MeshBuilder::interpolate(
            isoLevel, p[lone], p[other[2]], value[lone], value[other[2]]);

        emitTriangle(mesh, grid, a, b, c, outwardHint);
        return;
    }

    const int i0 = inside[0];
    const int i1 = inside[1];
    const int o0 = outside[0];
    const int o1 = outside[1];

    const Vec3 a = MeshBuilder::interpolate(
        isoLevel, p[i0], p[o0], value[i0], value[o0]);
    const Vec3 b = MeshBuilder::interpolate(
        isoLevel, p[i0], p[o1], value[i0], value[o1]);
    const Vec3 c = MeshBuilder::interpolate(
        isoLevel, p[i1], p[o1], value[i1], value[o1]);
    const Vec3 d = MeshBuilder::interpolate(
        isoLevel, p[i1], p[o0], value[i1], value[o0]);

    emitTriangle(mesh, grid, a, b, c, outwardHint);
    emitTriangle(mesh, grid, a, c, d, outwardHint);
}

} // namespace

TriangleMesh MarchingTetrahedra::build(const VoxelGrid& grid, float isoLevel)
{
    TriangleMesh mesh;

    if (grid.nx < 2 || grid.ny < 2 || grid.nz < 2)
        return mesh;

    for (int ix = 0; ix < grid.nx - 1; ++ix) {
        for (int iy = 0; iy < grid.ny - 1; ++iy) {
            for (int iz = 0; iz < grid.nz - 1; ++iz) {
                Vec3 cubePos[8];
                float cubeValue[8];
                int caseMask = 0;

                for (int c = 0; c < 8; ++c) {
                    const int sx = ix + kCornerOffset[c][0];
                    const int sy = iy + kCornerOffset[c][1];
                    const int sz = iz + kCornerOffset[c][2];

                    float px, py, pz;
                    grid.voxelCenter(sx, sy, sz, px, py, pz);

                    cubePos[c] = Vec3(px, py, pz);
                    cubeValue[c] = grid.cornerValue(sx, sy, sz);

                    if (cubeValue[c] <= isoLevel)
                        caseMask |= (1 << c);
                }

                if (caseMask == 0 || caseMask == 255)
                    continue;

                for (int t = 0; t < 6; ++t) {
                    Vec3 tetPos[4];
                    float tetValue[4];

                    for (int v = 0; v < 4; ++v) {
                        const int corner = kTetrahedra[t][v];
                        tetPos[v] = cubePos[corner];
                        tetValue[v] = cubeValue[corner];
                    }

                    polygoniseTetrahedron(tetPos, tetValue, isoLevel, grid, mesh);
                }
            }
        }
    }

    return mesh;
}
