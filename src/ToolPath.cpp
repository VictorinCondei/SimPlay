#include "ToolPath.h"
#include <cmath>
#include <algorithm>

ToolPath::ToolPath()
    : Xmin(0), Xmax(0), Ymin(0), Ymax(0), Zmin(0), Zmax(0) {}

void ToolPath::addMove(const Move& m) { moves.push_back(m); }

void ToolPath::computeBounds()
{
    Xmin = Ymin = Zmin =  1.0e30f;
    Xmax = Ymax = Zmax = -1.0e30f;
    for (const auto& m : moves) {
        for (int i = 0; i < 2; ++i) {
            float x = (i == 0 ? m.x0 : m.x1);
            float y = (i == 0 ? m.y0 : m.y1);
            float z = (i == 0 ? m.z0 : m.z1);
            Xmin = std::min(Xmin, x); Xmax = std::max(Xmax, x);
            Ymin = std::min(Ymin, y); Ymax = std::max(Ymax, y);
            Zmin = std::min(Zmin, z); Zmax = std::max(Zmax, z);
        }
    }
}

float ToolPath::totalFeedLength() const
{
    float L = 0.0f;
    for (const auto& m : moves) {
        if (m.gcode == 0) continue;       // rapid g0 not a cut
        float dx = m.x1 - m.x0;
        float dy = m.y1 - m.y0;
        float dz = m.z1 - m.z0;
        L += std::sqrt(dx*dx + dy*dy + dz*dz);
    }
    return L;
}
