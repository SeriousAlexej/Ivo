#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <QOpenGLWidget>
#include <QOpenGLFramebufferObject>
#include "settings/settings.h"
#include "mesh/mesh.h"
#include "renderlegacy3d.h"

CRenderer3DLegacy::CRenderer3DLegacy()
{
}

CRenderer3DLegacy::~CRenderer3DLegacy()
{
}

void CRenderer3DLegacy::Init()
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);
    glShadeModel(GL_SMOOTH);
    glm::vec4 diff = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 ambi = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, &diff[0]);
    glLightfv(GL_LIGHT1, GL_AMBIENT, &ambi[0]);
}

void CRenderer3DLegacy::ResizeView(int w, int h, float fovy)
{
    m_width = w;
    m_height = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glm::mat4 projMx = glm::perspective(glm::radians(fovy), (static_cast<float>(w))/(static_cast<float>(h)), 0.1f, 3000.0f);
    glMultMatrixf(&projMx[0][0]);
}

void CRenderer3DLegacy::ToggleLighting(bool enable)
{
    IRenderer3D::ToggleLighting(enable);
    if(m_lighting)
    {
        glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
    }
}

void CRenderer3DLegacy::ToggleGrid(bool enable)
{
    IRenderer3D::ToggleGrid(enable);
}

void CRenderer3DLegacy::DrawScene() const
{
    DrawBackground();
    DrawModel();
    DrawGrid();
    DrawAxis();
}

void CRenderer3DLegacy::DrawModel() const
{
    if(!m_model)
        return;

    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixf(&m_viewMatrix[0][0]);

    const glm::vec4 lightPosition = {m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z, 1.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, &lightPosition[0]);

    const std::vector<glm::vec3> &vert = m_model->GetVertices();
    const std::vector<glm::vec2> &uvs = m_model->GetUVCoords();
    const std::vector<glm::vec3> &norms = m_model->GetNormals();

    glBegin(GL_TRIANGLES);
    int i = 0;
    glColor3ub(255, 255, 255);
    for(const glm::uvec4 &t : m_model->GetTriangles())
    {
        bool faceSelected = m_model->IsTrianglePicked(i);

        BindTexture(t[3]);

        const glm::vec3 &vertex1 = vert[t[0]];
        const glm::vec3 &vertex2 = vert[t[1]];
        const glm::vec3 &vertex3 = vert[t[2]];

        const glm::vec2 &uv1 = uvs[t[0]];
        const glm::vec2 &uv2 = uvs[t[1]];
        const glm::vec2 &uv3 = uvs[t[2]];

        const glm::vec3 &faceNormal = norms[i++];

        glNormal3f(faceNormal[0], faceNormal[1], faceNormal[2]);

        if(faceSelected)
            glColor3ub(255, 0, 0);
        else
            glColor3ub(255, 255, 255);

        glTexCoord2f(uv1[0], uv1[1]);
        glVertex3f(vertex1[0], vertex1[1], vertex1[2]);

        glTexCoord2f(uv2[0], uv2[1]);
        glVertex3f(vertex2[0], vertex2[1], vertex2[2]);

        glTexCoord2f(uv3[0], uv3[1]);
        glVertex3f(vertex3[0], vertex3[1], vertex3[2]);
    }

    glEnd();

    UnbindTexture();
}

void CRenderer3DLegacy::DrawBackground() const
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if(m_lighting)
        glDisable(GL_LIGHTING);

    glBegin(GL_QUADS);
    glColor3ub(214, 237, 255);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glColor3ub(255, 255, 255);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    if(m_lighting)
        glEnable(GL_LIGHTING);
}

void CRenderer3DLegacy::DrawGrid() const
{
    if(!m_grid)
        return;

    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
        glVertex3f(m_cameraPosition.x + 50.0f, 0.0f, 0.0f);
        glVertex3f(m_cameraPosition.x - 50.0f, 0.0f, 0.0f);

        glVertex3f(0.0f, 0.0f, m_cameraPosition.z + 50.0f);
        glVertex3f(0.0f, 0.0f, m_cameraPosition.z - 50.0f);
    glEnd();

    int baseX = (int)m_cameraPosition.x;
    int baseZ = (int)m_cameraPosition.z;

    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_LINES);
        for(int i=-50; i<50; ++i)
        {
            glVertex3f(baseX+i, 0.0f, m_cameraPosition.z + 50.0f);
            glVertex3f(baseX+i, 0.0f, m_cameraPosition.z - 50.0f);
        }
        for(int i=-50; i<50; ++i)
        {
            glVertex3f(m_cameraPosition.x + 50.0f, 0.0f, baseZ+i);
            glVertex3f(m_cameraPosition.x - 50.0f, 0.0f, baseZ+i);
        }
    glEnd();
}

