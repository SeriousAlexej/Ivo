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
#include "io/saferead.h"
#include "rendererclassic.h"

CRenWin2D::CRenWin2D(QWidget *parent) :
    IRenWin(parent)
{
    m_cameraPosition = glm::vec3(0.0f, 0.0f, 10.0f);
    m_w = m_h = 100.0f;
    setMouseTracking(true);

    m_rend = new Renderer3D();
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
    if (m_model)
    {
        m_rend->SetModel(m_model);
        ZoomFit();
    }
}


CRenWin2D::~CRenWin2D()
{
    makeCurrent();
    if(m_texFolds)
        m_texFolds->destroy();

    doneCurrent();
}

void CRenWin2D::initializeGL()
{
    initializeOpenGLFunctions();
    m_rend->Init2D();
}

void CRenWin2D::DrawParts(const unsigned char renFlags)
{
    if(renFlags & CSettings::R_FLAPS)
    {
        m_rend->DrawFlaps();
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    m_rend->DrawGroups();

    if(renFlags & (CSettings::R_EDGES | CSettings::R_FOLDS))
    {
        m_rend->DrawEdges();
    }
}

void CRenWin2D::paintGL()
{
    if(!m_model)
        return;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(m_cameraPosition[0], m_cameraPosition[1], -1.0f);

    for(const SPaperSheet &ps : m_sheets)
    {
        m_rend->DrawPaperSheet(ps.m_position, ps.m_widthHeight);
        glClear(GL_DEPTH_BUFFER_BIT);  //todo why?
    }

    const unsigned char renFlags = CSettings::GetInstance().GetRenderFlags();

    DrawParts(renFlags);

    RenderSelection();
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
        QOpenGLFramebufferObjectFormat fboFormat;
        fboFormat.setSamples(6);
        fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
        fboFormat.setTextureTarget(GL_TEXTURE_2D_MULTISAMPLE);
        QOpenGLFramebufferObject fbo(fboW, fboH, fboFormat);
        if(!fbo.isValid())
        {
            QMessageBox::information(this, "Export Error", "Failed to create framebuffer object!");
            break;
        }
        fbo.bind();

        glLoadIdentity();
        glTranslatef(-sheet.m_position.x - papW*0.05f, -sheet.m_position.y - papH*0.05f, -1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        DrawParts(renFlags);

        QImage img(fbo.toImage());

        assert(fbo.release());

        if(!img.save(dstFolder + baseName + "_" + QString::number(sheetNum++) + "." + imgFormat.toLower(), imgFormat.toStdString().c_str(), imgQuality))
        {
            QMessageBox::information(this, "Export Error", "Failed to save one of image files!");
            break;
        }
    }

    glViewport(0, 0, m_w, m_h);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glClearColor(0.7f, 0.7f, 0.7f, 0.7f);

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
    SAFE_FREAD(&sheetsNum, sizeof(sheetsNum), 1, f);

    for(int i=0; i<sheetsNum; ++i)
    {
        m_sheets.push_back(SPaperSheet());
        SPaperSheet &sh = m_sheets.back();

        SAFE_FREAD(&(sh.m_position), sizeof(glm::vec2), 1, f);
        SAFE_FREAD(&(sh.m_widthHeight), sizeof(glm::vec2), 1, f);
    }
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
        case QEvent::Wheel :
        {
            QWheelEvent *we = static_cast<QWheelEvent*>(e);
            m_cameraPosition[2] = glm::clamp(m_cameraPosition[2] + 0.01f*we->delta(), 0.1f, 1000000.0f);
            makeCurrent();
            RecalcProjection();
            update();
            break;
        }
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
                    m_cameraPosition[2] = glm::clamp(oldPos[2] - static_cast<float>(m_mousePressPoint.ry() - newPos.ry())*0.1f, 0.1f, 1000000.0f);
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

void CRenWin2D::AddSheet(const glm::vec2 &pos, const glm::vec2 &widHei)
{
    m_sheets.push_back(SPaperSheet{pos, widHei});
}

void CRenWin2D::RecalcProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float hwidth = m_cameraPosition[2] * m_w/m_h;
    glOrtho(-hwidth, hwidth, -m_cameraPosition[2], m_cameraPosition[2], 0.1f, 2000.0f);
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
            AddSheet(mouseWorldCoords, papSize * 0.1f);
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

