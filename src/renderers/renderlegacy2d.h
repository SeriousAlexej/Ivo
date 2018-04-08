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
#ifndef RENDERLEGACY2D_H
#define RENDERLEGACY2D_H
#include <QOpenGLFunctions_2_0>
#include "renderbase2d.h"

class CRenderer2DLegacy : public IRenderer2D
{
public:
    CRenderer2DLegacy(QOpenGLFunctions_2_0& gl);
    virtual ~CRenderer2DLegacy();

    void    Init() override;
    void    ResizeView(int w, int h) override;

    void    PreDraw() const override;
    void    DrawScene() const override;
    void    DrawSelection(const SSelectionInfo& sinfo) const override;
    void    DrawPaperSheets(unsigned numHorizontal, unsigned numVertical) const override;

    void    RecalcProjection() override;

    QImage  DrawImageFromSheet(const glm::vec2 &pos) const override;

    void    ClearTextures() override;

private:
    void    DrawParts() const;
    void    DrawFlaps() const;
    void    DrawGroups() const;
    void    DrawEdges() const;
    void    RenderFlap(void *tr, int edge) const;
    void    RenderEdge(void *tr, int edge, int foldType) const;

    void    BindTexture(unsigned id) const;
    void    UnbindTexture() const;

    mutable int             m_boundTextureID = -1;
    QOpenGLFunctions_2_0&   m_gl;
};

#endif // RENDERLEGACY2D_H
