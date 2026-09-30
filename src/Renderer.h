#ifndef SRC_RENDERER_H_
#define SRC_RENDERER_H_

#include "Cutter.h"
#include "MeshBuilder.h"

#include <vector>

struct Color3
{
    float r, g, b;

    Color3() : r(1.0f), g(1.0f), b(1.0f) {}
    Color3(float rr, float gg, float bb) : r(rr), g(gg), b(bb) {}
};

struct TrailPt
{
    float x, y, z;
    bool isRapid;
};

class Renderer
{
public:
    Renderer();

    void initialiseGL() const;
    void resize(int width, int height);

    void beginFrame() const;
    void endFrame() const;

    void setCenter(float x, float y, float z);

    void drawMesh(const TriangleMesh& mesh) const;
    void drawCutter(const Cutter& cutter) const;
    void drawAxes(float length) const;

    void drawToolPath(const std::vector<TrailPt>& trail,
                      Color3 feedColor = Color3(1.00f, 0.82f, 0.18f),
                      Color3 rapidColor = Color3(0.35f, 0.85f, 1.00f),
                      float lineWidth = 2.0f) const;

    void rotate(float deltaYaw, float deltaPitch);
    void zoom(float amount);
    void pan(float dx, float dy);

private:
    void identityRotation();
    void rebuildRotationMatrix();

private:
    int   m_width;
    int   m_height;
    float m_distance;
    float m_panX;
    float m_panY;
    float m_centerX;
    float m_centerY;
    float m_centerZ;

    float m_yaw;
    float m_pitch;

    float m_rot[16];
};


#endif /* SRC_RENDERER_H_ */
