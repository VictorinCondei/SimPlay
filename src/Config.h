#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config
{
    float maxStep        = 1.0f;
    int   resolution     = 96;
    float toolDiameter   = 3.0f;
    float stockMargin    = -1.0f;   // negative → auto
    float toolLength     = -1.0f;   // negative → auto
    int   movesPerFrame  = 2;
    bool  wireframe      = false;
    int   windowWidth    = 1280;
    int   windowHeight   = 720;

    // ── Toolpath colours (RGB components, range 0.0 … 1.0) ───────────────
    // Feed moves (G01/G02/G03) — default cyan
    float toolpathFeedR  = 0.00f;
    float toolpathFeedG  = 0.90f;
    float toolpathFeedB  = 0.90f;

    // Rapid moves (G00) — default yellow
    float toolpathRapidR = 1.00f;
    float toolpathRapidG = 0.90f;
    float toolpathRapidB = 0.10f;

    // Line width in screen pixels (clamped to ≥ 0.5)
    float toolpathLineWidth = 1.5f;

    bool load(const std::string& fileName);
};

#endif // CONFIG_H
