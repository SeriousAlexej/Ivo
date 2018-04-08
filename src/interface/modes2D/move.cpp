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
#include <cassert>
#include "interface/selectioninfo.h"
#include "interface/renwin2dEditInfo.h"

using glm::vec2;

//------------------------------------------------------------------------
void CRenWin2D::MoveStart()
{
    TryFillSelection(PointToWorldCoords(m_editInfo->mousePressPoint));
    if(m_editInfo->selection.empty())
    {
        m_cameraMode = CAM_STILL;
        return;
    }
    m_editInfo->selectionOldPositions.clear();
    m_editInfo->selectionOldPositions.reserve(m_editInfo->selection.size());
    for(auto* grp : m_editInfo->selection)
        m_editInfo->selectionOldPositions.push_back(grp->GetPosition());
}

//------------------------------------------------------------------------
void CRenWin2D::MoveUpdate()
{
    const vec2 newPosOffset = PointToWorldCoords(m_editInfo->mousePosition)
                              -
                              PointToWorldCoords(m_editInfo->mousePressPoint);
    int i = 0;
    for(auto* grp : m_editInfo->selection)
    {
        const vec2 newPos = m_editInfo->selectionOldPositions[i++] + newPosOffset;
        grp->SetPosition(newPos.x, newPos.y);
    }
}

//------------------------------------------------------------------------
void CRenWin2D::MoveEnd()
{
    m_model->NotifyGroupsMovement(m_editInfo->selection, m_editInfo->selectionOldPositions);

    if(m_editInfo->selectionFilledOnSpot)
        ClearSelection();
}
