#include "Config.h"

#include <fstream>
#include <algorithm>
#include <cstdlib>

bool Config::load(const std::string& file)
{
    std::ifstream in(file);
    if (!in.is_open())
        return false;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key   = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        key.erase  (std::remove_if(key.begin(),   key.end(),   ::isspace), key.end());
        value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

        // ── existing keys ────────────────────────────────────────────────
        if      (key == "maxStep")       maxStep       = std::stof(value);
        else if (key == "resolution")    resolution    = std::stoi(value);
        else if (key == "toolDiameter")  toolDiameter  = std::stof(value);
        else if (key == "movesPerFrame") movesPerFrame = std::stoi(value);
        else if (key == "wireframe")     wireframe     = (value == "true");
        else if (key == "windowWidth")   windowWidth   = std::stoi(value);
        else if (key == "windowHeight")  windowHeight  = std::stoi(value);
        else if (key == "stockMargin") {
            if (value != "auto") stockMargin = std::stof(value);
        }
        else if (key == "toolLength") {
            if (value != "auto") toolLength = std::stof(value);
        }

        // ── new toolpath colour / width keys ─────────────────────────────
        else if (key == "toolpathFeedR")     toolpathFeedR     = std::stof(value);
        else if (key == "toolpathFeedG")     toolpathFeedG     = std::stof(value);
        else if (key == "toolpathFeedB")     toolpathFeedB     = std::stof(value);
        else if (key == "toolpathRapidR")    toolpathRapidR    = std::stof(value);
        else if (key == "toolpathRapidG")    toolpathRapidG    = std::stof(value);
        else if (key == "toolpathRapidB")    toolpathRapidB    = std::stof(value);
        else if (key == "toolpathLineWidth") toolpathLineWidth = std::stof(value);
    }

    return true;
}
