#include <glm/gtc/matrix_transform.hpp>
#include <QOpenGLWidget>
#include <QOpenGLFramebufferObject>
#include <QEvent>
#include <QMouseEvent>
#include <chrono>
#include "settings/settings.h"
#include "mesh/mesh.h"

#include "rendererclassic.h"

//todo ClearModel?

void Renderer3D::SetModel(CMesh *mdl)
{
    m_model = mdl;

    for(auto it=m_model->textures.begin(); it!=m_model->textures.end(); it++)
    {
        if(it->second != nullptr)
        {
            LoadTexture(it->second.get(), it->first);
        }
    }
}

void Renderer3D::Init()
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glShadeModel(GL_FLAT);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);
    glShadeModel(GL_SMOOTH);
    glm::vec4 diff = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 ambi = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, &diff[0]);
    glLightfv(GL_LIGHT1, GL_AMBIENT, &ambi[0]);
}

void Renderer3D::ResizeView(int w, int h)
{
    m_pickTexValid = false;
    m_width = w;
    m_height = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glm::mat4 projMx = glm::perspective(glm::radians(m_fovy), (static_cast<float>(w))/(static_cast<float>(h)), 0.1f, 3000.0f);
    glMultMatrixf(&projMx[0][0]);
}

void Renderer3D::ToggleLighting(bool enable)
{
    m_lighting = enable;
    if(m_lighting)
    {
        glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
    }
}

void Renderer3D::DrawAxis(glm::mat4 rotMx)
{
    glDisable(GL_LIGHTING);
    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-2.0f, 2.0f, -1.5f, 1.5f, 0.1f, 100.0f);

    glm::vec4 vecX(0.1f, 0.0f, 0.0f, 1.0f);
    glm::vec4 vecY(0.0f, 0.1f, 0.0f, 1.0f);
    glm::vec4 vecZ(0.0f, 0.0f, 0.1f, 1.0f);
    vecX = rotMx * vecX;
    vecY = rotMx * vecY;
    vecZ = rotMx * vecZ;

    glTranslatef(-1.9f, -1.4f, -20.0f);

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

void Renderer3D::DrawGrid(glm::vec3 cameraPosition)
{
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
        glVertex3f(cameraPosition.x + 50.0f, 0.0f, 0.0f);
        glVertex3f(cameraPosition.x - 50.0f, 0.0f, 0.0f);

        glVertex3f(0.0f, 0.0f, cameraPosition.z + 50.0f);
        glVertex3f(0.0f, 0.0f, cameraPosition.z - 50.0f);
    glEnd();

    int baseX = (int)cameraPosition.x;
    int baseZ = (int)cameraPosition.z;

    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_LINES);
        for(int i=-50; i<50; ++i)
        {
            glVertex3f(baseX+i, 0.0f, cameraPosition.z + 50.0f);
            glVertex3f(baseX+i, 0.0f, cameraPosition.z - 50.0f);
        }
        for(int i=-50; i<50; ++i)
        {
            glVertex3f(cameraPosition.x + 50.0f, 0.0f, baseZ+i);
            glVertex3f(cameraPosition.x - 50.0f, 0.0f, baseZ+i);
        }
    glEnd();
}

void Renderer3D::DrawBackground()
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

void Renderer3D::DrawModel(std::unordered_set<int> pickTriIndices)
{
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixf(&m_viewMatrix[0][0]);

    glLightfv(GL_LIGHT1, GL_POSITION, &m_lightPosition[0]);

    const std::vector<glm::vec3> &vert = m_model->GetVertices();
    const std::vector<glm::vec2> &uvs = m_model->GetUVCoords();
    const std::vector<glm::vec3> &norms = m_model->GetNormals();

    glBegin(GL_TRIANGLES);
    int i = 0;
    glColor3ub(255, 255, 255);
    for(const glm::uvec4 &t : m_model->GetTriangles())
    {
        bool faceSelected = pickTriIndices.find(i) != pickTriIndices.end();

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
            glTexCoord2f(0.0f, 0.0f);
        else
            glTexCoord2f(uv1[0], uv1[1]);
        glVertex3f(vertex1[0], vertex1[1], vertex1[2]);

        if(!faceSelected)
            glTexCoord2f(uv2[0], uv2[1]);
        glVertex3f(vertex2[0], vertex2[1], vertex2[2]);

        if(!faceSelected)
            glTexCoord2f(uv3[0], uv3[1]);
        glVertex3f(vertex3[0], vertex3[1], vertex3[2]);
    }

    glEnd();

    UnbindTexture();
}

void Renderer3D::UpdateViewMatrix(glm::mat4 viewMatrix)
{
    m_pickTexValid = false;
    m_viewMatrix = viewMatrix;
}

void Renderer3D::SetLightPosition(glm::vec3 position)
{
    m_lightPosition = {position.x, position.y, position.z, 1.0f};
}

QImage* Renderer3D::RefreshPickingTexture()
{
    if(m_pickTexValid)
        return &m_pickingTexture;

    m_pickTexValid = true;

    QOpenGLFramebufferObject fbo(m_width, m_height, QOpenGLFramebufferObject::Depth, GL_TEXTURE_2D);
    if(!fbo.isValid())
    {
        return nullptr; //too bad, can't do any picking
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

    m_pickingTexture = fbo.toImage();

    fbo.release();
    return &m_pickingTexture;
}

void Renderer3D::LoadTexture(QImage *img, unsigned index)
{
    if(!img)
    {
        m_textures[index].reset(nullptr);
    } else {
        if(m_textures[index])
            m_textures[index]->destroy();
        m_textures[index].reset(new QOpenGLTexture(*img));
        m_textures[index]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_textures[index]->setMagnificationFilter(QOpenGLTexture::Linear);
        m_textures[index]->setWrapMode(QOpenGLTexture::Repeat);
    }
}

void Renderer3D::BindTexture(unsigned id)
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

void Renderer3D::UnbindTexture()
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
