/*
 * Simulator.cpp
 *
 * Builds an initial stock signed-distance field around the cutting region,
 * subtracts the swept cutter volume for feed moves, and extracts the
 * renderable boundary mesh.
 */
#include "Simulator.h"
#include "MarchingTetrahedra.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{

bool isCuttingMove(const Move& move)
{
    // G01 is linear feed. G02/G03 are assumed to have been linearised
    // into short segments by ToolPathLoader and are also cutting motion.
    return move.gcode == 1 || move.gcode == 2 || move.gcode == 3;
}

} // namespace

Simulator::Simulator()
    : m_currentMove(0),
      m_stockMinX(0.0f), m_stockMinY(0.0f), m_stockMinZ(0.0f),
      m_stockMaxX(0.0f), m_stockMaxY(0.0f), m_stockMaxZ(0.0f)
{
}

void Simulator::initialise(const ToolPath& path,
                           float stockMargin,
                           int resolution,
                           CutterType cutterType,
                           float cutterRadius,
                           float cutterLength)
{
    m_path = path;
    m_currentMove = 0;
    m_mesh.clear();
    m_trail.clear();
    m_cutter = Cutter(cutterType, cutterRadius, cutterLength);

    if (m_path.moves.empty()) {
        std::cerr << "Simulator: cannot initialise an empty toolpath." << std::endl;
        return;
    }

    float cutMinX =  1.0e30f;
    float cutMinY =  1.0e30f;
    float cutMinZ =  1.0e30f;
    float cutMaxX = -1.0e30f;
    float cutMaxY = -1.0e30f;
    float cutMaxZ = -1.0e30f;
    bool haveCut = false;

    // Pass 1: collect full cutting Z range.
    for (const Move& move : m_path.moves) {
        if (!isCuttingMove(move))
            continue;

        haveCut = true;
        cutMinZ = std::min(cutMinZ, std::min(move.z0, move.z1));
        cutMaxZ = std::max(cutMaxZ, std::max(move.z0, move.z1));
    }

    if (!haveCut) {
        cutMinX = m_path.Xmin;
        cutMaxX = m_path.Xmax;
        cutMinY = m_path.Ymin;
        cutMaxY = m_path.Ymax;
        cutMinZ = m_path.Zmin;
        cutMaxZ = m_path.Zmax;
    } else {
        // Pass 2: derive XY bounds from moves that are close to the actual cutting depth.
        // This keeps "stock from toolpath", but ignores long high-level sweeps that make
        // the stock unrealistically huge for shallow jobs.
        const float xyZThreshold = std::max(cutterRadius * 2.0f, 2.0f);

        bool haveActiveXY = false;

        for (const Move& move : m_path.moves) {
            if (!isCuttingMove(move))
                continue;

            const float localMinZ = std::min(move.z0, move.z1);
            if (localMinZ > cutMinZ + xyZThreshold)
                continue;

            haveActiveXY = true;
            cutMinX = std::min(cutMinX, std::min(move.x0, move.x1));
            cutMaxX = std::max(cutMaxX, std::max(move.x0, move.x1));
            cutMinY = std::min(cutMinY, std::min(move.y0, move.y1));
            cutMaxY = std::max(cutMaxY, std::max(move.y0, move.y1));
        }

        // Fallback: if the threshold rejected everything, use all cutting XY.
        if (!haveActiveXY) {
            cutMinX =  1.0e30f;
            cutMinY =  1.0e30f;
            cutMaxX = -1.0e30f;
            cutMaxY = -1.0e30f;

            for (const Move& move : m_path.moves) {
                if (!isCuttingMove(move))
                    continue;

                cutMinX = std::min(cutMinX, std::min(move.x0, move.x1));
                cutMaxX = std::max(cutMaxX, std::max(move.x0, move.x1));
                cutMinY = std::min(cutMinY, std::min(move.y0, move.y1));
                cutMaxY = std::max(cutMaxY, std::max(move.y0, move.y1));
            }
        }
    }


    if (!haveCut) {
        cutMinX = m_path.Xmin;
        cutMaxX = m_path.Xmax;
        cutMinY = m_path.Ymin;
        cutMaxY = m_path.Ymax;
        cutMinZ = m_path.Zmin;
        cutMaxZ = m_path.Zmax;
    }

    stockMargin = std::max(stockMargin, cutterRadius * 1.5f);
    resolution = std::max(resolution, 16);

    m_stockMinX = cutMinX - stockMargin;
    m_stockMaxX = cutMaxX + stockMargin;
    m_stockMinY = cutMinY - stockMargin;
    m_stockMaxY = cutMaxY + stockMargin;
    m_stockMinZ = cutMinZ - stockMargin;
    m_stockMaxZ = cutMaxZ + cutterRadius + stockMargin * 0.25f;

    const float extentX = std::max(m_stockMaxX - m_stockMinX, 1.0f);
    const float extentY = std::max(m_stockMaxY - m_stockMinY, 1.0f);
    const float extentZ = std::max(m_stockMaxZ - m_stockMinZ, 1.0f);

    const float largestExtent = std::max(extentX, std::max(extentY, extentZ));
    float cell = largestExtent / static_cast<float>(resolution - 1);

    // Small safety improvement only:
    // if Z would collapse to fewer than ~6 cells, refine a bit,
    // but never by more than 2x versus the original cell.
    if (extentZ / cell < 6.0f)
        cell *= 0.5f;

    int nx = std::max(2, static_cast<int>(std::ceil(extentX / cell)) + 1);
    int ny = std::max(2, static_cast<int>(std::ceil(extentY / cell)) + 1);
    int nz = std::max(2, static_cast<int>(std::ceil(extentZ / cell)) + 1);

    // Hard cap for startup sanity.
    const long long maxVoxels = 2000000LL;
    long long totalVoxels =
        static_cast<long long>(nx) *
        static_cast<long long>(ny) *
        static_cast<long long>(nz);

    if (totalVoxels > maxVoxels) {
        const float scale =
            std::cbrt(static_cast<float>(totalVoxels) /
                      static_cast<float>(maxVoxels));

        cell *= scale;

        nx = std::max(2, static_cast<int>(std::ceil(extentX / cell)) + 1);
        ny = std::max(2, static_cast<int>(std::ceil(extentY / cell)) + 1);
        nz = std::max(2, static_cast<int>(std::ceil(extentZ / cell)) + 1);

        totalVoxels =
            static_cast<long long>(nx) *
            static_cast<long long>(ny) *
            static_cast<long long>(nz);
    }

    m_grid.init(nx, ny, nz, cell, m_stockMinX, m_stockMinY, m_stockMinZ);

    rebuildStockField();

    const Move& first = m_path.moves.front();
    m_cutter.x = first.x0;
    m_cutter.y = first.y0;
    m_cutter.z = first.z0;

    m_trail.push_back({m_cutter.x, m_cutter.y, m_cutter.z, false});

    //rebuildMesh();

    std::cout << "Simulator: cutting bounds X[" << cutMinX << ".." << cutMaxX
              << "] Y[" << cutMinY << ".." << cutMaxY
              << "] Z[" << cutMinZ << ".." << cutMaxZ << "]\n"
              << "Simulator: stock bounds X[" << m_stockMinX << ".." << m_stockMaxX
              << "] Y[" << m_stockMinY << ".." << m_stockMaxY
              << "] Z[" << m_stockMinZ << ".." << m_stockMaxZ << "]\n"
              << "Simulator: grid " << nx << "x" << ny << "x" << nz
              << ", cell=" << cell
              << ", stock voxels=" << totalVoxels
              << ", initial triangles=" << m_mesh.triangleCount()
              << std::endl;
}


