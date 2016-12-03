#include <QOpenGLFramebufferObject>
#include <QEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include "mesh/mesh.h"
#include "renwin2d.h"
#include "settings/settings.h"

CRenWin2D::CRenWin2D(QWidget *parent) :
    QOpenGLWidget(parent), m_model(nullptr)
{
    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setMajorVersion(2);
    format.setMinorVersion(0);
    format.setSamples(2);
    setFormat(format);
    m_cameraPosition = glm::vec3(0.0f, 0.0f, 10.0f);
    m_w = m_h = 100.0f;
    m_boundTextureID = -1;
    setMouseTracking(true);
}

void CRenWin2D::SetMode(EditMode m)
{
    if(m_cameraMode == CAM_MODE)
    {
        ModeEnd();
    }

    m_editMode = m;
    m_cameraMode = CAM_STILL;
    m_currGroup = nullptr;
    m_currSheet = nullptr;
    update();
}

void CRenWin2D::SetModel(CMesh *mdl)
{
    m_currGroup = nullptr;
    m_currSheet = nullptr;
    m_model = mdl;
    m_sheets.clear();
    ZoomFit();
}

void CRenWin2D::CreateFoldTextures()
{
    QImage imgBlack(1, 1, QImage::Format_ARGB32);
    QImage imgValley(16, 1, QImage::Format_ARGB32);
    QImage imgMountain(16, 1, QImage::Format_ARGB32);
    imgBlack.fill(QColor(0,0,0,255));
    imgValley.fill(QColor(0,0,0,255));
    imgMountain.fill(QColor(0,0,0,255));

    for(int i=0; i<6; ++i)
    {
        imgValley.setPixelColor(i, 0, QColor(0,0,0,0));
        imgMountain.setPixelColor(i, 0, QColor(0,0,0,0));
    }
    imgMountain.setPixelColor(3, 0, QColor(0,0,0,255));

    m_texValleyFold.reset(new QOpenGLTexture(imgValley));
    m_texValleyFold->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    m_texValleyFold->setWrapMode(QOpenGLTexture::Repeat);

    m_texMountainFold.reset(new QOpenGLTexture(imgMountain));
    m_texMountainFold->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    m_texMountainFold->setWrapMode(QOpenGLTexture::Repeat);

    m_texPitchBlack.reset(new QOpenGLTexture(imgBlack));
    m_texPitchBlack->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    m_texPitchBlack->setWrapMode(QOpenGLTexture::Repeat);
}

CRenWin2D::~CRenWin2D()
{
    makeCurrent();
    for(auto it=m_textures.begin(); it!=m_textures.end(); it++)
    {
        if(it->second)
        {
            it->second->destroy();
        }
    }

    if(m_texValleyFold)
        m_texValleyFold->destroy();

    if(m_texMountainFold)
        m_texMountainFold->destroy();

    if(m_texPitchBlack)
        m_texPitchBlack->destroy();
    doneCurrent();
}

void CRenWin2D::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.7f, 0.7f, 0.7f, 0.7f);
    CreateFoldTextures();
}

void CRenWin2D::paintGL()
{
    if(m_model)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glTranslatef(m_cameraPosition[0], m_cameraPosition[1], -1.0f);

        RenderPaperSheets();

        RenderGroups();

        const unsigned char renFlags = CSettings::GetInstance().GetRenderFlags();

        glColor3f(0.0f, 0.0f, 0.0f);
        glLineWidth(CSettings::GetInstance().GetLineWidth());

        if(renFlags & CSettings::R_FLAPS)
        {
            RenderFlaps();
        }
        if(renFlags & (CSettings::R_EDGES | CSettings::R_FOLDS))
        {
            RenderEdges();
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(1.0f);

        RenderSelection();
    }
}

