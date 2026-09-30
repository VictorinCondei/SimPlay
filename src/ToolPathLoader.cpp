/*
 * ToolPathLoader.cpp
 *
 * Converts legacy NCprog / NCinstr output into explicit linear tool motions.
 *
 * Important: NCprog preserves each raw line. Coordinate-only lines such as
 * `X152.997Y83.8` may not have `ins.type == 'G'`, even though they inherit the
 * preceding G01. This loader therefore owns motion-mode state explicitly.
 */
#include "ToolPathLoader.h"
#include "util/NCprog.h"
#include "util/NCinstr.h"

#include <cmath>
#include <iostream>

namespace {

bool isMotionCode(int code)
{
    return code == 0 || code == 1 || code == 2 || code == 3;
}

bool isDefined(double value)
{
    return value != undefDouble;
}

void appendSubdividedMove(ToolPath& out, const Move& move, float maxStep)
{
    const float dx = move.x1 - move.x0;
    const float dy = move.y1 - move.y0;
    const float dz = move.z1 - move.z0;
    const float length = std::sqrt(dx*dx + dy*dy + dz*dz);

    if (maxStep <= 0.0f || length <= maxStep) {
        out.addMove(move);
        return;
    }

    const int segments = static_cast<int>(std::ceil(length / maxStep));
    for (int i = 0; i < segments; ++i) {
        const float t0 = static_cast<float>(i) / static_cast<float>(segments);
        const float t1 = static_cast<float>(i + 1) / static_cast<float>(segments);

        Move part = move;
        part.x0 = move.x0 + dx * t0;
        part.y0 = move.y0 + dy * t0;
        part.z0 = move.z0 + dz * t0;
        part.x1 = move.x0 + dx * t1;
        part.y1 = move.y0 + dy * t1;
        part.z1 = move.z0 + dz * t1;
        out.addMove(part);
    }
}

} // namespace

bool ToolPathLoader::load(const std::string& fileName, ToolPath& out, float maxStep)
{
    out.moves.clear();

    NCprog program(fileName);
    if (program.TotalPoints <= 0) {
        std::cerr << "ToolPathLoader: no NC instructions in " << fileName << std::endl;
        return false;
    }

    // The ISO sample uses absolute G90 coordinates. The loader keeps modal
    // coordinate state, so omitted X/Y/Z values inherit the previous endpoint.
    float currentX = 0.0f;
    float currentY = 0.0f;
    float currentZ = 0.0f;
    bool havePosition = false;

    int activeMotion = -1;
    float activeFeed = 100.0f;

    for (const NCinstr& ins : program.prog) {
        // An explicitly written G00/G01/G02/G03 changes the modal motion.
        if (ins.type == 'G' && isMotionCode(ins.code))
            activeMotion = ins.code;

        if (ins.F != undefInt)
            activeFeed = static_cast<float>(ins.F);

        const bool hasX = isDefined(ins.X);
        const bool hasY = isDefined(ins.Y);
        const bool hasZ = isDefined(ins.Z);
        if (!hasX && !hasY && !hasZ)
            continue;

        // Do not emit motion before a valid G00/G01/G02/G03 is active.
        // This also filters setup lines such as G16 I0 J0 K1000.
        if (!isMotionCode(activeMotion))
            continue;

        const float nextX = hasX ? static_cast<float>(ins.X) : currentX;
        const float nextY = hasY ? static_cast<float>(ins.Y) : currentY;
        const float nextZ = hasZ ? static_cast<float>(ins.Z) : currentZ;

        // The first coordinate establishes the starting point; it is not a
        // synthetic move from origin. In the supplied program this is the
        // G00 X145.546 Y77.904 Z95 line.
        if (!havePosition) {
            currentX = nextX;
            currentY = nextY;
            currentZ = nextZ;
            havePosition = true;
            continue;
        }

        Move move;
        move.x0 = currentX;
        move.y0 = currentY;
        move.z0 = currentZ;
        move.x1 = nextX;
        move.y1 = nextY;
        move.z1 = nextZ;
        move.gcode = activeMotion;
        move.feed = activeFeed;

        // Ignore duplicate points, but retain the endpoint in modal state.
        const float dx = move.x1 - move.x0;
        const float dy = move.y1 - move.y0;
        const float dz = move.z1 - move.z0;
        if (dx*dx + dy*dy + dz*dz > 1.0e-10f)
            appendSubdividedMove(out, move, maxStep);

        currentX = nextX;
        currentY = nextY;
        currentZ = nextZ;
    }

    if (out.moves.empty()) {
        std::cerr << "ToolPathLoader: no supported positional motions in "
                  << fileName << std::endl;
        return false;
    }

    // ---- Diagnostic: check continuity ----
    for (size_t i = 1; i < out.moves.size(); ++i) {
        const Move& prev = out.moves[i - 1];
        const Move& curr = out.moves[i];

        float dx = curr.x0 - prev.x1;
        float dy = curr.y0 - prev.y1;
        float dz = curr.z0 - prev.z1;

        float gap = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (gap > 1e-5f) {
            std::cout << "\n*** GAP DETECTED ***\n";
            std::cout << "Between move " << i - 1 << " and " << i << '\n';

            std::cout << "Previous end : ("
                      << prev.x1 << ", "
                      << prev.y1 << ", "
                      << prev.z1 << ")\n";

            std::cout << "Current start: ("
                      << curr.x0 << ", "
                      << curr.y0 << ", "
                      << curr.z0 << ")\n";

            std::cout << "Gap = " << gap << "\n\n";
        }
    }
    // ---- End diagnostic ----

    out.computeBounds();
    std::cout << "ToolPathLoader: " << out.size() << " moves, "
              << "feed length=" << out.totalFeedLength() << ", "
              << "bounds X[" << out.Xmin << ".." << out.Xmax << "] "
              << "Y[" << out.Ymin << ".." << out.Ymax << "] "
              << "Z[" << out.Zmin << ".." << out.Zmax << "]"
              << std::endl;
    return true;
}
