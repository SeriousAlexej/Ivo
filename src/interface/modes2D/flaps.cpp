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
#include "interface/selectioninfo.h"
#include "interface/renwin2dEditInfo.h"

using glm::vec2;

//------------------------------------------------------------------------
void CRenWin2D::FlapsStart()
{
    const vec2 mouseWorldCoords = PointToWorldCoords(m_editInfo->mousePressPoint);
    CMesh::STriangle2D* trUnderCursor = nullptr;
    int edgeUnderCursor = 0;
    m_model->GetStuffUnderCursor(mouseWorldCoords, trUnderCursor, edgeUnderCursor);
    if(trUnderCursor)
    {
        trUnderCursor->GetEdge(edgeUnderCursor)->NextFlapPosition();
    }
    m_cameraMode = CAM_STILL;
}

//------------------------------------------------------------------------
void CRenWin2D::FlapsUpdate()
{
}

//------------------------------------------------------------------------
void CRenWin2D::FlapsEnd()
{
}
