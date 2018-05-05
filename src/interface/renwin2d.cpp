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
#include <QMenu>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <limits>
#include <cassert>
#include "mesh/mesh.h"
#include "interface/renwin2d.h"
#include "settings/settings.h"
#include "renderers/renderlegacy2d.h"
#include "interface/editinfo2d.h"
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
    IRenWin(parent),
    m_showMenu(false),
    m_contextMenu(nullptr)
{
    m_w = m_h = 100.0f;
    setMouseTracking(true);

    m_editInfo.reset(new SEditInfo());
    m_editInfo->cameraPosition = vec3(0.0f, 0.0f, 10.0f);
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
    if(m_editInfo->modeIsActive && m_mode)
        m_mode->BRelease();

    m_mode.reset(m);
    m_mode->m_editInfo = m_editInfo.get();
    m_editInfo->editMode = typeid(*m);
    update();
}

void CRenWin2D::SetDefaultMode(IMode2D* m)
{
    m_defaultMode.reset(m);
    m_defaultMode->m_passive = true;
    m_defaultMode->m_editInfo = m_editInfo.get();
}

void CRenWin2D::SetModel(CMesh* mdl)
{
    ClearSelection();
    m_model = mdl;
    m_editInfo->mesh = mdl;
    m_renderer->SetModel(mdl);
    ZoomFit();
}

void CRenWin2D::SetContextMenu(QMenu* menu)
{
    m_contextMenu = menu;
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
    if(!m_model)
        return QWidget::event(e);

    const vec3 prevPos = m_editInfo->cameraPosition;

    switch(e->type())
    {
        case QEvent::Wheel :
        {
            setFocus();
            QWheelEvent* we = static_cast<QWheelEvent*>(e);
            if(!m_mode->Wheel(we->delta()))
                m_defaultMode->Wheel(we->delta());
            break;
        }
        case QEvent::MouseButtonPress :
        {
            setFocus();
            QMouseEvent* me = static_cast<QMouseEvent*>(e);

            m_editInfo->mousePressPointOrig = vec2(me->localPos().x(), me->localPos().y());
            m_editInfo->mousePressPoint = PointToWorldCoords(me->localPos());
            switch(me->button())
            {
                case Qt::MiddleButton :
                {
                    if(!m_mode->MBPress())
                        m_defaultMode->MBPress();
                    break;
                }
                case Qt::RightButton :
                {
                    m_showMenu = true;
                    if(!m_mode->RBPress())
                        m_defaultMode->RBPress();
                    break;
                }
                case Qt::LeftButton :
                {
                    if(!m_mode->LBPress())
                        m_defaultMode->LBPress();
                    break;
                }
                default : break;
            }
            break;
        }
        case QEvent::MouseButtonRelease :
        {
            if(!m_mode->BRelease() && !m_defaultMode->BRelease() && m_showMenu && m_contextMenu)
                m_contextMenu->exec(QCursor::pos());

            m_showMenu = false;

            if(m_editInfo->selectionFilledOnSpot)
                ClearSelection();
            break;
        }
        case QEvent::MouseMove :
        {
            m_showMenu = false;

            QMouseEvent* me = static_cast<QMouseEvent*>(e);
            const QPointF& newPos = me->localPos();
            m_editInfo->mousePositionOrig = vec2(newPos.x(), newPos.y());
            m_editInfo->mousePosition = PointToWorldCoords(newPos);

            if(m_editInfo->modeIsActive)
                m_mode->Move();
            else
                m_defaultMode->Move();
            break;
        }
        default : return QWidget::event(e);
    }

    if(prevPos != m_editInfo->cameraPosition)
    {
        makeCurrent();
        if(prevPos[2] == m_editInfo->cameraPosition[2])
            m_renderer->UpdateCameraPosition(m_editInfo->cameraPosition);
        else
            RecalcProjection();
        doneCurrent();
    }

    update();
    return true;
}

void CRenWin2D::RecalcProjection()
{
    m_renderer->UpdateCameraPosition(m_editInfo->cameraPosition);
    m_renderer->RecalcProjection();
}

vec2 CRenWin2D::PointToWorldCoords(const QPointF& pt) const
{
    float height = 2.0f * m_editInfo->cameraPosition[2] * m_h/m_w;
    vec2 topLeftWorldCoords = vec2(-m_editInfo->cameraPosition[0], -m_editInfo->cameraPosition[1]);
    topLeftWorldCoords += vec2(-m_editInfo->cameraPosition[2], height*0.5f);
    vec2 pointWorldCoords = vec2(+(pt.x()/m_w)*m_editInfo->cameraPosition[2]*2.0f,
                                 -(pt.y()/m_h)*height);
    pointWorldCoords += topLeftWorldCoords;
    return pointWorldCoords;
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

    m_editInfo->cameraPosition = vec3(-((highestX + lowestX) * 0.5f),
                                      -((highestY + lowestY) * 0.5f),
                                      max((highestY - lowestY) * 0.5f,(highestX - lowestX) * 0.5f) * m_w / m_h);

    makeCurrent();
    RecalcProjection();
    doneCurrent();
    update();
}
