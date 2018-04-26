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
#include <typeindex>
#include <glm/vec2.hpp>
#include "mesh/mesh.h"

struct SEditInfo
{
    SEditInfo() : editMode(std::type_index(typeid(nullptr))) {}

    CMesh*                         mesh;
    bool                           modeIsActive;
    glm::vec2                      mousePressPointOrig;
    glm::vec2                      mousePressPoint;
    glm::vec2                      mousePosition;
    std::type_index                editMode;
    CMesh::STriangle2D*            currTri = nullptr;
    int                            currEdge = -1;
    glm::vec2                      currEdgeVec;
    glm::vec2                      rotationCenter = glm::vec2(0.0f,0.0f);
    bool                           selectionFilledOnSpot = false;
    std::vector<glm::vec2>         selectionOldPositions;
    std::vector<float>             selectionOldRotations;
    std::vector<float>             selectionLastRotations;
    std::vector<CMesh::STriGroup*> selection;
};

#endif
