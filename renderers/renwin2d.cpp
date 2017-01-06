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
#include "renderlegacy2d.h"

CRenWin2D::CRenWin2D(QWidget *parent) :
    IRenWin(parent)
{
    m_cameraPosition = glm::vec3(0.0f, 0.0f, 10.0f);
    m_w = m_h = 100.0f;
    setMouseTracking(true);

    m_renderer = std::unique_ptr<IRenderer2D>(new CRenderer2DLegacy());
}

CRenWin2D::~CRenWin2D()
{
    makeCurrent();
    m_renderer.reset(nullptr);
    doneCurrent();
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
    m_renderer->SetModel(mdl);
    m_sheets.clear();
    ZoomFit();
}

void CRenWin2D::ReserveTextureID(unsigned id)
{
    m_renderer->ReserveTextureID(id);
}

void CRenWin2D::LoadTexture(const QImage *img, unsigned index)
{
    makeCurrent();
    m_renderer->LoadTexture(img, index);
    doneCurrent();
    update();
}

void CRenWin2D::ClearTextures()
{
    makeCurrent();
    m_renderer->ClearTextures();
    doneCurrent();
    update();
}

void CRenWin2D::initializeGL()
{
    initializeOpenGLFunctions();
    m_renderer->Init();
    RecalcProjection();
}

void CRenWin2D::paintGL()
{
    const SSelectionInfo selInfo {PointToWorldCoords(m_mousePosition),
                                  (int)m_editMode,
                                  (void*)m_currGroup,
                                  (void*)m_currTri,
                                  m_currEdge};

    m_renderer->PreDraw();

    for(const SPaperSheet &ps : m_sheets)
    {
        m_renderer->DrawPaperSheet(ps.m_position, ps.m_widthHeight);
    }

    m_renderer->DrawScene();
    m_renderer->DrawSelection(selInfo);
    m_renderer->PostDraw();
}

void CRenWin2D::ExportSheets(const QString baseName)
{
    if(m_sheets.empty()) return;

    QString dstFolder = QFileDialog::getExistingDirectory(this, "Save directory");

    if(dstFolder.isEmpty()) return;

    if(!(dstFolder.endsWith("/") || dstFolder.endsWith("\\")))
        dstFolder += "/";

    const CSettings& sett = CSettings::GetInstance();

    const unsigned char imgQuality = sett.GetImageQuality();
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

    makeCurrent();

    int sheetNum = 1;
    for(const SPaperSheet& sheet : m_sheets)
    {
        QImage img;
        try
        {
            img = m_renderer->DrawImageFromSheet(sheet.m_position);
        } catch(std::exception& error)
        {
            QMessageBox::information(this, "Export Error", error.what());
            doneCurrent();
            return;
        }

        if(!img.save(dstFolder + baseName + "_" + QString::number(sheetNum++) + "." + imgFormat.toLower(), imgFormat.toStdString().c_str(), imgQuality))
        {
            QMessageBox::information(this, "Export Error", "Failed to save one of image files!");
            doneCurrent();
            return;
        }
    }

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

void CRenWin2D::resizeGL(int w, int h)
{
    m_w = w;
    m_h = h;
    m_renderer->ResizeView(w, h);
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
            doneCurrent();
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
                    doneCurrent();
                    shouldUpdate = true;
                    break;
                }
                case CAM_TRANSLATE :
                {
                    m_cameraPosition[0] = oldPos[0] + static_cast<float>(newPos.rx() - m_mousePressPoint.rx())*0.0025f*m_cameraPosition[2];
                    m_cameraPosition[1] = oldPos[1] - static_cast<float>(newPos.ry() - m_mousePressPoint.ry())*0.0025f*m_cameraPosition[2];
                    m_renderer->UpdateCameraPosition(m_cameraPosition);
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
    m_renderer->UpdateCameraPosition(m_cameraPosition);
    m_renderer->RecalcProjection();
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

    const auto groups = m_model->GetGroups();
    for(const CMesh::STriGroup& grp : groups)
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
    doneCurrent();
    update();
}

