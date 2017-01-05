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

void Renderer3D::Init2D()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.7f, 0.7f, 0.7f, 0.7f);
    CreateFoldTextures();
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

void Renderer3D::CreateFoldTextures()
{
    QImage imgFolds(16, 4, QImage::Format_ARGB32); //row 0 - black, row 1 - valley, row 2 - mountain, row 3 - white
    imgFolds.fill(QColor(0,0,0,255));

    for(int i=0; i<6; ++i)
    {
        imgFolds.setPixelColor(i, 1, QColor(0,0,0,0));
        imgFolds.setPixelColor(i, 2, QColor(0,0,0,0));
    }
    for(int i=0; i<16; ++i)
    {
        imgFolds.setPixelColor(i, 3, QColor(255, 255, 255, 255));
    }
    imgFolds.setPixelColor(3, 2, QColor(0,0,0,255));

    m_texFolds.reset(new QOpenGLTexture(imgFolds));
    m_texFolds->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    m_texFolds->setWrapMode(QOpenGLTexture::Repeat);
}



void Renderer3D::DrawFlaps() const
{
    if(m_texFolds)
        m_texFolds->bind();

    glBegin(GL_QUADS);
    for(const CMesh::SEdge &e : m_model->GetEdges())
    {
        if(!e.IsSnapped())
        {
            switch(e.GetFlapPosition())
            {
            case CMesh::SEdge::FP_LEFT:
                RenderFlap(e.GetTriangle(0), e.GetTriIndex(0));
                break;
            case CMesh::SEdge::FP_RIGHT:
                RenderFlap(e.GetTriangle(1), e.GetTriIndex(1));
                break;
            case CMesh::SEdge::FP_BOTH:
                RenderFlap(e.GetTriangle(0), e.GetTriIndex(0));
                RenderFlap(e.GetTriangle(1), e.GetTriIndex(1));
                break;
            case CMesh::SEdge::FP_NONE:
            default:
                break;
            }
        }
    }
    glEnd();

    if(m_texFolds && m_texFolds->isBound())
        m_texFolds->release();
}


void Renderer3D::DrawEdges()
{
    const unsigned char renFlags = CSettings::GetInstance().GetRenderFlags();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    if(m_texFolds)
        m_texFolds->bind();

    glBegin(GL_QUADS);
    for(const CMesh::SEdge &e : m_model->GetEdges())
    {
        int foldType = (int)e.GetFoldType();
        if(foldType == CMesh::SEdge::FT_FLAT && e.IsSnapped())
            continue;

     if(e.HasTwoTriangles())
     {
         if(e.IsSnapped() && (renFlags & CSettings::R_FOLDS))
         {
             RenderEdge(e.GetTriangle(0), e.GetTriIndex(0), foldType);
         } else if(!e.IsSnapped() && (renFlags & CSettings::R_EDGES)) {
             RenderEdge(e.GetTriangle(0), e.GetTriIndex(0), CMesh::SEdge::FT_FLAT);
             RenderEdge(e.GetTriangle(1), e.GetTriIndex(1), CMesh::SEdge::FT_FLAT);
         }
     } else if(renFlags & CSettings::R_EDGES) {
         void *t = static_cast<void*>(e.GetAnyTriangle());
         int edge = e.GetAnyTriIndex();
         RenderEdge(t, edge, CMesh::SEdge::FT_FLAT);
     }
    }
    glEnd();
    glDisable(GL_BLEND);

    if(m_texFolds && m_texFolds->isBound())
        m_texFolds->release();
}

void Renderer3D::RenderFlap(void *tr, int edge) const
{
    const CMesh::STriangle2D& t = *static_cast<CMesh::STriangle2D*>(tr);
    const CMesh::STriGroup *g = t.GetGroup();
    const float dep = g->GetDepth() + CMesh::STriGroup::GetDepthStep()*0.3f;
    const float dep2 = dep + CMesh::STriGroup::GetDepthStep()*0.15f;
    const glm::vec2 &v1 = t[edge];
    const glm::vec2 &v2 = t[(edge+1)%3];
    const glm::vec2 vN = t.GetNormal(edge) * 0.5f;

    float x[4];
    float y[4];
    if(t.IsFlapSharp(edge))
    {
        x[0] = v1.x;
        y[0] = v1.y;
        x[1] = 0.5f*v1.x + 0.5f*v2.x + vN.x;
        y[1] = 0.5f*v1.y + 0.5f*v2.y + vN.y;
        x[2] = v2.x;
        y[2] = v2.y;
        x[3] = 0.5f*v1.x + 0.5f*v2.x;
        y[3] = 0.5f*v1.y + 0.5f*v2.y;
    } else {
        x[0] = v1.x;
        y[0] = v1.y;
        x[1] = 0.9f*v1.x + 0.1f*v2.x + vN.x;
        y[1] = 0.9f*v1.y + 0.1f*v2.y + vN.y;
        x[2] = 0.1f*v1.x + 0.9f*v2.x + vN.x;
        y[2] = 0.1f*v1.y + 0.9f*v2.y + vN.y;
        x[3] = v2.x;
        y[3] = v2.y;
    }

    static const glm::mat2 rotMx90deg = glm::mat2( 0, 1,
                                                  -1, 0);
    const float normalScaler = 0.015f * CSettings::GetInstance().GetLineWidth();

    //render inner part of flap
    glTexCoord2f(0.0f, 0.8f); //white
    glVertex3f(x[0], y[0], -dep2);
    glVertex3f(x[1], y[1], -dep2);
    glVertex3f(x[2], y[2], -dep2);
    glVertex3f(x[3], y[3], -dep2);

    //render edges of flap
    glTexCoord2f(0.0f, 0.1f); //black
    for(int i=0; i<4; i++)
    {
        int i2 = (i+1)%4;
        const float& x1 = x[i];
        const float& x2 = x[i2];
        const float& y1 = y[i];
        const float& y2 = y[i2];
        const glm::vec2 eN = glm::normalize(rotMx90deg * glm::vec2(x2-x1, y2-y1)) * normalScaler;

        glVertex3f(x1 - eN.x, y1 - eN.y, -dep);
        glVertex3f(x1 + eN.x, y1 + eN.y, -dep);
        glVertex3f(x2 + eN.x, y2 + eN.y, -dep);
        glVertex3f(x2 - eN.x, y2 - eN.y, -dep);
    }
}

