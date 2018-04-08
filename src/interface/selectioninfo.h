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
#ifndef SELECTION_INFO_H
#define SELECTION_INFO_H
#include "interface/renwin2d.h"
#include "mesh/mesh.h"

struct SSelectionInfo
{
    bool                        m_modeIsActive;
    glm::vec2                   m_mouseWorldPosStart;
    glm::vec2                   m_mouseWorldPos;
    CRenWin2D::EditMode         m_editMode;
    CMesh::STriangle2D*         m_triangle;
    int                         m_edge;
    std::vector
        <CMesh::STriGroup*>     m_selection;
};

#endif
