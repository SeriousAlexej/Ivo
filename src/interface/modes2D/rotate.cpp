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
#include <glm/mat2x2.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include "interface/modes2D/rotate.h"
#include "geometric/aabbox.h"
#include "geometric/compgeom.h"

using glm::vec2;
using glm::mat2;
using glm::degrees;
using glm::radians;
using glm::clamp;
using glm::normalize;
using glm::dot;
using glm::length;
using glm::abs;
using glm::sin;
using glm::cos;
using glm::rotation;
using glm::angleFromTo;

void CModeRotate::MouseLBPress()
{
    const vec2& mouseWorldCoords = EditInfo().mousePressPoint;
    EditInfo().mesh->GetStuffUnderCursor(mouseWorldCoords, EditInfo().currTri, EditInfo().currEdge);
    TryFillSelection();
    if(EditInfo().selection.empty())
    {
        Deactivate();
        return;
    }

    if(EditInfo().currTri)
    {
        CMesh::STriangle2D& trRef = *(EditInfo().currTri);
        int currEdge = EditInfo().currEdge;
        EditInfo().currEdgeVec = normalize(trRef[(currEdge+1)%3] - trRef[currEdge]);
    }

    SAABBox2D bbox = EditInfo().selection[0]->GetAABBox();
    EditInfo().selectionOldRotations.clear();
    EditInfo().selectionLastRotations.clear();
    EditInfo().selectionOldPositions.clear();
    EditInfo().selectionOldRotations.reserve(EditInfo().selection.size());
    EditInfo().selectionLastRotations.reserve(EditInfo().selection.size());
    EditInfo().selectionOldPositions.reserve(EditInfo().selection.size());

    for(auto* grp : EditInfo().selection)
    {
        bbox = bbox.Union(grp->GetAABBox());
        EditInfo().selectionOldRotations.push_back(grp->GetRotation());
        EditInfo().selectionLastRotations.push_back(grp->GetRotation());
        EditInfo().selectionOldPositions.push_back(grp->GetPosition());
    }
    EditInfo().rotationCenter = bbox.position;
}

void CModeRotate::MouseMove()
{
    const vec2 startPos = EditInfo().mousePressPoint - EditInfo().rotationCenter;
    const vec2 newPos = EditInfo().mousePosition - EditInfo().rotationCenter;
    float angleRad = angleFromTo(startPos, newPos);
    float newAngle = degrees(angleRad);
    mat2 rotMx = rotation(angleRad);

    static const float snapDelta = 5.0f;
    if(EditInfo().currTri)
    {
        const CMesh::STriangle2D& tri = *(EditInfo().currTri);
        const vec2& triV1 = tri[EditInfo().currEdge];
        const vec2& triV2 = tri[(EditInfo().currEdge+1)%3];
        const vec2 edgeVec = triV2 - triV1;
        float angleOX = degrees(angleFromTo(vec2(1.0f, 0.0f), edgeVec));

        vec2 edgeVecRotated = rotMx * EditInfo().currEdgeVec;
        float currAngleOX = degrees(angleFromTo(vec2(1.0f, 0.0f), edgeVecRotated));

        for(float snapAngle = -180.0f; snapAngle < 200.0f; snapAngle += 45.0f)
        {
            if(abs(snapAngle - currAngleOX) < snapDelta)
            {
                const auto* referenceGroup = EditInfo().currTri->GetGroup();
                bool refFound = false;
                int i = 0;
                for(auto* grp : EditInfo().selection)
                {
                    if(grp == referenceGroup)
                    {
                        refFound = true;
                        break;
                    }
                    i++;
                }
                assert(refFound);
                float refGroupLastRot = EditInfo().selectionLastRotations[i];

                newAngle = referenceGroup->GetRotation() + snapAngle - angleOX - refGroupLastRot;
                angleRad = radians(newAngle);
                rotMx = rotation(angleRad);
                break;
            }
        }
    }

    int i = 0;
    for(auto* grp : EditInfo().selection)
    {
        const vec2 grpShift = EditInfo().selectionOldPositions[i] - EditInfo().rotationCenter;
        const vec2 newPos = EditInfo().rotationCenter + rotMx * grpShift;
        grp->SetPosition(newPos.x, newPos.y);
        grp->SetRotation(newAngle + EditInfo().selectionLastRotations[i++]);
    }
}

void CModeRotate::MouseLBRelease()
{
    EditInfo().mesh->NotifyGroupsTransformation(EditInfo().selection,
                                                EditInfo().selectionOldPositions,
                                                EditInfo().selectionOldRotations);
    EditInfo().currTri = nullptr;
}
