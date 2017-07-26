#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <QOpenGLFramebufferObject>
#include "settings/settings.h"
#include "mesh/mesh.h"
#include "renderlegacy3d.h"

CRenderer3DLegacy::CRenderer3DLegacy(QOpenGLFunctions_2_0& gl) :
    m_gl(gl)
{
}

CRenderer3DLegacy::~CRenderer3DLegacy()
{
}

void CRenderer3DLegacy::Init()
{
    m_gl.glEnable(GL_TEXTURE_2D);
    m_gl.glEnable(GL_DEPTH_TEST);
    m_gl.glDepthFunc(GL_LESS);
    m_gl.glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    m_gl.glEnable(GL_MULTISAMPLE);
    m_gl.glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    m_gl.glEnable(GL_COLOR_MATERIAL);
    m_gl.glEnable(GL_LIGHTING);
    m_gl.glEnable(GL_LIGHT1);
    m_gl.glShadeModel(GL_SMOOTH);
    glm::vec4 diff = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 ambi = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    m_gl.glLightfv(GL_LIGHT1, GL_DIFFUSE, &diff[0]);
    m_gl.glLightfv(GL_LIGHT1, GL_AMBIENT, &ambi[0]);
}

void CRenderer3DLegacy::ResizeView(int w, int h, float fovy)
{
    m_width = w;
    m_height = h;
    m_gl.glViewport(0, 0, w, h);
    m_gl.glMatrixMode(GL_PROJECTION);
    m_gl.glLoadIdentity();
    glm::mat4 projMx = glm::perspective(glm::radians(fovy), (static_cast<float>(w))/(static_cast<float>(h)), 0.1f, 3000.0f);
    m_gl.glMultMatrixf(&projMx[0][0]);
}

void CRenderer3DLegacy::ToggleLighting(bool enable)
{
    IRenderer3D::ToggleLighting(enable);
    if(m_lighting)
    {
        m_gl.glEnable(GL_LIGHTING);
    } else {
        m_gl.glDisable(GL_LIGHTING);
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

    m_gl.glClear(GL_DEPTH_BUFFER_BIT);
    m_gl.glMatrixMode(GL_MODELVIEW);
    m_gl.glLoadIdentity();
    m_gl.glMultMatrixf(&m_viewMatrix[0][0]);

    const glm::vec4 lightPosition = {m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z, 1.0f};
    m_gl.glLightfv(GL_LIGHT1, GL_POSITION, &lightPosition[0]);

    const std::vector<glm::vec3> &vert = m_model->GetVertices();
    const std::vector<glm::vec2> &uvs = m_model->GetUVCoords();
    const std::vector<glm::vec3> &norms = m_model->GetNormals();

    m_gl.glBegin(GL_TRIANGLES);
    m_gl.glColor3ub(255, 255, 255);
    int i = 0;
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

        m_gl.glNormal3f(faceNormal[0], faceNormal[1], faceNormal[2]);

        if(faceSelected)
            m_gl.glColor3ub(255, 0, 0);
        else
            m_gl.glColor3ub(255, 255, 255);

        m_gl.glTexCoord2f(uv1[0], uv1[1]);
        m_gl.glVertex3f(vertex1[0], vertex1[1], vertex1[2]);

        m_gl.glTexCoord2f(uv2[0], uv2[1]);
        m_gl.glVertex3f(vertex2[0], vertex2[1], vertex2[2]);

        m_gl.glTexCoord2f(uv3[0], uv3[1]);
        m_gl.glVertex3f(vertex3[0], vertex3[1], vertex3[2]);
    }

    m_gl.glEnd();

    UnbindTexture();
}

void CRenderer3DLegacy::DrawBackground() const
{
    m_gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_gl.glMatrixMode(GL_PROJECTION);
    m_gl.glPushMatrix();
    m_gl.glLoadIdentity();
    m_gl.glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f);

    m_gl.glMatrixMode(GL_MODELVIEW);
    m_gl.glLoadIdentity();

    if(m_lighting)
        m_gl.glDisable(GL_LIGHTING);

    m_gl.glBegin(GL_QUADS);
    m_gl.glColor3ub(214, 237, 255);
    m_gl.glVertex3f(-1.0f, 1.0f, -1.0f);
    m_gl.glVertex3f(1.0f, 1.0f, -1.0f);
    m_gl.glColor3ub(255, 255, 255);
    m_gl.glVertex3f(1.0f, -1.0f, -1.0f);
    m_gl.glVertex3f(-1.0f, -1.0f, -1.0f);
    m_gl.glEnd();

    m_gl.glMatrixMode(GL_PROJECTION);
    m_gl.glPopMatrix();

    if(m_lighting)
        m_gl.glEnable(GL_LIGHTING);
}

