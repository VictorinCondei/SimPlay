#include "Config.h"
#include "Renderer.h"
#include "Simulator.h"
#include "ToolPath.h"
#include "ToolPathLoader.h"

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <GLUT/glut.h>
#else
#  include <GL/gl.h>
#  include <GL/glut.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{

Config    g_config;
Renderer  g_renderer;
Simulator g_simulator;

// Animation state
bool g_running = false;
bool g_wireframe = false;

// Mouse state
bool g_leftDown = false;
bool g_rightDown = false;
bool g_middleDown = false;

bool g_showToolPath = true;
bool g_showMesh = true;

int g_lastMouseX = 0;
int g_lastMouseY = 0;

// Scene helpers
int   g_movesPerFrame = 2;
float g_axisLength = 10.0f;

// Convert simulator trail points to renderer trail points.
// This keeps the renderer independent from simulator internals.
std::vector<TrailPt> buildRenderTrail()
{
    const std::vector<TrailPoint>& src = g_simulator.trail();

    std::vector<TrailPt> out;
    out.reserve(src.size());

    for (std::size_t i = 0; i < src.size(); ++i) {
        TrailPt p;
        p.x = src[i].x;
        p.y = src[i].y;
        p.z = src[i].z;
        p.isRapid = src[i].rapid;
        out.push_back(p);
    }

    return out;
}

void updateWindowTitle()
{
    std::string title = "SimPlay - ";
    title += std::to_string(g_simulator.currentMove());
    title += "/";
    title += std::to_string(g_simulator.moveCount());
    title += " moves, ";
    title += std::to_string(g_simulator.mesh().triangleCount());
    title += " triangles";

    if (g_running)
        title += " [running]";
    else if (g_simulator.finished())
        title += " [done]";
    else
        title += " [paused]";

    glutSetWindowTitle(title.c_str());
}

void display()
{
    g_renderer.beginFrame();

    glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);

    if (g_showMesh)
        g_renderer.drawMesh(g_simulator.mesh());

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (g_showToolPath) {
        const std::vector<TrailPt> trail = buildRenderTrail();

        g_renderer.drawToolPath(
            trail,
            Color3(g_config.toolpathFeedR,
                   g_config.toolpathFeedG,
                   g_config.toolpathFeedB),
            Color3(g_config.toolpathRapidR,
                   g_config.toolpathRapidG,
                   g_config.toolpathRapidB),
            g_config.toolpathLineWidth);
    }

    g_renderer.drawCutter(g_simulator.cutter());
    g_renderer.drawAxes(g_axisLength);

    g_renderer.endFrame();
}

void reshape(int width, int height)
{
    g_renderer.resize(width, height);
}

void idle()
{
    if (!g_running)
        return;

    bool changed = false;
    std::size_t startMove = g_simulator.currentMove();

    for (int i = 0; i < g_movesPerFrame; ++i) {
        if (!g_simulator.step()) {
            g_running = false;
            break;
        }
        changed = true;
    }

    if (changed) {
        std::size_t endMove = g_simulator.currentMove();
        std::size_t moved = endMove - startMove;

        // Rebuild only from time to time while animating.
        // This keeps playback responsive.
        static std::size_t sinceLastMesh = 0;
        sinceLastMesh += moved;

        if (!g_running || sinceLastMesh >= 50) {
            g_simulator.rebuildMesh();
            sinceLastMesh = 0;
        }
    }

    updateWindowTitle();
    glutPostRedisplay();
}

void keyboard(unsigned char key, int, int)
{
    switch (key) {
        case 27:
        case 'q':
        case 'Q':
            std::exit(0);
            break;

        case 'p':
        case 'P':
        case ' ':
            if (!g_simulator.finished())
                g_running = !g_running;
            updateWindowTitle();
            break;

        case 'r':
        case 'R':
            g_running = false;
            g_simulator.reset();
            updateWindowTitle();
            glutPostRedisplay();
            break;

        case 't':
        case 'T':
            g_showToolPath = !g_showToolPath;
            glutPostRedisplay();
            break;

        case 'm':
        case 'M':
            g_showMesh = !g_showMesh;
            glutPostRedisplay();
            break;

        case 'f':
        case 'F':
            g_running = false;
            g_simulator.runToEnd();
            updateWindowTitle();
            glutPostRedisplay();
            break;

        case 'w':
        case 'W':
            g_wireframe = !g_wireframe;
            glutPostRedisplay();
            break;

        case '+':
        case '=':
            g_renderer.zoom(-5.0f);
            glutPostRedisplay();
            break;

        case '-':
        case '_':
            g_renderer.zoom(5.0f);
            glutPostRedisplay();
            break;

        default:
            break;
    }
}

