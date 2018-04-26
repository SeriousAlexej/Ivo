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
#include "interface/modes2D/move.h"

using glm::vec2;

void CModeMove::MouseLBPress()
{
    TryFillSelection();
    if(EditInfo().selection.empty())
    {
        Deactivate();
        return;
    }
    EditInfo().selectionOldPositions.clear();
    EditInfo().selectionOldPositions.reserve(EditInfo().selection.size());
    for(auto* grp : EditInfo().selection)
        EditInfo().selectionOldPositions.push_back(grp->GetPosition());
}

void CModeMove::MouseMove()
{
    const vec2 newPosOffset = EditInfo().mousePosition - EditInfo().mousePressPoint;
    int i = 0;
    for(auto* grp : EditInfo().selection)
    {
        const vec2 newPos = EditInfo().selectionOldPositions[i++] + newPosOffset;
        grp->SetPosition(newPos.x, newPos.y);
    }
}

void CModeMove::MouseLBRelease()
{
    EditInfo().mesh->NotifyGroupsMovement(EditInfo().selection, EditInfo().selectionOldPositions);
}
