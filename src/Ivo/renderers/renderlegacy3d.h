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
#ifndef RENDERLEGACY3D_H
#define RENDERLEGACY3D_H
#include <QOpenGLFunctions_2_0>
#include "renderbase3d.h"

class CMesh;

class CRenderer3DLegacy : public IRenderer3D
{
public:
    CRenderer3DLegacy(QOpenGLFunctions_2_0& gl);
    virtual ~CRenderer3DLegacy();

    void    Init() override;
    void    ResizeView(int w, int h, float fovy) override;
    void    ToggleLighting(bool enable) override;
    void    ToggleGrid(bool enable) override;

    void    DrawScene() const override;

    void    UpdateViewMatrix(const glm::mat4& viewMatrix) override;

    QImage  GetPickingTexture() const override;

    void    ClearTextures() override;

private:
    void    DrawModel() const;
    void    DrawBackground() const;
    void    DrawGrid() const;
    void    DrawAxis() const;

    void    BindTexture(unsigned id) const;
    void    UnbindTexture() const;

    mutable int             m_boundTextureID = -1;
    QOpenGLFunctions_2_0&   m_gl;
};

#endif // RENDERLEGACY3D_H