float Simulator::stockSignedDistance(float x, float y, float z) const
{
    const float dx =
        std::max(std::max(m_stockMinX - x, 0.0f), x - m_stockMaxX);
    const float dy =
        std::max(std::max(m_stockMinY - y, 0.0f), y - m_stockMaxY);
    const float dz =
        std::max(std::max(m_stockMinZ - z, 0.0f), z - m_stockMaxZ);

    const bool outside = (dx > 0.0f || dy > 0.0f || dz > 0.0f);

    if (outside)
        return std::sqrt(dx * dx + dy * dy + dz * dz);

    const float distToMinX = x - m_stockMinX;
    const float distToMaxX = m_stockMaxX - x;
    const float distToMinY = y - m_stockMinY;
    const float distToMaxY = m_stockMaxY - y;
    const float distToMinZ = z - m_stockMinZ;
    const float distToMaxZ = m_stockMaxZ - z;

    const float insideDist =
        std::min(distToMinX,
        std::min(distToMaxX,
        std::min(distToMinY,
        std::min(distToMaxY,
        std::min(distToMinZ, distToMaxZ)))));

    return -insideDist;
}

void Simulator::rebuildStockField()
{
    if (m_grid.voxels.empty())
        return;

    for (int ix = 0; ix < m_grid.nx; ++ix) {
        for (int iy = 0; iy < m_grid.ny; ++iy) {
            for (int iz = 0; iz < m_grid.nz; ++iz) {
                float x, y, z;
                m_grid.voxelCenter(ix, iy, iz, x, y, z);
                m_grid.setValue(ix, iy, iz, stockSignedDistance(x, y, z));
            }
        }
    }
}