void CRenWin2D::ExportSheets(const QString baseName)
{
    if(m_sheets.empty()) return;

    QString dstFolder = QFileDialog::getExistingDirectory(this, "Save directory");

    if(dstFolder.isEmpty()) return;

    if(!(dstFolder.endsWith("/") || dstFolder.endsWith("\\")))
        dstFolder += "/";

    makeCurrent();

    const CSettings& sett = CSettings::GetInstance();
    QString imgFormat = "PNG";
    switch(sett.GetImageFormat())
    {
        case CSettings::IF_BMP :
        {
            imgFormat = "BMP";
            break;
        }
        case CSettings::IF_JPG :
        {
            imgFormat = "JPG";
            break;
        }
        case CSettings::IF_PNG :
        {
            imgFormat = "PNG";
            break;
        }
        default: assert(false);
    }
    const int papW = sett.GetPaperWidth();
    const int papH = sett.GetPaperHeight();
    const int fboW = (int)(papW * sett.GetResolutionScale());
    const int fboH = (int)(papH * sett.GetResolutionScale());
    const unsigned char renFlags = sett.GetRenderFlags();
    const unsigned char imgQuality = sett.GetImageQuality();

    QOpenGLFramebufferObject fbo(fboW, fboH, QOpenGLFramebufferObject::Depth, GL_TEXTURE_2D);
    if(!fbo.isValid())
    {
        QMessageBox::information(this, "Export Error", "Failed to create framebuffer object!");
        return;
    }
    fbo.bind();
    glViewport(0, 0, fboW, fboH);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-papW * 0.05f, papW * 0.05f, -papH * 0.05f, papH * 0.05f, 0.1f, 2000.0f);

    glMatrixMode(GL_MODELVIEW);
    int sheetNum = 1;
    for(auto &sheet : m_sheets)
    {
        glLoadIdentity();
        glTranslatef(-sheet.m_position.x - papW*0.05f, -sheet.m_position.y - papH*0.05f, -1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(1.0f);

        RenderGroups();

        glColor3f(0.0f, 0.0f, 0.0f);
        glLineWidth(sett.GetLineWidth());

        if(renFlags & CSettings::R_FLAPS)
        {
            RenderFlaps();
        }
        if(renFlags & (CSettings::R_EDGES | CSettings::R_FOLDS))
        {
            RenderEdges();
        }

        QImage img(fbo.toImage());

        if(!img.save(dstFolder + baseName + "_" + QString::number(sheetNum++) + "." + imgFormat.toLower(), imgFormat.toStdString().c_str(), imgQuality))
        {
            QMessageBox::information(this, "Export Error", "Failed to save one of image files!");
            break;
        }
    }

    glViewport(0, 0, m_w, m_h);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.0f);

    glClearColor(0.7f, 0.7f, 0.7f, 0.7f);

    assert(fbo.release());

    doneCurrent();

    QMessageBox::information(this, "Export", "Images have been exported successfully!");
}

void CRenWin2D::SerializeSheets(FILE *f) const
{
    int sheetsNum = m_sheets.size();
    std::fwrite(&sheetsNum, sizeof(sheetsNum), 1, f);
    for(const SPaperSheet& sh : m_sheets)
    {
        std::fwrite(&(sh.m_position), sizeof(glm::vec2), 1, f);
        std::fwrite(&(sh.m_widthHeight), sizeof(glm::vec2), 1, f);
    }
}

void CRenWin2D::DeserializeSheets(FILE *f)
{
    m_sheets.clear();
    m_currSheet = nullptr;

    int sheetsNum;
    std::fread(&sheetsNum, sizeof(sheetsNum), 1, f);

    for(int i=0; i<sheetsNum; ++i)
    {
        m_sheets.push_back(SPaperSheet());
        SPaperSheet &sh = m_sheets.back();

        std::fread(&(sh.m_position), sizeof(glm::vec2), 1, f);
        std::fread(&(sh.m_widthHeight), sizeof(glm::vec2), 1, f);
    }
}

