/*
 * ToolPathLoader.h converts a parsed NCprog into a flat ToolPath.
 */
#ifndef SRC_TOOLPATH_LOADER_H_
#define SRC_TOOLPATH_LOADER_H_

#include <string>
#include "ToolPath.h"

class ToolPathLoader
{
public:
    // Loads a .iso / .nc / .gcode file via NCprog, converts to a flat
    // ToolPath of linear (G00/G01) moves. Long segments are sub-sampled at
    // maxStep so the Simulator can step the cutter at sub-voxel resolution.
    // Arc moves (G02/G03) are linearised to single segments â€” full arc
    // tessellation from I/J is out of scope for the core.
    static bool load(const std::string& fileName, ToolPath& out, float maxStep = 1.0f);
	//static bool load(const std::string& fileName, ToolPath& out, const std::vector<float>& ratios = std::vector<float>());
};

#endif /* SRC_TOOLPATH_LOADER_H_ */