void mouseButton(int button, int state, int x, int y)
{
    g_lastMouseX = x;
    g_lastMouseY = y;

    if (button == GLUT_LEFT_BUTTON)
        g_leftDown = (state == GLUT_DOWN);

    if (button == GLUT_RIGHT_BUTTON)
        g_rightDown = (state == GLUT_DOWN);

    if (button == GLUT_MIDDLE_BUTTON)
        g_middleDown = (state == GLUT_DOWN);

    // GLUT mouse wheel
    if (state == GLUT_DOWN && button == 3)
        g_renderer.zoom(-5.0f);

    if (state == GLUT_DOWN && button == 4)
        g_renderer.zoom(5.0f);

    glutPostRedisplay();
}

void mouseMotion(int x, int y)
{
    const float dx = static_cast<float>(x - g_lastMouseX);
    const float dy = static_cast<float>(y - g_lastMouseY);

    g_lastMouseX = x;
    g_lastMouseY = y;

    if (g_leftDown)
        g_renderer.pan(dx * 0.35f, -dy * 0.35f);

    const int mods = glutGetModifiers();
    float factor = (mods & GLUT_ACTIVE_SHIFT) ? 0.25f : 1.0f;

    if (g_rightDown)
    {
        const float dxNorm = dx / std::max(1, g_config.windowWidth);
        const float dyNorm = dy / std::max(1, g_config.windowHeight);

        const int mods = glutGetModifiers();
        const float fine = (mods & GLUT_ACTIVE_SHIFT) ? 0.25f : 1.0f;

        const float yawSpeed   = 180.0f;
        const float pitchSpeed = 140.0f;

        g_renderer.rotate(dxNorm * yawSpeed * fine,
                          dyNorm * pitchSpeed * fine);
    }



    if (g_middleDown)
        g_renderer.zoom(dy * 0.5f);

    glutPostRedisplay();
}

CutterType chooseCutterType()
{
    // Keep FLAT_END as the default unless Config later exposes a tool type.
    return CutterType::FLAT_END;
}

bool loadProgram(const std::string& isoFile, int resolution, float diameter)
{
    ToolPath path;
    if (!ToolPathLoader::load(isoFile, path, g_config.maxStep))
        return false;

    if (path.moves.empty()) {
        std::cerr << "No G00/G01/G02/G03 positional moves were found in "
                  << isoFile << std::endl;
        return false;
    }

    const float radius = std::max(0.05f, diameter * 0.5f);

    float margin;
    if (g_config.stockMargin > 0.0f)
        margin = g_config.stockMargin;
    else
        margin = std::max(2.0f * diameter, 5.0f);

    float length;
    if (g_config.toolLength > 0.0f)
        length = g_config.toolLength;
    else
        length = std::max(4.0f * diameter, 10.0f);

    g_simulator.initialise(path,
                           margin,
                           resolution,
                           chooseCutterType(),
                           radius,
                           length);

    const VoxelGrid& grid = g_simulator.grid();

    g_renderer.setCenter(
        g_simulator.centerX(),
        g_simulator.centerY(),
        g_simulator.centerZ());

    const float maxExtent = std::max(
        grid.nx * grid.cell,
        std::max(grid.ny * grid.cell, grid.nz * grid.cell));

    g_axisLength = maxExtent * 0.2f;
    g_renderer.zoom(maxExtent * 1.5f - 140.0f);

    std::cout << "Loaded " << path.size() << " path moves. "
              << "Initial mesh has " << g_simulator.mesh().triangleCount()
              << " boundary triangles." << std::endl;

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " path/to/program.iso" << std::endl;
        return 1;
    }

    g_config.load("simplay.conf");

    const std::string isoFile = argv[1];
    const int resolution = std::max(16, g_config.resolution);
    const float toolDiameter = g_config.toolDiameter;

    if (!loadProgram(isoFile, resolution, toolDiameter))
        return 1;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(g_config.windowWidth, g_config.windowHeight);
    glutInitWindowPosition(100, 80);
    glutCreateWindow("SimPlay");

    g_renderer.initialiseGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutIdleFunc(idle);

    updateWindowTitle();
    glutMainLoop();
    return 0;
}
