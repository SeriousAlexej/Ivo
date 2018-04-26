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
#ifndef RENWIN2D_H
#define RENWIN2D_H
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <QString>
#include <memory>
#include <cstdio>
#include <unordered_map>
#include <functional>
#include "interface/renwin.h"
#include "notification/subscriber.h"

class IRenderer2D;
class IMode2D;
struct SEditInfo;

class CRenWin2D : public IRenWin, public Subscriber
{
Q_OBJECT

public:
    explicit CRenWin2D(QWidget* parent = nullptr);
    virtual ~CRenWin2D();

    void         SetModel(CMesh* mdl) override final;
    void         SetMode(IMode2D* m);
    void         ExportSheets(const QString baseName);
    void         ZoomFit() override final;

public slots:
    void         LoadTexture(const QImage* img, unsigned index) override;
    void         ClearTextures() override;
    void         ClearSelection();

protected:
    virtual void initializeGL() override final;
    virtual void paintGL() override final;
    virtual void resizeGL(int w, int h) override final;
    virtual bool event(QEvent* e) override final;

private:
    enum ECameraMode
    {
        CAM_TRANSLATE,
        CAM_STILL,
        CAM_ZOOM,
        CAM_MODE
    };

    void         SetCameraMode(ECameraMode m);
    void         TryFillSelection(const glm::vec2& pos);
    void         RecalcProjection();
    void         ModeLMB();
    void         ModeUpdate();
    void         ModeEnd();
    glm::vec2    PointToWorldCoords(const QPointF& pt) const;
    void         FillOccupiedSheetsSize(unsigned& horizontal, unsigned& vertical) const;

private:
    ECameraMode                     m_cameraMode = CAM_STILL;
    glm::vec3                       m_cameraPosition; //3rd component - zoom coeff.
    float                           m_w;
    float                           m_h;
    std::unique_ptr<SEditInfo>      m_editInfo;
    std::unique_ptr<IRenderer2D>    m_renderer;
    std::unique_ptr<IMode2D>        m_mode;
};

#endif // RENWIN2D_H