void Simulator::carveMove(const Move& move)
{
    if (!isCuttingMove(move)) {
        m_cutter.x = move.x1;
        m_cutter.y = move.y1;
        m_cutter.z = move.z1;
        return;
    }

    const float dx = move.x1 - move.x0;
    const float dy = move.y1 - move.y0;
    const float dz = move.z1 - move.z0;
    const float length = std::sqrt(dx * dx + dy * dy + dz * dz);

    // Dense enough that adjacent cutter samples overlap safely.
    const float spacing = std::max(m_grid.cell * 0.25f, 0.001f);
    const int samples = std::max(1, static_cast<int>(std::ceil(length / spacing)));

    for (int sample = 0; sample <= samples; ++sample) {
        const float t = static_cast<float>(sample) / static_cast<float>(samples);
        m_cutter.x = move.x0 + dx * t;
        m_cutter.y = move.y0 + dy * t;
        m_cutter.z = move.z0 + dz * t;
        m_cutter.carve(m_grid);
    }
}

bool Simulator::step()
{
    if (finished())
        return false;

    const Move& move = m_path.moves[m_currentMove];
    carveMove(move);
    ++m_currentMove;

    const bool rapid = (move.gcode == 0);
    m_trail.push_back({m_cutter.x, m_cutter.y, m_cutter.z, rapid});

    return true;
}


void Simulator::runToEnd()
{
    while (step()) {}
    rebuildMesh();
}

void Simulator::rebuildMesh()
{
    const bool useMarchingTetrahedra = true;

    if (useMarchingTetrahedra)
        m_mesh = MarchingTetrahedra::build(m_grid, 0.0f);
    else
        m_mesh = MeshBuilder::build(m_grid, 0.0f);
}


void Simulator::reset()
{
    if (m_grid.voxels.empty())
        return;

    rebuildStockField();

    m_currentMove = 0;
    m_trail.clear();

    if (!m_path.moves.empty()) {
        const Move& first = m_path.moves.front();
        m_cutter.x = first.x0;
        m_cutter.y = first.y0;
        m_cutter.z = first.z0;
        m_trail.push_back({m_cutter.x, m_cutter.y, m_cutter.z, false});
    }

    rebuildMesh();
}

bool Simulator::finished() const
{
    return m_currentMove >= m_path.size();
}

float Simulator::centerX() const
{
    return m_grid.origin[0] +
           (m_grid.nx - 1) * m_grid.cell * 0.5f;
}

float Simulator::centerY() const
{
    return m_grid.origin[1] +
           (m_grid.ny - 1) * m_grid.cell * 0.5f;
}

float Simulator::centerZ() const
{
    return m_grid.origin[2] +
           (m_grid.nz - 1) * m_grid.cell * 0.5f;
}
