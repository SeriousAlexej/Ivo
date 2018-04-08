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
#ifndef RENDERBASE2D_H
#define RENDERBASE2D_H
#include <QImage>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "abstractrenderer.h"

class CMesh;
struct SSelectionInfo;

class IRenderer2D : public IAbstractRenderer
{
public:
    IRenderer2D() = default;
    virtual ~IRenderer2D();

    virtual void    SetModel(const CMesh* mdl) override;
    virtual void    Init() override = 0;
    virtual void    ResizeView(int w, int h) = 0;

    virtual void    PreDraw() const override;
    virtual void    DrawScene() const override = 0;
    virtual void    DrawSelection(const SSelectionInfo& sinfo) const = 0;
    virtual void    DrawPaperSheets(unsigned numHorizontal, unsigned numVertical) const = 0;
    virtual void    PostDraw() const override;

    virtual void    UpdateCameraPosition(const glm::vec3& camPos);
    virtual void    RecalcProjection() = 0;

    virtual QImage  DrawImageFromSheet(const glm::vec2& pos) const = 0;

protected:
    void            CreateFoldTextures();

    glm::vec3       m_cameraPosition;
    mutable std::unique_ptr<QOpenGLTexture> m_texFolds;
};

#endif // RENDERBASE2D_H
