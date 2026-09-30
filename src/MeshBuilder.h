/*
 * MeshBuilder.h — Marching Cubes mesh generator.
 *
 * Reads a VoxelGrid's solid/empty scalar field and generates triangles only
 * where solid material meets empty space. The output mesh is ready for the
 * OpenGL Renderer.
 */
#ifndef SIMPLAY_MESHBUILDER_H
#define SIMPLAY_MESHBUILDER_H

#include <cstddef>
#include <vector>

class VoxelGrid;

struct Vec3
{
    float x;
    float y;
    float z;

    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float px, float py, float pz) : x(px), y(py), z(pz) {}
};

struct TriangleMesh
{
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<unsigned int> indices;

    void clear();
    std::size_t triangleCount() const { return indices.size() / 3; }
    bool empty() const { return indices.empty(); }
};

class MeshBuilder
{
public:
    // Rebuild the complete stock boundary. `isoLevel` should normally remain
    // 0.5 because VoxelGrid stores 1.0 for solid and 0.0 for removed material.
    static TriangleMesh build(const VoxelGrid& grid, float isoLevel = 0.0f);

//private:
    static Vec3 interpolate(float isoLevel,
                            const Vec3& a, const Vec3& b,
                            float valueA, float valueB);
};

#endif // SIMPLAY_MESHBUILDER_H
