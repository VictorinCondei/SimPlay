/*
 * ToolPath.h flat list of linear cutter moves.
 *
 * Produced by ToolPathLoader from a parsed NCprog. Each Move is one linear
 * segment (G00 rapid or G01 feed); long segments are already sub-sampled by
 * the loader so the Simulator can step at sub-voxel resolution without
 * further subdivision.
 */
#ifndef SRC_TOOLPATH_H_
#define SRC_TOOLPATH_H_

#include <vector>
#include <cstddef>

struct Move
{
    float x0, y0, z0;     // segment start (world)
    float x1, y1, z1;     // segment end   (world)
    int   gcode;          // 0 = rapid (G00), 1 = feed (G01)
    float feed;           // feed rate (units/min)

    Move() : x0(0),y0(0),z0(0), x1(0),y1(0),z1(0), gcode(1), feed(0) {}
};

class ToolPath
{
public:
    std::vector<Move> moves;
    float Xmin, Xmax, Ymin, Ymax, Zmin, Zmax;

    ToolPath();

    void   addMove(const Move& m);
    void   computeBounds();
    size_t size() const { return moves.size(); }

    // Total linear length of feed moves (rapids excluded from the cut, since
    // the tool is above the stock).
    float totalFeedLength() const;
};

#endif /* SRC_TOOLPATH_H_ */
