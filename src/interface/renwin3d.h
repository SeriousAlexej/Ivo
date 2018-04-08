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
#ifndef RENWIN3D_H
#define RENWIN3D_H
#include <QTimer>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/matrix.hpp>
#include <memory>
#include "renwin.h"
#include "notification/subscriber.h"

class CMesh;
class IRenderer3D;

class CRenWin3D : public IRenWin, public Subscriber
{
    Q_OBJECT

public:
    enum EditMode
    {
        EM_NONE,
        EM_POLYPAINT
    };

    explicit CRenWin3D(QWidget *parent = nullptr);
    virtual ~CRenWin3D();

    void         SetModel(CMesh *mdl) override final;
    void         ZoomFit() override final;
    void         SetEditMode(EditMode mode);

public slots:
    void         LoadTexture(const QImage *img, unsigned index) override;
    void         ClearTextures() override;

protected:
    virtual void initializeGL() override final;
    virtual void paintGL() override final;
    virtual void resizeGL(int w, int h) override final;
    virtual bool event(QEvent *e) override final;

private:
    void         UpdateViewAngles();
    void         UpdateViewMatrix();
    void         UpdatedSettings();

    enum
    { CAM_FLYOVER,
      CAM_TRANSLATE,
      CAM_ROTATE,
      CAM_STILL,
      CAM_ZOOM }        m_cameraMode = CAM_STILL;
    glm::vec3           m_cameraPosition = glm::vec3(0.0f,0.0f,0.0f);
    glm::vec2           m_cameraRotation = glm::vec2(0.0f,0.0f);
    glm::vec3           m_right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3           m_front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3           m_up = glm::vec3(0.0f, 1.0f, 0.0f);
    float               m_fovh = 70.0f;
    EditMode            m_editMode = EM_NONE;
    QPointF             m_mousePressPoint;
    unsigned            m_width = 800;
    unsigned            m_height = 600;
    QImage              m_pickingTexture;
    bool                m_pickTexValid = false;
    QTimer              m_updateTimer;
    std::unique_ptr
        <IRenderer3D>   m_renderer;
};

#endif // RENWIN3D_H
