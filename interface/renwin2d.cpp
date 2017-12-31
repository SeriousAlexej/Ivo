#include <QOpenGLFramebufferObject>
#include <QEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <limits>
#include "mesh/mesh.h"
#include "renwin2d.h"
#include "settings/settings.h"
#include "renderers/renderlegacy2d.h"
#include "interface/selectioninfo.h"

using glm::vec2;
using glm::vec3;
using glm::mat2;
using glm::length;
using glm::normalize;
using glm::clamp;
using glm::degrees;
using glm::radians;
using glm::dot;
using glm::acos;
using glm::sin;
using glm::cos;
using glm::abs;
using glm::max;
using glm::min;

struct CRenWin2D::SEditInfo
{
    QPointF             mousePressPoint;
    CRenWin2D::EditMode editMode = CRenWin2D::EM_MOVE;
    CMesh::STriGroup*   currGroup = nullptr;
    CMesh::STriangle2D* currTri = nullptr;
    int                 currEdge = -1;
    vec2                currEdgeVec;
    vec2                currGroupOldPos = vec2(0.0f, 0.0f);
    float               currGroupOldRot = 0.0f;
    vec2                fromCurrGroupCenter = vec2(0.0f,0.0f);
    float               currGroupLastRot = 0.0f;
    QPointF             mousePosition;
};

CRenWin2D::CRenWin2D(QWidget *parent) :
    IRenWin(parent)
{
    m_cameraPosition = vec3(0.0f, 0.0f, 10.0f);
    m_w = m_h = 100.0f;
    setMouseTracking(true);

    m_editInfo.reset(new SEditInfo());
}

CRenWin2D::~CRenWin2D()
{
    makeCurrent();
    m_renderer.reset(nullptr);
    doneCurrent();
}

void CRenWin2D::ClearSelection()
{
    m_editInfo->currGroup = nullptr;
    update();
}

void CRenWin2D::SetMode(EditMode m)
{
    if(m_cameraMode == CAM_MODE)
    {
        ModeEnd();
    }

    m_editInfo->editMode = m;
    m_cameraMode = CAM_STILL;
    m_editInfo->currGroup = nullptr;
    update();
}

void CRenWin2D::SetModel(CMesh *mdl)
{
    m_editInfo->currGroup = nullptr;
    m_model = mdl;
    m_renderer->SetModel(mdl);
    ZoomFit();
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
    QOpenGLFunctions_2_0* gfx = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_2_0>();
    if(!gfx)
        throw std::logic_error("OpenGL 2.0 is not available!");
    m_renderer.reset(new CRenderer2DLegacy(*gfx));
    m_renderer->Init();
    RecalcProjection();
}

void CRenWin2D::FillOccupiedSheetsSize(unsigned &horizontal, unsigned &vertical) const
{
    horizontal = 0u;
    vertical = 0u;

    if(!m_model)
        return;

    const CSettings& sett = CSettings::GetInstance();
    unsigned papHeight = sett.GetPaperHeight();
    unsigned papWidth = sett.GetPaperWidth();
    const vec2 rightBottom = m_model->GetAABBox2D().GetRightBottom();
    if(rightBottom.x > 0.0f && rightBottom.y < 0.0f)
    {
        horizontal = 1u + static_cast<unsigned>(rightBottom.x * 10.0f) / papWidth;
        vertical   = 1u + static_cast<unsigned>(-rightBottom.y * 10.0f) / papHeight;
    }
}

void CRenWin2D::paintGL()
{
    const SSelectionInfo selInfo {PointToWorldCoords(m_editInfo->mousePosition),
                                  m_editInfo->editMode,
                                  m_editInfo->currGroup,
                                  m_editInfo->currTri,
                                  m_editInfo->currEdge};

    m_renderer->PreDraw();

    unsigned papHorizontal;
    unsigned papVertical;
    FillOccupiedSheetsSize(papHorizontal, papVertical);
    m_renderer->DrawPaperSheets(papHorizontal, papVertical);

    m_renderer->DrawScene();
    m_renderer->DrawSelection(selInfo);
    m_renderer->PostDraw();
}