void CRenderer3DLegacy::DrawAxis() const
{
    glDisable(GL_LIGHTING);
    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);

    glm::mat4 rotMx = m_viewMatrix;
    rotMx[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    glm::vec4 vecX(0.1f, 0.0f, 0.0f, 1.0f);
    glm::vec4 vecY(0.0f, 0.1f, 0.0f, 1.0f);
    glm::vec4 vecZ(0.0f, 0.0f, 0.1f, 1.0f);
    vecX = rotMx * vecX;
    vecY = rotMx * vecY;
    vecZ = rotMx * vecZ;

    glTranslatef(-1.9f, -1.9f, -20.0f);

    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(vecX.x, vecX.y, vecX.z);
    glEnd();
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(vecY.x, vecY.y, vecY.z);
    glEnd();
    glColor3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(vecZ.x, vecZ.y, vecZ.z);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    if(m_lighting)
    {
        glEnable(GL_LIGHTING);
    }
    glColor3f(1.0f, 1.0f, 1.0f);
}

void CRenderer3DLegacy::UpdateViewMatrix(const glm::mat4& viewMatrix)
{
    const glm::mat3 rotMx(viewMatrix);
    const glm::vec3 d(viewMatrix[3]);

    m_viewMatrix = viewMatrix;
    m_cameraPosition = -d * rotMx;
}

QImage CRenderer3DLegacy::GetPickingTexture() const
{
    if(!m_model)
    {
        throw std::logic_error("Can not pick without model");
    }

    QOpenGLFramebufferObject fbo(m_width, m_height, QOpenGLFramebufferObject::Depth, GL_TEXTURE_2D);
    if(!fbo.isValid())
    {
        throw std::logic_error("Could not create framebuffer object");
    }
    fbo.bind();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixf(&m_viewMatrix[0][0]);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_TRIANGLES);

    const std::vector<glm::vec3> &vert = m_model->GetVertices();

    int i = 0;
    for(const glm::uvec4 &t : m_model->GetTriangles())
    {
        const glm::vec3 &vertex1 = vert[t[0]];
        const glm::vec3 &vertex2 = vert[t[1]];
        const glm::vec3 &vertex3 = vert[t[2]];

        int r = i & 0x0000FF;
        int g = i & 0x00FF00; g >>= 8;
        int b = i & 0xFF0000; b >>= 16;

        glColor3ub(r, g, b);
        glVertex3f(vertex1[0], vertex1[1], vertex1[2]);
        glVertex3f(vertex2[0], vertex2[1], vertex2[2]);
        glVertex3f(vertex3[0], vertex3[1], vertex3[2]);
        i++;
    }

    glEnd();

    if(m_lighting)
        glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

    QImage pickTexture = fbo.toImage();

    assert(fbo.release());

    return pickTexture;
}

void CRenderer3DLegacy::BindTexture(unsigned id) const
{
    const bool renTexture = CSettings::GetInstance().GetRenderFlags() & CSettings::R_TEXTR;
    if(renTexture && m_boundTextureID != (int)id)
    {
        glEnd();
        if(m_textures[id])
        {
            m_textures[id]->bind();
        } else if(m_boundTextureID >= 0 && m_textures[m_boundTextureID])
        {
            m_textures[m_boundTextureID]->release();
        }
        glBegin(GL_TRIANGLES);
        m_boundTextureID = id;
    }
}

void CRenderer3DLegacy::UnbindTexture() const
{
    const bool renTexture = CSettings::GetInstance().GetRenderFlags() & CSettings::R_TEXTR;
    if(renTexture)
    {
        for(auto it=m_textures.begin(); it!=m_textures.end(); it++)
        {
            if(m_textures[it->first] && m_textures[it->first]->isBound())
            {
                m_textures[it->first]->release();
                break;
            }
        }
        m_boundTextureID = -1;
    }
}

void CRenderer3DLegacy::ClearTextures()
{
    m_boundTextureID = -1;
    IRenderer3D::ClearTextures();
}
