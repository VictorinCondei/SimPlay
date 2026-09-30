#ifndef SRC_SIMULATOR_H_
#define SRC_SIMULATOR_H_

#include "Cutter.h"
#include "MeshBuilder.h"
#include "ToolPath.h"
#include "VoxelGrid.h"

#include <vector>

struct TrailPoint
{
    float x, y, z;
    bool rapid;
};

class Simulator
{
public:
    Simulator();

    void initialise(const ToolPath& path,
                    float stockMargin,
                    int resolution,
                    CutterType cutterType,
                    float cutterRadius,
                    float cutterLength);

    bool step();
    void runToEnd();
    void reset();

    bool finished() const;

    float centerX() const;
    float centerY() const;
    float centerZ() const;

    const TriangleMesh& mesh() const { return m_mesh; }
    const std::vector<TrailPoint>& trail() const { return m_trail; }
    const Cutter& cutter() const { return m_cutter; }
    const VoxelGrid&      grid()          const { return m_grid;          }
    std::size_t           currentMove()   const { return m_currentMove;   }
    std::size_t           moveCount()     const { return m_path.size();   }

//private:
    void carveMove(const Move& move);
    void rebuildMesh();

    void rebuildStockField();
    float stockSignedDistance(float x, float y, float z) const;

private:
    ToolPath m_path;
    std::size_t m_currentMove;

    VoxelGrid m_grid;
    Cutter m_cutter;
    TriangleMesh m_mesh;
    std::vector<TrailPoint> m_trail;

    float m_stockMinX, m_stockMinY, m_stockMinZ;
    float m_stockMaxX, m_stockMaxY, m_stockMaxZ;
};

#endif /* SRC_SIMULATOR_H_ */