void CRenderer3DLegacy::DrawGrid() const
{
    if(!m_grid)
        return;

    m_gl.glDisable(GL_LIGHTING);
    m_gl.glColor3f(0.0f, 0.0f, 0.0f);
    m_gl.glBegin(GL_LINES);
        m_gl.glVertex3f(m_cameraPosition.x + 50.0f, 0.0f, 0.0f);
        m_gl.glVertex3f(m_cameraPosition.x - 50.0f, 0.0f, 0.0f);

        m_gl.glVertex3f(0.0f, 0.0f, m_cameraPosition.z + 50.0f);
        m_gl.glVertex3f(0.0f, 0.0f, m_cameraPosition.z - 50.0f);
    m_gl.glEnd();

    int baseX = (int)m_cameraPosition.x;
    int baseZ = (int)m_cameraPosition.z;

    m_gl.glColor3f(0.4f, 0.4f, 0.4f);
    m_gl.glBegin(GL_LINES);
        for(int i=-50; i<50; ++i)
        {
            m_gl.glVertex3f(baseX+i, 0.0f, m_cameraPosition.z + 50.0f);
            m_gl.glVertex3f(baseX+i, 0.0f, m_cameraPosition.z - 50.0f);
        }
        for(int i=-50; i<50; ++i)
        {
            m_gl.glVertex3f(m_cameraPosition.x + 50.0f, 0.0f, baseZ+i);
            m_gl.glVertex3f(m_cameraPosition.x - 50.0f, 0.0f, baseZ+i);
        }
    m_gl.glEnd();
}

void CRenderer3DLegacy::DrawAxis() const
{
    m_gl.glDisable(GL_LIGHTING);
    m_gl.glClear(GL_DEPTH_BUFFER_BIT);

    m_gl.glMatrixMode(GL_MODELVIEW);
    m_gl.glLoadIdentity();

    m_gl.glMatrixMode(GL_PROJECTION);
    m_gl.glPushMatrix();
    m_gl.glLoadIdentity();
    m_gl.glOrtho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);

    glm::mat4 rotMx = m_viewMatrix;
    rotMx[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    glm::vec4 vecX(0.1f, 0.0f, 0.0f, 1.0f);
    glm::vec4 vecY(0.0f, 0.1f, 0.0f, 1.0f);
    glm::vec4 vecZ(0.0f, 0.0f, 0.1f, 1.0f);
    vecX = rotMx * vecX;
    vecY = rotMx * vecY;
    vecZ = rotMx * vecZ;

    m_gl.glTranslatef(-1.9f, -1.9f, -20.0f);

    m_gl.glColor3f(1.0f, 0.0f, 0.0f);
    m_gl.glBegin(GL_LINES);
        m_gl.glVertex3f(0.0f, 0.0f, 0.0f);
        m_gl.glVertex3f(vecX.x, vecX.y, vecX.z);
    m_gl.glEnd();
    m_gl.glColor3f(0.0f, 1.0f, 0.0f);
    m_gl.glBegin(GL_LINES);
        m_gl.glVertex3f(0.0f, 0.0f, 0.0f);
        m_gl.glVertex3f(vecY.x, vecY.y, vecY.z);
    m_gl.glEnd();
    m_gl.glColor3f(0.0f, 0.0f, 1.0f);
    m_gl.glBegin(GL_LINES);
        m_gl.glVertex3f(0.0f, 0.0f, 0.0f);
        m_gl.glVertex3f(vecZ.x, vecZ.y, vecZ.z);
    m_gl.glEnd();

    m_gl.glPopMatrix();
    m_gl.glMatrixMode(GL_MODELVIEW);

    if(m_lighting)
        m_gl.glEnable(GL_LIGHTING);

    m_gl.glColor3f(1.0f, 1.0f, 1.0f);
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

    m_gl.glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    m_gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_gl.glMatrixMode(GL_MODELVIEW);
    m_gl.glLoadIdentity();
    m_gl.glMultMatrixf(&m_viewMatrix[0][0]);

    m_gl.glDisable(GL_LIGHTING);
    m_gl.glDisable(GL_TEXTURE_2D);

    m_gl.glBegin(GL_TRIANGLES);

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

        m_gl.glColor3ub(r, g, b);
        m_gl.glVertex3f(vertex1[0], vertex1[1], vertex1[2]);
        m_gl.glVertex3f(vertex2[0], vertex2[1], vertex2[2]);
        m_gl.glVertex3f(vertex3[0], vertex3[1], vertex3[2]);
        i++;
    }

    m_gl.glEnd();

    if(m_lighting)
        m_gl.glEnable(GL_LIGHTING);
    m_gl.glEnable(GL_TEXTURE_2D);

    m_gl.glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

    QImage pickTexture = fbo.toImage();

    assert(fbo.release());

    return pickTexture;
}

void CRenderer3DLegacy::BindTexture(unsigned id) const
{
    const bool renTexture = CSettings::GetInstance().GetRenderFlags() & CSettings::R_TEXTR;
    if(renTexture && m_boundTextureID != (int)id)
    {
        m_gl.glEnd();
        if(m_textures[id])
        {
            m_textures[id]->bind();
        } else if(m_boundTextureID >= 0 && m_textures[m_boundTextureID])
        {
            m_textures[m_boundTextureID]->release();
        }
        m_gl.glBegin(GL_TRIANGLES);
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
