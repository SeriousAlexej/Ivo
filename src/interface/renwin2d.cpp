/*
    Ivo - a free software for unfolding 3D models and papercrafting
    Copyright (C) 2015-2018 Oleksii Sierov (seriousalexej@gmail.com)
	
    Ivo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ivo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Ivo.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QOpenGLFramebufferObject>
#include <QEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <limits>
#include <cassert>
#include "mesh/mesh.h"
#include "interface/renwin2d.h"
#include "settings/settings.h"
#include "renderers/renderlegacy2d.h"
#include "interface/selectioninfo.h"
#include "interface/modes2D/mode2D.h"

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

CRenWin2D::CRenWin2D(QWidget *parent) :
    IRenWin(parent)
{
    m_cameraPosition = vec3(0.0f, 0.0f, 10.0f);
    m_w = m_h = 100.0f;
    setMouseTracking(true);

    m_editInfo.reset(new SEditInfo());
    Subscribe<CMesh::GroupStructureChanging>(&CRenWin2D::ClearSelection);
}

CRenWin2D::~CRenWin2D()
{
    makeCurrent();
    m_renderer.reset(nullptr);
    doneCurrent();
}

void CRenWin2D::ClearSelection()
{
    m_editInfo->selectionFilledOnSpot = false;
    m_editInfo->selectionOldPositions.clear();
    m_editInfo->selectionOldRotations.clear();
    m_editInfo->selectionLastRotations.clear();
    m_editInfo->selection.clear();
    update();
}

void CRenWin2D::SetMode(IMode2D* m)
{
    if(m_cameraMode == CAM_MODE)
        m_mode->MouseLBRelease();

    m_mode.reset(m);
    m_mode->m_editInfo = m_editInfo.get();
    m_editInfo->editMode = std::type_index(typeid(*m));
    SetCameraMode(CAM_STILL);
    update();
}

void CRenWin2D::SetCameraMode(ECameraMode m)
{
    m_cameraMode = m;
    m_editInfo->modeIsActive = (m == CAM_MODE);
}

void CRenWin2D::SetModel(CMesh *mdl)
{
    ClearSelection();
    m_model = mdl;
    m_editInfo->mesh = mdl;
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
    m_renderer->PreDraw();

    unsigned papHorizontal;
    unsigned papVertical;
    FillOccupiedSheetsSize(papHorizontal, papVertical);
    m_renderer->DrawPaperSheets(papHorizontal, papVertical);

    m_renderer->DrawScene();
    m_renderer->DrawSelection(*m_editInfo);
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

bool CRenWin2D::event(QEvent* e)
{
    static vec3 oldPos = m_cameraPosition;

    switch(e->type())
    {
        case QEvent::Wheel :
        {
            setFocus();
            QWheelEvent* we = static_cast<QWheelEvent*>(e);
            m_cameraPosition[2] = clamp(float(m_cameraPosition[2] + 0.01f*we->delta()), 0.1f, 1000000.0f);
            makeCurrent();
            RecalcProjection();
            doneCurrent();
            update();
            break;
        }
        case QEvent::MouseButtonPress :
        {
            setFocus();
            QMouseEvent *me = static_cast<QMouseEvent*>(e);

            if(m_cameraMode != CAM_STILL)
            {
                if(m_cameraMode == CAM_ZOOM && me->button() == Qt::LeftButton)
                    SetCameraMode(CAM_TRANSLATE);

                break;
            }

            m_editInfo->mousePressPointOrig = vec2(me->localPos().x(), me->localPos().y());
            m_editInfo->mousePressPoint = PointToWorldCoords(me->localPos());
            oldPos = m_cameraPosition;
            switch(me->button())
            {
                case Qt::MiddleButton :
                {
                    SetCameraMode(CAM_TRANSLATE);
                    break;
                }
                case Qt::RightButton :
                {
                    SetCameraMode(CAM_ZOOM);
                    break;
                }
                case Qt::LeftButton :
                {
                    SetCameraMode(CAM_MODE);
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
                ModeEnd();

            SetCameraMode(CAM_STILL);
            update();
            break;
        }
        case QEvent::MouseMove :
        {
            QMouseEvent* me = static_cast<QMouseEvent*>(e);
            QPointF newPos = me->pos();
            m_editInfo->mousePosition = PointToWorldCoords(newPos);
            switch(m_cameraMode)
            {
                case CAM_ZOOM :
                {
                    m_cameraPosition[2] = clamp(oldPos[2] - float(m_editInfo->mousePressPointOrig.y - newPos.ry())*0.1f, 0.1f, 1000000.0f);
                    makeCurrent();
                    RecalcProjection();
                    doneCurrent();
                    break;
                }
                case CAM_TRANSLATE :
                {
                    m_cameraPosition[0] = oldPos[0] + (newPos.rx() - m_editInfo->mousePressPointOrig.x)*0.0025f*m_cameraPosition[2];
                    m_cameraPosition[1] = oldPos[1] - (newPos.ry() - m_editInfo->mousePressPointOrig.y)*0.0025f*m_cameraPosition[2];
                    m_renderer->UpdateCameraPosition(m_cameraPosition);
                    break;
                }
                case CAM_MODE :
                {
                    ModeUpdate();
                    break;
                }
                default : break;
            }
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

vec2 CRenWin2D::PointToWorldCoords(const QPointF& pt) const
{
    float height = 2.0f * m_cameraPosition[2] * m_h/m_w;
    vec2 topLeftWorldCoords = vec2(-m_cameraPosition[0], -m_cameraPosition[1]);
    topLeftWorldCoords += vec2(-m_cameraPosition[2], height*0.5f);
    vec2 pointWorldCoords = vec2(+(pt.x()/m_w)*m_cameraPosition[2]*2.0f,
                                 -(pt.y()/m_h)*height);
    pointWorldCoords += topLeftWorldCoords;
    return pointWorldCoords;
}

void CRenWin2D::ModeLMB()
{
    if(!m_model || !m_mode)
        return;

    m_mode->m_active = true;
    m_mode->MouseLBPress();
    if(!m_mode->m_active)
        SetCameraMode(CAM_STILL);
}

void CRenWin2D::ModeUpdate()
{
    if(!m_model)
        return;

    m_mode->m_active = true;
    m_mode->MouseMove();
    if(!m_mode->m_active)
        SetCameraMode(CAM_STILL);
}

void CRenWin2D::ModeEnd()
{
    if(!m_model)
        return;

    m_mode->m_active = true;
    m_mode->MouseLBRelease();
    if(!m_mode->m_active)
        SetCameraMode(CAM_STILL);
    if(m_editInfo->selectionFilledOnSpot)
        ClearSelection();
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
        const float grpAABBh = grp.GetAABBHalfSide();
        highestY = max(highestY, grpPos.y + grpAABBh);
        lowestY  = min(lowestY,  grpPos.y - grpAABBh);
        highestX = max(highestX, grpPos.x + grpAABBh);
        lowestX  = min(lowestX,  grpPos.x - grpAABBh);
    }

    m_cameraPosition = vec3(-((highestX + lowestX) * 0.5f),
                            -((highestY + lowestY) * 0.5f),
                            max((highestY - lowestY) * 0.5f,(highestX - lowestX) * 0.5f) * m_w / m_h);

    makeCurrent();
    RecalcProjection();
    doneCurrent();
    update();
}
