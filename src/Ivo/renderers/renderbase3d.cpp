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
#include "renderbase3d.h"

void IRenderer3D::SetModel(const CMesh* mdl)
{
    IAbstractRenderer::SetModel(mdl);
}

void IRenderer3D::ToggleLighting(bool enable)
{
    m_lighting = enable;
}

void IRenderer3D::ToggleGrid(bool enable)
{
    m_grid = enable;
}

void IRenderer3D::PreDraw() const
{
    //nothing
}

void IRenderer3D::PostDraw() const
{
    //nothing
}