void CRenWin2D::ExportSheets(const QString baseName)
{
    if(!m_model)
        return;

    unsigned papHorizontal;
    unsigned papVertical;
    FillOccupiedSheetsSize(papHorizontal, papVertical);
    if(papHorizontal == 0 || papVertical == 0)
        return;

    QString dstFolder = QFileDialog::getExistingDirectory(this, "Save directory");

    if(dstFolder.isEmpty()) return;

    if(!(dstFolder.endsWith("/") || dstFolder.endsWith("\\")))
        dstFolder += "/";

    const CSettings& sett = CSettings::GetInstance();
    unsigned papHeight = sett.GetPaperHeight();
    unsigned papWidth = sett.GetPaperWidth();
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
    for(unsigned x=0; x<papHorizontal; x++)
    for(unsigned y=0; y<papVertical; y++)
    {
        const vec2 sheetPos(x * papWidth * 0.1f, (y+1) * (papHeight * 0.1f) * -1.0f);
        if(!m_model->Intersects(
                    SAABBox2D(vec2(sheetPos.x + papWidth * 0.1f, sheetPos.y),
                              vec2(sheetPos.x, sheetPos.y + papHeight * 0.1f))
                    ))
            continue;

        QImage img;
        try
        {
            img = m_renderer->DrawImageFromSheet(sheetPos);
        } catch(std::exception& error)
        {
            QMessageBox::information(this, "Export Error", error.what());
            doneCurrent();
            return;
        }

        if(!img.save(dstFolder + baseName + "_" + QString::number(sheetNum++) + "." + imgFormat.toLower(),
                     imgFormat.toStdString().c_str(),
                     imgQuality))
        {
            QMessageBox::information(this, "Export Error", "Failed to save one of image files!");
            doneCurrent();
            return;
        }
    }

    doneCurrent();

    QMessageBox::information(this, "Export", "Images have been exported successfully!");
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
    static vec3 oldPos = m_cameraPosition;

    switch(e->type())
    {
        case QEvent::Wheel :
        {
            QWheelEvent *we = static_cast<QWheelEvent*>(e);
            m_cameraPosition[2] = clamp(float(m_cameraPosition[2] + 0.01f*we->delta()), 0.1f, 1000000.0f);
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

            m_editInfo->mousePressPoint = me->pos();
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
            const auto& editMode = m_editInfo->editMode;
            if(m_editInfo->mousePosition != newPos &&
                    (editMode == EM_SNAP           ||
                     editMode == EM_CHANGE_FLAPS   ||
                     editMode == EM_ROTATE))
                shouldUpdate = true;
            m_editInfo->mousePosition = newPos;
            switch(m_cameraMode)
            {
                case CAM_ZOOM :
                {
                    m_cameraPosition[2] = clamp(oldPos[2] - float(m_editInfo->mousePressPoint.ry() - newPos.ry())*0.1f, 0.1f, 1000000.0f);
                    makeCurrent();
                    RecalcProjection();
                    doneCurrent();
                    shouldUpdate = true;
                    break;
                }
                case CAM_TRANSLATE :
                {
                    m_cameraPosition[0] = oldPos[0] + (newPos.rx() - m_editInfo->mousePressPoint.rx())*0.0025f*m_cameraPosition[2];
                    m_cameraPosition[1] = oldPos[1] - (newPos.ry() - m_editInfo->mousePressPoint.ry())*0.0025f*m_cameraPosition[2];
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

void CRenWin2D::RecalcProjection()
{
    m_renderer->UpdateCameraPosition(m_cameraPosition);
    m_renderer->RecalcProjection();
}

vec2 CRenWin2D::PointToWorldCoords(QPointF &pt) const
{
    float width = 2.0f * m_cameraPosition[2] * m_w/m_h;
    vec2 topLeftWorldCoords = vec2(-m_cameraPosition[0], -m_cameraPosition[1]);
    topLeftWorldCoords += vec2(-width*0.5f, m_cameraPosition[2]);
    vec2 pointWorldCoords = vec2((pt.rx()/m_w)*width,
                                          -(pt.ry()/m_h)*m_cameraPosition[2]*2.0f );
    pointWorldCoords += topLeftWorldCoords ;
    return pointWorldCoords;
}

void CRenWin2D::ModeLMB()
{
    if(!m_model)
        return;

    m_editInfo->currGroup = nullptr;
    vec2 mouseWorldCoords = PointToWorldCoords(m_editInfo->mousePressPoint);

    switch(m_editInfo->editMode)
    {
        case EM_ROTATE :
        {
            CMesh::STriangle2D *trUnderCursor = nullptr;
            m_model->GetStuffUnderCursor(mouseWorldCoords, trUnderCursor, m_editInfo->currEdge);

            CMesh::STriGroup *tGroup = m_model->GroupUnderCursor(mouseWorldCoords);
            m_editInfo->currGroup = tGroup;
            if(!tGroup)
            {
                m_cameraMode = CAM_STILL;
                break;
            }
            m_editInfo->currTri = trUnderCursor;
            if(trUnderCursor)
            {
                CMesh::STriangle2D& trRef = *trUnderCursor;
                m_editInfo->currEdgeVec = normalize(trRef[(m_editInfo->currEdge+1)%3] - trRef[m_editInfo->currEdge]);
            }
            m_editInfo->fromCurrGroupCenter = mouseWorldCoords - tGroup->GetPosition();
            m_editInfo->currGroupLastRot = tGroup->GetRotation();
            m_editInfo->currGroupOldRot = tGroup->GetRotation();
            break;
        }
        case EM_MOVE :
        {
            CMesh::STriGroup* tGroup = m_model->GroupUnderCursor(mouseWorldCoords);
            m_editInfo->currGroup = tGroup;
            if(!tGroup)
            {
                m_cameraMode = CAM_STILL;
                return;
            }
            m_editInfo->fromCurrGroupCenter = mouseWorldCoords - tGroup->GetPosition();
            m_editInfo->currGroupOldPos = tGroup->GetPosition();
            break;
        }
        case EM_SNAP :
        {
            CMesh::STriangle2D* trUnderCursor = nullptr;
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
    default : break;
    }
}

void CRenWin2D::ModeUpdate(QPointF &mpos)
{
    if(!m_model)
        return;

    switch(m_editInfo->editMode)
    {
        case EM_MOVE :
        {
            const vec2 mNewPos = PointToWorldCoords(mpos) - m_editInfo->fromCurrGroupCenter;
            if(m_editInfo->currGroup)
                m_editInfo->currGroup->SetPosition(mNewPos[0], mNewPos[1]);
            break;
        }
        case EM_ROTATE :
        {
            if(!m_editInfo->currGroup)
                break;
            CMesh::STriGroup *tGroup = static_cast<CMesh::STriGroup*>(m_editInfo->currGroup);

            vec2 mNewPos = PointToWorldCoords(mpos) - tGroup->GetPosition();
            float newAngle = dot(mNewPos, m_editInfo->fromCurrGroupCenter) / (length(mNewPos) * length(m_editInfo->fromCurrGroupCenter));
            newAngle = degrees(acos(newAngle));

            vec2 &vA = m_editInfo->fromCurrGroupCenter,
                      &vB = mNewPos;

            if(vA[0]*vB[1] - vB[0]*vA[1] < 0.0f)
            {
                newAngle *= -1.0f;
            }

            static const float snapDelta = 5.0f;
            if(m_editInfo->currTri)
            {
                const CMesh::STriangle2D& tri = *(m_editInfo->currTri);
                const vec2& triV1 = tri[m_editInfo->currEdge];
                const vec2& triV2 = tri[(m_editInfo->currEdge+1)%3];
                vec2 edgeVec = normalize(triV2 - triV1);
                float angleOX = acos(clamp(edgeVec.x, -1.0f, 1.0f));
                if(edgeVec.y < 0.0f)
                {
                    angleOX *= -1.0f;
                }
                angleOX = degrees(angleOX);

                const float angleRad = radians(newAngle);
                const mat2 rotMx(vec2(cos(angleRad), sin(angleRad)),
                                 vec2(-sin(angleRad), cos(angleRad)));
                vec2 edgeVecRotated = rotMx * m_editInfo->currEdgeVec;
                float currAngleOX = acos(clamp(edgeVecRotated.x, -1.0f, 1.0f));
                if(edgeVecRotated.y < 0.0f)
                {
                    currAngleOX *= -1.0f;
                }
                currAngleOX = degrees(currAngleOX);
                for(float snapAngle = -180.0f; snapAngle < 200.0f; snapAngle += 45.0f)
                {
                    if(abs(snapAngle - currAngleOX) < snapDelta)
                    {
                        newAngle = tGroup->GetRotation() + snapAngle - angleOX - m_editInfo->currGroupLastRot;
                        break;
                    }
                }
            }

            tGroup->SetRotation(newAngle + m_editInfo->currGroupLastRot);
            break;
        }
    default : break;
    }
}

void CRenWin2D::ModeEnd()
{
    if(!m_model || !m_editInfo->currGroup)
        return;
    CMesh::STriGroup& tGroup = *(m_editInfo->currGroup);

    switch(m_editInfo->editMode)
    {
        case EM_MOVE :
        {
            m_model->NotifyGroupMovement(tGroup, m_editInfo->currGroupOldPos);
            break;
        }
        case EM_ROTATE :
        {
            m_model->NotifyGroupRotation(tGroup, m_editInfo->currGroupOldRot);
            m_editInfo->currTri = nullptr;
            break;
        }
    default : break;
    }
}

void CRenWin2D::ZoomFit()
{
    if(!m_model)
        return;
    float highestY = std::numeric_limits<float>::lowest();
    float lowestY  = std::numeric_limits<float>::max();
    float highestX = highestY;
    float lowestX  = lowestY;

    const std::list<CMesh::STriGroup>& groups = m_model->GetGroups();
    for(const CMesh::STriGroup& grp : groups)
    {
        const vec2 grpPos = grp.GetPosition();
        highestY = max(highestY, grpPos.y + grp.GetAABBHalfSide());
        lowestY  = min(lowestY,  grpPos.y - grp.GetAABBHalfSide());
        highestX = max(highestX, grpPos.x + grp.GetAABBHalfSide());
        lowestX  = min(lowestX,  grpPos.x - grp.GetAABBHalfSide());
    }

    m_cameraPosition = vec3(-((highestX + lowestX) * 0.5f),
                            -((highestY + lowestY) * 0.5f),
                            max((highestY - lowestY) * 0.5f,(highestX - lowestX) * 0.5f));

    makeCurrent();
    RecalcProjection();
    doneCurrent();
    update();
}

