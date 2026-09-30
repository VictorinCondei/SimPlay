#include "Renderer.h"

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#  include <GLUT/glut.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#  include <GL/glut.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{

const float kPi = 3.14159265358979323846f;

void drawHemisphere(float radius, int slices, int stacks)
{
    for (int i = 0; i < stacks; ++i)
    {
        const float v0 = static_cast<float>(i) / static_cast<float>(stacks);
        const float v1 = static_cast<float>(i + 1) / static_cast<float>(stacks);

        const float phi0 = 0.5f * kPi * v0;
        const float phi1 = 0.5f * kPi * v1;

        glBegin(GL_QUAD_STRIP);

        for (int j = 0; j <= slices; ++j)
        {
            const float u = static_cast<float>(j) / static_cast<float>(slices);
            const float theta = 2.0f * kPi * u;

            const float c0 = std::cos(phi0);
            const float s0 = std::sin(phi0);
            const float c1 = std::cos(phi1);
            const float s1 = std::sin(phi1);

            const float ct = std::cos(theta);
            const float st = std::sin(theta);

            const float x0 = radius * c0 * ct;
            const float y0 = radius * c0 * st;
            const float z0 = -radius * s0;

            const float x1 = radius * c1 * ct;
            const float y1 = radius * c1 * st;
            const float z1 = -radius * s1;

            glNormal3f(x0 / radius, y0 / radius, z0 / radius);
            glVertex3f(x0, y0, z0);

            glNormal3f(x1 / radius, y1 / radius, z1 / radius);
            glVertex3f(x1, y1, z1);
        }

        glEnd();
    }
}

void drawHemisphereCap(float radius, int slices)
{
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(0.0f, 0.0f, -radius);

    for (int j = 0; j <= slices; ++j)
    {
        const float u = static_cast<float>(j) / static_cast<float>(slices);
        const float theta = 2.0f * kPi * u;

        const float x = radius * std::cos(theta);
        const float y = radius * std::sin(theta);
        const float z = 0.0f;

        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f(x, y, z);
    }

    glEnd();
}

} // namespace

Renderer::Renderer()
    : m_width(1280),
      m_height(720),
      m_distance(140.0f),
      m_panX(0.0f),
      m_panY(0.0f),
      m_centerX(0.0f),
      m_centerY(0.0f),
      m_centerZ(0.0f),
	  m_yaw(0.0f),
	  m_pitch(0.0f)
{
    identityRotation();

    rotate(-35.0f, 0.0f);
    rotate(0.0f, 28.0f);
}

void Renderer::identityRotation()
{
    m_rot[0]  = 1.0f; m_rot[1]  = 0.0f; m_rot[2]  = 0.0f; m_rot[3]  = 0.0f;
    m_rot[4]  = 0.0f; m_rot[5]  = 1.0f; m_rot[6]  = 0.0f; m_rot[7]  = 0.0f;
    m_rot[8]  = 0.0f; m_rot[9]  = 0.0f; m_rot[10] = 1.0f; m_rot[11] = 0.0f;
    m_rot[12] = 0.0f; m_rot[13] = 0.0f; m_rot[14] = 0.0f; m_rot[15] = 1.0f;
}



void Renderer::setCenter(float x, float y, float z)
{
    m_centerX = x;
    m_centerY = y;
    m_centerZ = z;
}

void Renderer::initialiseGL() const
{
    const GLfloat lightPosition[] = { 40.0f, 60.0f, 80.0f, 1.0f };
    const GLfloat ambient[]       = { 0.18f, 0.18f, 0.18f, 1.0f };
    const GLfloat diffuse[]       = { 0.92f, 0.92f, 0.92f, 1.0f };

    glClearColor(0.08f, 0.10f, 0.13f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Safer while mesh winding is under active iteration.
    glDisable(GL_CULL_FACE);

    glEnable(GL_NORMALIZE);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    glShadeModel(GL_SMOOTH);
}

void Renderer::resize(int width, int height)
{
    m_width  = std::max(1, width);
    m_height = std::max(1, height);

    glViewport(0, 0, m_width, m_height);
}

void Renderer::beginFrame() const
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        45.0,
        static_cast<double>(m_width) / m_height,
        0.1,
        5000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(
        m_panX,
        m_panY,
        -m_distance);

    glMultMatrixf(m_rot);

    glTranslatef(
        -m_centerX,
        -m_centerY,
        -m_centerZ);
}

void Renderer::drawMesh(const TriangleMesh& mesh) const
{
    if (mesh.vertices.empty() || mesh.indices.empty() || mesh.normals.empty())
        return;

    const GLfloat materialDiffuse[]   = { 0.70f, 0.73f, 0.76f, 1.0f };
    const GLfloat materialSpecular[]  = { 0.22f, 0.25f, 0.30f, 1.0f };
    const GLfloat materialShininess[] = { 22.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   materialDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  materialSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, materialShininess);

    glBegin(GL_TRIANGLES);

    for (std::size_t i = 0; i < mesh.indices.size(); ++i)
    {
        const unsigned int index = mesh.indices[i];

        if (index >= mesh.vertices.size() || index >= mesh.normals.size())
            continue;

        const Vec3& n = mesh.normals[index];
        const Vec3& v = mesh.vertices[index];

        glNormal3f(n.x, n.y, n.z);
        glVertex3f(v.x, v.y, v.z);
    }

    glEnd();
}