void Renderer3D::RenderEdge(void *tr, int edge, int foldType) const
{
    const CMesh::STriangle2D& t = *static_cast<CMesh::STriangle2D*>(tr);
    const CMesh::STriGroup *g = t.GetGroup();
    const glm::vec2 &v1 = t[edge];
    const glm::vec2 &v2 = t[(edge+1)%3];
    const glm::vec2 vN = t.GetNormal(edge) * 0.015f * CSettings::GetInstance().GetLineWidth();
    const float len = t.GetEdgeLen(edge) * (float)CSettings::GetInstance().GetStippleLoop();
    const float dep = g->GetDepth() - CMesh::STriGroup::GetDepthStep()*0.3f;

    float foldSelector = 1.0f;

    switch(foldType)
    {
    case CMesh::SEdge::FT_FLAT:
        foldSelector = 1.0f;
        break;
    case CMesh::SEdge::FT_VALLEY:
        foldSelector = 2.0f;
        break;
    case CMesh::SEdge::FT_MOUNTAIN:
        foldSelector = 3.0f;
        break;
    default: assert(false);
    }

    static const float oneForth = 1.0f/4.0f;

    glTexCoord2f(0.0f, oneForth * (foldSelector - 1.0f) + 0.1f);
    glVertex3f(v1.x - vN.x, v1.y - vN.y, -dep);

    glTexCoord2f(0.0f, oneForth * foldSelector - 0.1f);
    glVertex3f(v1.x + vN.x, v1.y + vN.y, -dep);

    glTexCoord2f(len, oneForth * foldSelector - 0.1f);
    glVertex3f(v2.x + vN.x, v2.y + vN.y, -dep);

    glTexCoord2f(len, oneForth * (foldSelector - 1.0f) + 0.1f);
    glVertex3f(v2.x - vN.x, v2.y - vN.y, -dep);
}

void Renderer3D::DrawPaperSheet(glm::vec2 position, glm::vec2 size)
{
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(position.x, position.y, -3.0f);
    glVertex3f(position.x+size.x, position.y, -3.0f);
    glVertex3f(position.x+size.x, position.y+size.y, -3.0f);
    glVertex3f(position.x, position.y+size.y, -3.0f);
    glEnd();
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex3f(position.x+0.5f, position.y-0.5f, -10.0f);
    glVertex3f(position.x+0.5f+size.x, position.y-0.5f, -10.0f);
    glVertex3f(position.x+0.5f+size.x, position.y+size.y-0.5f, -10.0f);
    glVertex3f(position.x+0.5f, position.y+size.y-0.5f, -10.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex3f(position.x, position.y, -5.0f);
    glVertex3f(position.x+size.x, position.y, -5.0f);
    glVertex3f(position.x+size.x, position.y+size.y, -5.0f);
    glVertex3f(position.x, position.y+size.y, -5.0f);
    glEnd();
}

void Renderer3D::DrawGroups()
{
    const std::vector<glm::vec2> &uvs = m_model->GetUVCoords();

    glBegin(GL_TRIANGLES);
    const auto &parts = m_model->GetGroups();
    for(auto it=parts.begin(); it!=parts.end(); ++it)
    {
        const CMesh::STriGroup &grp = *it;
        const std::list<CMesh::STriangle2D*>& grpTris = grp.GetTriangles();

        for(auto it2=grpTris.begin(), itEnd = grpTris.end(); it2!=itEnd; ++it2)
        {
            const CMesh::STriangle2D& tr2D = **it2;
            const glm::uvec4 &t = m_model->GetTriangles()[tr2D.ID()];

            BindTexture(t[3]);

            const glm::vec2 vertex1 = tr2D[0];
            const glm::vec2 vertex2 = tr2D[1];
            const glm::vec2 vertex3 = tr2D[2];

            const glm::vec2 &uv1 = uvs[t[0]];
            const glm::vec2 &uv2 = uvs[t[1]];
            const glm::vec2 &uv3 = uvs[t[2]];

            glTexCoord2f(uv1[0], uv1[1]);
            glVertex3f(vertex1[0], vertex1[1], -grp.GetDepth());

            glTexCoord2f(uv2[0], uv2[1]);
            glVertex3f(vertex2[0], vertex2[1], -grp.GetDepth());

            glTexCoord2f(uv3[0], uv3[1]);
            glVertex3f(vertex3[0], vertex3[1], -grp.GetDepth());

        }
    }
    glEnd();

    UnbindTexture();
}