void CRenWin2D::RenderPaperSheets()
{
    for(const SPaperSheet &ps : m_sheets)
    {
        glColor3f(0.0f, 0.0f, 0.0f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(ps.m_position.x, ps.m_position.y, -3.0f);
        glVertex3f(ps.m_position.x+ps.m_widthHeight.x, ps.m_position.y, -3.0f);
        glVertex3f(ps.m_position.x+ps.m_widthHeight.x, ps.m_position.y+ps.m_widthHeight.y, -3.0f);
        glVertex3f(ps.m_position.x, ps.m_position.y+ps.m_widthHeight.y, -3.0f);
        glEnd();
        glColor3f(0.2f, 0.2f, 0.2f);
        glBegin(GL_QUADS);
        glVertex3f(ps.m_position.x+0.5f, ps.m_position.y-0.5f, -10.0f);
        glVertex3f(ps.m_position.x+0.5f+ps.m_widthHeight.x, ps.m_position.y-0.5f, -10.0f);
        glVertex3f(ps.m_position.x+0.5f+ps.m_widthHeight.x, ps.m_position.y+ps.m_widthHeight.y-0.5f, -10.0f);
        glVertex3f(ps.m_position.x+0.5f, ps.m_position.y+ps.m_widthHeight.y-0.5f, -10.0f);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(ps.m_position.x, ps.m_position.y, -5.0f);
        glVertex3f(ps.m_position.x+ps.m_widthHeight.x, ps.m_position.y, -5.0f);
        glVertex3f(ps.m_position.x+ps.m_widthHeight.x, ps.m_position.y+ps.m_widthHeight.y, -5.0f);
        glVertex3f(ps.m_position.x, ps.m_position.y+ps.m_widthHeight.y, -5.0f);
        glEnd();
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void CRenWin2D::RenderGroups()
{
    const bool renTexture = CSettings::GetInstance().GetRenderFlags() & CSettings::R_TEXTR;

    const std::vector<glm::vec2> &uvs = m_model->GetUVCoords();

    glBegin(GL_TRIANGLES);
    for(auto it=m_model->m_groups.begin(); it!=m_model->m_groups.end(); ++it)
    {
        CMesh::STriGroup &grp = *it;
        const std::list<CMesh::STriangle2D*>& grpTris = grp.GetTriangles();

        for(auto it2=grpTris.begin(), itEnd = grpTris.end(); it2!=itEnd; ++it2)
        {
            const CMesh::STriangle2D& tr2D = **it2;
            const glm::uvec4 &t = m_model->GetTriangles()[tr2D.ID()];

            if(renTexture && m_boundTextureID != (int)t[3])
            {
                glEnd();
                if(m_textures[t[3]])
                {
                    m_textures[t[3]]->bind();
                } else if(m_boundTextureID >= 0 && m_textures[m_boundTextureID])
                {
                    m_textures[m_boundTextureID]->release();
                }
                glBegin(GL_TRIANGLES);
                m_boundTextureID = t[3];
            }

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

void CRenWin2D::RenderFlaps() const
{
    for(CMesh::SEdge &e : m_model->m_edges)
    {
        if(!e.IsSnapped() && e.GetFlapPosition() != CMesh::SEdge::FP_NONE)
        {
            if(e.GetFlapPosition() == CMesh::SEdge::FP_LEFT)
            {
                RenderFlap(e.GetTriangle(0), e.GetTriIndex(0));
            } else
            if(e.GetFlapPosition() == CMesh::SEdge::FP_RIGHT)
            {
                RenderFlap(e.GetTriangle(1), e.GetTriIndex(1));
            } else
            if(e.GetFlapPosition() == CMesh::SEdge::FP_BOTH)
            {
                RenderFlap(e.GetTriangle(0), e.GetTriIndex(0));
                RenderFlap(e.GetTriangle(1), e.GetTriIndex(1));
            }
        }
    }
}

void CRenWin2D::RenderEdges()
{
    const unsigned char renFlags = CSettings::GetInstance().GetRenderFlags();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glBegin(GL_QUADS);
    for(CMesh::SEdge &e : m_model->m_edges)
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

    if(m_texMountainFold && m_texMountainFold->isBound())
        m_texMountainFold->release();

    if(m_texValleyFold && m_texValleyFold->isBound())
        m_texValleyFold->release();

    if(m_texPitchBlack && m_texPitchBlack->isBound())
        m_texPitchBlack->release();
}

void CRenWin2D::RenderFlap(void *tr, int edge) const
{
    const CMesh::STriangle2D& t = *static_cast<CMesh::STriangle2D*>(tr);
    CMesh::STriGroup *g = t.GetGroup();
    float dep = g->GetDepth() + CMesh::STriGroup::GetDepthStep()*0.3f;
    const glm::vec2 &v1 = t[edge];
    const glm::vec2 &v2 = t[(edge+1)%3];
    const glm::vec2 &vN = t.GetNormal(edge);

    glBegin(GL_LINE_STRIP);

    for(int i=0; i<2; ++i)
    {
        if(t.IsFlapSharp(edge))
        {
            glVertex3f(v1.x, v1.y, -dep);
            glVertex3f(0.5f*v1.x+0.5f*v2.x+vN.x, 0.5f*v1.y+0.5f*v2.y+vN.y, -dep);
            glVertex3f(v2.x, v2.y, -dep);
        } else {
            glVertex3f(v1.x, v1.y, -dep);
            glVertex3f(0.9f*v1.x+0.1f*v2.x+vN.x, 0.9f*v1.y+0.1f*v2.y+vN.y, -dep);
            glVertex3f(0.1f*v1.x+0.9f*v2.x+vN.x, 0.1f*v1.y+0.9f*v2.y+vN.y, -dep);
            glVertex3f(v2.x, v2.y, -dep);
        }

        glEnd();

        if(i==0)
        {
            dep += CMesh::STriGroup::GetDepthStep()*0.15f;
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_POLYGON);
        }
    }
    glColor3f(0.0f, 0.0f, 0.0f);
}

void CRenWin2D::RenderEdge(void *tr, int edge, int foldType) const
{
    const CMesh::STriangle2D& t = *static_cast<CMesh::STriangle2D*>(tr);
    CMesh::STriGroup *g = t.GetGroup();
    const glm::vec2 &v1 = t[edge];
    const glm::vec2 &v2 = t[(edge+1)%3];
    const glm::vec2 vN = t.GetNormal(edge) * 0.04f * CSettings::GetInstance().GetLineWidth();
    float len = glm::length(v2 - v1) * 0.5f * (float)CSettings::GetInstance().GetStippleLoop();
    float dep = g->GetDepth() - CMesh::STriGroup::GetDepthStep()*0.3f;

    switch(foldType)
    {
    case CMesh::SEdge::FT_FLAT:
        if(m_texPitchBlack && !m_texPitchBlack->isBound())
        {
            glEnd();
            m_texPitchBlack->bind();
            glBegin(GL_QUADS);
        }
        break;
    case CMesh::SEdge::FT_VALLEY:
        if(m_texValleyFold && !m_texValleyFold->isBound())
        {
            glEnd();
            m_texValleyFold->bind();
            glBegin(GL_QUADS);
        }
        break;
    case CMesh::SEdge::FT_MOUNTAIN:
        if(m_texMountainFold && !m_texMountainFold->isBound())
        {
            glEnd();
            m_texMountainFold->bind();
            glBegin(GL_QUADS);
        }
        break;
    default: assert(false);
    }

    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(v1.x - vN.x, v1.y - vN.y, -dep);

    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(v1.x + vN.x, v1.y + vN.y, -dep);

    glTexCoord2f(len, 1.0f);
    glVertex3f(v2.x + vN.x, v2.y + vN.y, -dep);

    glTexCoord2f(len, 0.0f);
    glVertex3f(v2.x - vN.x, v2.y - vN.y, -dep);
}

void CRenWin2D::RenderSelection()
{
    glClear(GL_DEPTH_BUFFER_BIT);

    if(m_editMode == EM_SNAP || m_editMode == EM_CHANGE_FLAPS || m_editMode == EM_ROTATE)
    {
        CMesh::STriangle2D *trUnderCursor = nullptr;
        int edgeUnderCursor = 0;
        glm::vec2 mwp = PointToWorldCoords(m_mousePosition);
        m_model->GetStuffUnderCursor(mwp, trUnderCursor, edgeUnderCursor);

        if(m_editMode == EM_ROTATE && m_currTri)
        {
            trUnderCursor = (CMesh::STriangle2D*)m_currTri;
            edgeUnderCursor = m_currEdge;
        }

        //highlight edge under cursor (if it has neighbour)
        if(trUnderCursor && (trUnderCursor->GetEdge(edgeUnderCursor)->HasTwoTriangles() || m_editMode == EM_ROTATE))
        {
            if(m_editMode == EM_CHANGE_FLAPS &&
               trUnderCursor->GetEdge(edgeUnderCursor)->IsSnapped())
                return;

            const glm::vec2 &v1 = (*trUnderCursor)[0];
            const glm::vec2 &v2 = (*trUnderCursor)[1];
            const glm::vec2 &v3 = (*trUnderCursor)[2];

            glm::vec2 e1Middle;

            if(m_editMode == EM_CHANGE_FLAPS)
            {
                glColor3f(0.0f, 0.0f, 1.0f);
            } else if(m_editMode == EM_SNAP) {
                if(trUnderCursor->GetEdge(edgeUnderCursor)->IsSnapped())
                    glColor3f(1.0f, 0.0f, 0.0f);
                else
                    glColor3f(0.0f, 1.0f, 0.0f);
            } else {
                glColor3f(0.0f, 1.0f, 1.0f);
            }
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            switch(edgeUnderCursor)
            {
                case 0:
                    glVertex2f(v1[0], v1[1]);
                    glVertex2f(v2[0], v2[1]);
                    e1Middle = 0.5f*(v1+v2);
                    break;
                case 1:
                    glVertex2f(v3[0], v3[1]);
                    glVertex2f(v2[0], v2[1]);
                    e1Middle = 0.5f*(v3+v2);
                    break;
                case 2:
                    glVertex2f(v1[0], v1[1]);
                    glVertex2f(v3[0], v3[1]);
                    e1Middle = 0.5f*(v1+v3);
                    break;
                default : break;
            }

            if(m_editMode != EM_ROTATE)
            {
                CMesh::STriangle2D* tr2 = trUnderCursor->GetEdge(edgeUnderCursor)->GetOtherTriangle(trUnderCursor);
                int e2 = trUnderCursor->GetEdge(edgeUnderCursor)->GetOtherTriIndex(trUnderCursor);
                const glm::vec2 &v12 = (*tr2)[0];
                const glm::vec2 &v22 = (*tr2)[1];
                const glm::vec2 &v32 = (*tr2)[2];

                switch(e2)
                {
                    case 0:
                        glVertex2f(v12[0], v12[1]);
                        glVertex2f(v22[0], v22[1]);
                        glVertex2f(e1Middle[0], e1Middle[1]);
                        e1Middle = 0.5f*(v12+v22);
                        glVertex2f(e1Middle[0], e1Middle[1]);
                        break;
                    case 1:
                        glVertex2f(v32[0], v32[1]);
                        glVertex2f(v22[0], v22[1]);
                        glVertex2f(e1Middle[0], e1Middle[1]);
                        e1Middle = 0.5f*(v32+v22);
                        glVertex2f(e1Middle[0], e1Middle[1]);
                        break;
                    case 2:
                        glVertex2f(v12[0], v12[1]);
                        glVertex2f(v32[0], v32[1]);
                        glVertex2f(e1Middle[0], e1Middle[1]);
                        e1Middle = 0.5f*(v12+v32);
                        glVertex2f(e1Middle[0], e1Middle[1]);
                        break;
                    default : break;
                }
            }
            glEnd();
            glLineWidth(1.0f);
            glColor3f(1.0f, 1.0f, 1.0f);
        }
    } else {
        //draw selection rectangle
        if(m_currGroup)
        {
            CMesh::STriGroup *tGroup = static_cast<CMesh::STriGroup*>(m_currGroup);
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_LOOP);
            glm::vec2 pos = tGroup->GetPosition();
            float aabbxh = tGroup->GetAABBHalfSide();
            glVertex2f(pos[0]-aabbxh, pos[1]+aabbxh);
            glVertex2f(pos[0]+aabbxh, pos[1]+aabbxh);
            glVertex2f(pos[0]+aabbxh, pos[1]-aabbxh);
            glVertex2f(pos[0]-aabbxh, pos[1]-aabbxh);
            glEnd();
            glColor3f(1.0f, 1.0f, 1.0f);
        }
    }
}

void CRenWin2D::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    m_w = w;
    m_h = h;
    RecalcProjection();
}

bool CRenWin2D::event(QEvent *e)
{
    static glm::vec3 oldPos = m_cameraPosition;

    switch(e->type())
    {
        case QEvent::MouseButtonPress :
        {
            QMouseEvent *me = static_cast<QMouseEvent*>(e);

            if(m_cameraMode != CAM_STILL)
            {
                if(m_cameraMode == CAM_ZOOM && me->button() == Qt::LeftButton)
                {
                    m_cameraMode = CAM_TRANSLATE;
                }
                break;
            }

            m_mousePressPoint = me->pos();
            oldPos = m_cameraPosition;
            switch(me->button())
            {
                case Qt::MiddleButton :
                {
                    m_cameraMode = CAM_TRANSLATE;
                    break;
                }
                case Qt::RightButton :
                {
                    m_cameraMode = CAM_ZOOM;
                    break;
                }
                case Qt::LeftButton :
                {
                    m_cameraMode = CAM_MODE;
                    ModeLMB();
                    update();
                    break;
                }
                default : break;
            }
            break;
        }
        case QEvent::MouseButtonRelease :
        {
            if(m_cameraMode == CAM_MODE)
            {
                ModeEnd();
            }
            m_cameraMode = CAM_STILL;
            break;
        }
        case QEvent::MouseMove :
        {
            QMouseEvent *me = static_cast<QMouseEvent*>(e);
            QPointF newPos = me->pos();
            bool shouldUpdate = false;
            if(m_mousePosition != newPos && (m_editMode == EM_SNAP || m_editMode == EM_CHANGE_FLAPS || m_editMode == EM_ROTATE))
                shouldUpdate = true;
            m_mousePosition = newPos;
            switch(m_cameraMode)
            {
                case CAM_ZOOM :
                {
                    m_cameraPosition[2] = glm::clamp(oldPos[2] - static_cast<float>(m_mousePressPoint.ry() - newPos.ry())*0.005f, 0.1f, 1000000.0f);
                    makeCurrent();
                    RecalcProjection();
                    shouldUpdate = true;
                    break;
                }
                case CAM_TRANSLATE :
                {
                    m_cameraPosition[0] = oldPos[0] + static_cast<float>(newPos.rx() - m_mousePressPoint.rx())*0.0025f*m_cameraPosition[2];
                    m_cameraPosition[1] = oldPos[1] - static_cast<float>(newPos.ry() - m_mousePressPoint.ry())*0.0025f*m_cameraPosition[2];
                    shouldUpdate = true;
                    break;
                }
                case CAM_MODE :
                {
                    ModeUpdate(newPos);
                    shouldUpdate = true;
                    break;
                }
                default : break;
            }
            if(shouldUpdate)
                update();
            break;
        }
        default : return QWidget::event(e);
    }
    return true;
}

void CRenWin2D::RecalcProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float hwidth = m_cameraPosition[2] * m_w/m_h;
    glOrtho(-hwidth, hwidth, -m_cameraPosition[2], m_cameraPosition[2], 0.1f, 2000.0f);
}

void CRenWin2D::ReserveTextureID(unsigned id)
{
    if(m_textures.find(id) == m_textures.end())
    {
        m_textures[id] = nullptr;
    }
}

void CRenWin2D::LoadTexture(QImage* img, unsigned index)
{
    makeCurrent();
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
    update();
}

void CRenWin2D::ClearTextures()
{
    makeCurrent();
    for(auto it=m_textures.begin(); it!=m_textures.end(); it++)
    {
        if(it->second)
        {
            it->second->destroy();
            it->second.reset(nullptr);
        }
    }
    m_textures.clear();
    m_boundTextureID = -1;
    update();

}

glm::vec2 CRenWin2D::PointToWorldCoords(QPointF &pt) const
{
    float width = 2.0f * m_cameraPosition[2] * m_w/m_h;
    glm::vec2 topLeftWorldCoords = glm::vec2(-m_cameraPosition[0], -m_cameraPosition[1]);
    topLeftWorldCoords += glm::vec2(-width*0.5f, m_cameraPosition[2]);
    glm::vec2 pointWorldCoords = glm::vec2((pt.rx()/m_w)*width,
                                          -(pt.ry()/m_h)*m_cameraPosition[2]*2.0f );
    pointWorldCoords += topLeftWorldCoords ;
    return pointWorldCoords;
}

void CRenWin2D::ModeLMB()
{
    if(!m_model)
        return;

    m_currGroup = nullptr;
    glm::vec2 mouseWorldCoords = PointToWorldCoords(m_mousePressPoint);

    switch(m_editMode)
    {
        case EM_ROTATE :
        {
            CMesh::STriangle2D *trUnderCursor = nullptr;
            m_model->GetStuffUnderCursor(mouseWorldCoords, trUnderCursor, m_currEdge);

            const CMesh::STriGroup *tGroup = m_model->GroupUnderCursor(mouseWorldCoords);
            m_currGroup = static_cast<void*>(const_cast<CMesh::STriGroup*>(tGroup));
            if(!tGroup)
            {
                m_cameraMode = CAM_STILL;
                break;
            }
            m_currTri = (void*)trUnderCursor;
            if(trUnderCursor)
            {
                CMesh::STriangle2D& trRef = *trUnderCursor;
                m_currEdgeVec = glm::normalize(trRef[(m_currEdge+1)%3] - trRef[m_currEdge]);
            }
            m_fromCurrGroupCenter = mouseWorldCoords - tGroup->GetPosition();
            m_currGroupLastRot = tGroup->GetRotation();
            m_currGroupOldRot = tGroup->GetRotation();
            break;
        }
        case EM_MOVE :
        {
            const CMesh::STriGroup *tGroup = m_model->GroupUnderCursor(mouseWorldCoords);
            m_currGroup = static_cast<void*>(const_cast<CMesh::STriGroup*>(tGroup));
            if(!tGroup)
            {
                m_cameraMode = CAM_STILL;
                return;
            }
            m_fromCurrGroupCenter = mouseWorldCoords - tGroup->GetPosition();
            m_currGroupOldPos = tGroup->GetPosition();
            break;
        }
        case EM_SNAP :
        {
            CMesh::STriangle2D *trUnderCursor = nullptr;
            int edgeUnderCursor = 0;
            m_model->GetStuffUnderCursor(mouseWorldCoords, trUnderCursor, edgeUnderCursor);
            if(trUnderCursor)
            {
                CMesh::STriGroup* grp = trUnderCursor->GetGroup();
                if(trUnderCursor->GetEdge(edgeUnderCursor)->IsSnapped())
                {
                    grp->BreakEdge(trUnderCursor, edgeUnderCursor);
                } else {
                    grp->JoinEdge(trUnderCursor, edgeUnderCursor);
                }
            }
            m_cameraMode = CAM_STILL;
            break;
        }
        case EM_CHANGE_FLAPS :
        {
            CMesh::STriangle2D *trUnderCursor = nullptr;
            int edgeUnderCursor = 0;
            m_model->GetStuffUnderCursor(mouseWorldCoords, trUnderCursor, edgeUnderCursor);
            if(trUnderCursor)
            {
                trUnderCursor->GetEdge(edgeUnderCursor)->NextFlapPosition();
            }
            m_cameraMode = CAM_STILL;
            break;
        }
        case EM_ADD_SHEET :
        {
            mouseWorldCoords.x = static_cast<float>(static_cast<int>(mouseWorldCoords.x));
            mouseWorldCoords.y = static_cast<float>(static_cast<int>(mouseWorldCoords.y));
            const CSettings& sett = CSettings::GetInstance();
            glm::vec2 papSize((float)sett.GetPaperWidth(), (float)sett.GetPaperHeight());
            m_sheets.push_back(SPaperSheet{mouseWorldCoords, papSize * 0.1f});
            m_cameraMode = CAM_STILL;
            break;
        }
        case EM_MOVE_SHEET :
        {
            m_currSheet = nullptr;
            for(auto it = m_sheets.begin(); it != m_sheets.end(); ++it)
            {
                const SPaperSheet &s = *it;
                if(mouseWorldCoords.x >= s.m_position.x &&
                   mouseWorldCoords.x <= s.m_position.x + s.m_widthHeight.x &&
                   mouseWorldCoords.y >= s.m_position.y &&
                   mouseWorldCoords.y <= s.m_position.y + s.m_widthHeight.y)
                {
                    m_currSheet = &(*it);
                    break;
                }
            }
            if(!m_currSheet)
            {
                m_cameraMode = CAM_STILL;
                return;
            }
            m_fromCurrGroupCenter = mouseWorldCoords - m_currSheet->m_position;
            break;
        }
        case EM_REM_SHEET :
        {
            m_currSheet = nullptr;
            for(auto it = m_sheets.begin(); it != m_sheets.end(); ++it)
            {
                const SPaperSheet &s = *it;
                if(mouseWorldCoords.x >= s.m_position.x &&
                   mouseWorldCoords.x <= s.m_position.x + s.m_widthHeight.x &&
                   mouseWorldCoords.y >= s.m_position.y &&
                   mouseWorldCoords.y <= s.m_position.y + s.m_widthHeight.y)
                {
                    m_sheets.erase(it);
                    break;
                }
            }
            break;
        }
    default : break;
    }
}

void CRenWin2D::ModeUpdate(QPointF &mpos)
{
    if(!m_model)
        return;

    switch(m_editMode)
    {
        case EM_MOVE :
        {
            glm::vec2 mNewPos = PointToWorldCoords(mpos) - m_fromCurrGroupCenter;
            if(m_currGroup)
                static_cast<CMesh::STriGroup*>(m_currGroup)->SetPosition(mNewPos[0], mNewPos[1]);
            break;
        }
        case EM_ROTATE :
        {
            if(!m_currGroup)
                break;
            CMesh::STriGroup *tGroup = static_cast<CMesh::STriGroup*>(m_currGroup);

            glm::vec2 mNewPos = PointToWorldCoords(mpos) - tGroup->GetPosition();
            float newAngle = glm::dot(mNewPos, m_fromCurrGroupCenter) / (glm::length(mNewPos) * glm::length(m_fromCurrGroupCenter));
            newAngle = glm::degrees(glm::acos(newAngle));

            glm::vec2 &vA = m_fromCurrGroupCenter,
                      &vB = mNewPos;

            if(vA[0]*vB[1] - vB[0]*vA[1] < 0.0f)
            {
                newAngle *= -1.0f;
            }

            static const float snapDelta = 5.0f;
            if(m_currTri)
            {
                const CMesh::STriangle2D& tri = *(CMesh::STriangle2D*)m_currTri;
                const glm::vec2& triV1 = tri[m_currEdge];
                const glm::vec2& triV2 = tri[(m_currEdge+1)%3];
                glm::vec2 edgeVec = glm::normalize(triV2 - triV1);
                float angleOX = glm::acos(glm::clamp(edgeVec.x, -1.0f, 1.0f));
                if(edgeVec.y < 0.0f)
                {
                    angleOX *= -1.0f;
                }
                angleOX = glm::degrees(angleOX);

                const glm::mat2 rotMx(glm::vec2(glm::cos(glm::radians(newAngle)), glm::sin(glm::radians(newAngle))),
                                      glm::vec2(-glm::sin(glm::radians(newAngle)), glm::cos(glm::radians(newAngle))));
                glm::vec2 edgeVecRotated = rotMx * m_currEdgeVec;
                float currAngleOX = glm::acos(glm::clamp(edgeVecRotated.x, -1.0f, 1.0f));
                if(edgeVecRotated.y < 0.0f)
                {
                    currAngleOX *= -1.0f;
                }
                currAngleOX = glm::degrees(currAngleOX);
                for(float snapAngle = -180.0f; snapAngle < 200.0f; snapAngle += 45.0f)
                {
                    if(glm::abs(snapAngle - currAngleOX) < snapDelta)
                    {
                        newAngle = tGroup->GetRotation() + snapAngle - angleOX - m_currGroupLastRot;
                        break;
                    }
                }
            }

            tGroup->SetRotation(newAngle + m_currGroupLastRot);
            break;
        }
        case EM_MOVE_SHEET :
        {
            glm::vec2 mNewPos = PointToWorldCoords(mpos) - m_fromCurrGroupCenter;
            mNewPos.x = static_cast<float>(static_cast<int>(mNewPos.x));
            mNewPos.y = static_cast<float>(static_cast<int>(mNewPos.y));
            if(m_currSheet)
                m_currSheet->m_position = mNewPos;
            break;
        }
    default : break;
    }
}

void CRenWin2D::ModeEnd()
{
    if(!m_model || !m_currGroup)
        return;
    CMesh::STriGroup *tGroup = static_cast<CMesh::STriGroup*>(m_currGroup);

    switch(m_editMode)
    {
        case EM_MOVE :
        {
            m_model->NotifyGroupMovement(*tGroup, m_currGroupOldPos);
            break;
        }
        case EM_ROTATE :
        {
            m_model->NotifyGroupRotation(*tGroup, m_currGroupOldRot);
            m_currTri = nullptr;
            break;
        }
    default : break;
    }
}

void CRenWin2D::UpdateSheetsSize()
{
    const CSettings& sett = CSettings::GetInstance();
    glm::vec2 papSize((float)sett.GetPaperWidth(), (float)sett.GetPaperHeight());
    papSize *= 0.1f;
    for(auto &sheet : m_sheets)
    {
        sheet.m_widthHeight = papSize;
    }
}

void CRenWin2D::ZoomFit()
{
    if(!m_model)
        return;
    float highestY = -99999999999999.0f,
          lowestY  = 99999999999999.0f,
          highestX = -99999999999999.0f,
          lowestX  = 99999999999999.0f;
    for(const CMesh::STriGroup& grp : m_model->m_groups)
    {
        const glm::vec2 grpPos = grp.GetPosition();
        highestY = glm::max(highestY, grpPos.y + grp.GetAABBHalfSide());
        lowestY = glm::min(lowestY, grpPos.y - grp.GetAABBHalfSide());
        highestX = glm::max(highestX, grpPos.x + grp.GetAABBHalfSide());
        lowestX = glm::min(lowestX, grpPos.x - grp.GetAABBHalfSide());
    }

    m_cameraPosition = glm::vec3(-((highestX + lowestX) * 0.5f), -((highestY + lowestY) * 0.5f), glm::max((highestY - lowestY) * 0.5f, (highestX - lowestX) * 0.5f));

    makeCurrent();
    RecalcProjection();
    update();
}