void Renderer::drawCutter(const Cutter& cutter) const
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

    glDisable(GL_LIGHTING);
    glColor3f(0.95f, 0.35f, 0.10f);

    glPushMatrix();
    glTranslatef(cutter.x, cutter.y, cutter.z);

    GLUquadric* quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH);

    if (cutter.type == CutterType::FLAT_END)
    {
        glPushMatrix();
        glRotatef(180.0f, 1.0f, 0.0f, 0.0f);

        gluCylinder(
            quadric,
            cutter.radius,
            cutter.radius,
            cutter.length,
            20,
            1);

        gluDisk(
            quadric,
            0.0,
            cutter.radius,
            20,
            1);

        glPopMatrix();
    }
    else
    {
        const float shankLength = std::max(0.0f, cutter.length - cutter.radius);

        if (shankLength > 0.0f)
        {
            glPushMatrix();
            glTranslatef(0.0f, 0.0f, -cutter.radius);
            glRotatef(180.0f, 1.0f, 0.0f, 0.0f);

            gluCylinder(
                quadric,
                cutter.radius,
                cutter.radius,
                shankLength,
                20,
                1);

            glPopMatrix();
        }

        glPushMatrix();
        glTranslatef(0.0f, 0.0f, -cutter.radius);
        drawHemisphere(cutter.radius, 24, 12);
        drawHemisphereCap(cutter.radius, 24);
        glPopMatrix();
    }

    gluDeleteQuadric(quadric);

    glPopMatrix();
    glPopAttrib();
}

void Renderer::drawAxes(float length) const
{
    glPushAttrib(GL_LIGHTING_BIT | GL_LINE_BIT | GL_CURRENT_BIT);

    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);

    glBegin(GL_LINES);

    glColor3f(0.92f, 0.20f, 0.20f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(length, 0.0f, 0.0f);

    glColor3f(0.20f, 0.92f, 0.20f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, length, 0.0f);

    glColor3f(0.25f, 0.45f, 1.00f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, length);

    glEnd();

    glPopAttrib();
}

void Renderer::drawToolPath(const std::vector<TrailPt>& trail,
                            Color3 feedColor,
                            Color3 rapidColor,
                            float lineWidth) const
{
    if (trail.size() < 2)
        return;

    glPushAttrib(GL_LIGHTING_BIT |
                 GL_LINE_BIT |
                 GL_CURRENT_BIT |
                 GL_ENABLE_BIT |
                 GL_DEPTH_BUFFER_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glLineWidth(std::max(0.5f, lineWidth));

    // Slight depth bias effect for lines so they don't disappear into the mesh.
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);

    glBegin(GL_LINES);

    for (std::size_t i = 1; i < trail.size(); ++i)
    {
        const TrailPt& a = trail[i - 1];
        const TrailPt& b = trail[i];

        const bool isRapid = b.isRapid;
        const Color3 color = isRapid ? rapidColor : feedColor;

        glColor3f(color.r, color.g, color.b);
        glVertex3f(a.x, a.y, a.z);
        glVertex3f(b.x, b.y, b.z);
    }

    glEnd();

    // Draw path points on top for clearer reading of sparse or stepped motion.
    glPointSize(std::max(3.0f, lineWidth + 1.0f));
    glBegin(GL_POINTS);

    for (std::size_t i = 0; i < trail.size(); ++i)
    {
        const bool isRapid = (i == 0) ? false : trail[i].isRapid;
        const Color3 color = isRapid ? rapidColor : feedColor;

        glColor3f(color.r, color.g, color.b);
        glVertex3f(trail[i].x, trail[i].y, trail[i].z);
    }

    glEnd();

    glPopAttrib();
}

void Renderer::endFrame() const
{
    glutSwapBuffers();
}

void Renderer::rotate(float deltaYaw, float deltaPitch)
{
    m_yaw   += deltaYaw;
    m_pitch += deltaPitch;

    if (m_pitch > 89.0f)  m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    rebuildRotationMatrix();
}

void Renderer::rebuildRotationMatrix()
{
    const float degToRad = kPi / 180.0f;

    const float yawRad   = m_yaw   * degToRad;
    const float pitchRad = m_pitch * degToRad;

    const float cy = std::cos(yawRad);
    const float sy = std::sin(yawRad);
    const float cx = std::cos(pitchRad);
    const float sx = std::sin(pitchRad);

    // Rotation = yaw around world Y, then pitch around local X.
    // Column-major layout for OpenGL.
    m_rot[0]  =  cy;
    m_rot[1]  =  sy * sx;
    m_rot[2]  =  sy * cx;
    m_rot[3]  =  0.0f;

    m_rot[4]  =  0.0f;
    m_rot[5]  =  cx;
    m_rot[6]  = -sx;
    m_rot[7]  =  0.0f;

    m_rot[8]  = -sy;
    m_rot[9]  =  cy * sx;
    m_rot[10] =  cy * cx;
    m_rot[11] =  0.0f;

    m_rot[12] =  0.0f;
    m_rot[13] =  0.0f;
    m_rot[14] =  0.0f;
    m_rot[15] =  1.0f;
}


void Renderer::zoom(float amount)
{
    m_distance = std::max(1.0f, m_distance + amount);
}

void Renderer::pan(float dx, float dy)
{
    m_panX += dx;
    m_panY += dy;
}
