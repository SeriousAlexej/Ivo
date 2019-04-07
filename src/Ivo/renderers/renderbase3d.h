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
#ifndef RENDERBASE3D_H
#define RENDERBASE3D_H
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/matrix.hpp>
#include <QImage>
#include "abstractrenderer.h"

class CMesh;

class IRenderer3D : public IAbstractRenderer
{
public:
    IRenderer3D() = default;
    virtual ~IRenderer3D() = default;

    virtual void    SetModel(const CMesh* mdl) override;
    virtual void    Init() override = 0;
    virtual void    ResizeView(int w, int h, float fovy) = 0;
    virtual void    ToggleLighting(bool enable) = 0;
    virtual void    ToggleGrid(bool enable) = 0;

    virtual void    PreDraw() const override;
    virtual void    DrawScene() const override = 0;
    virtual void    PostDraw() const override;

    virtual void    UpdateViewMatrix(const glm::mat4& viewMatrix) = 0;
    virtual QImage  GetPickingTexture() const = 0;

protected:
    glm::mat4       m_viewMatrix = glm::mat4(1);
    glm::vec3       m_cameraPosition;
    bool            m_lighting = true;
    bool            m_grid = true;
};

#endif // RENDERBASE3D_H
