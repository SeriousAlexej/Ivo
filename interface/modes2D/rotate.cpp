#include <cassert>
#include <glm/mat2x2.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include "interface/selectioninfo.h"
#include "interface/renwin2dEditInfo.h"
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

//------------------------------------------------------------------------
void CRenWin2D::RotateStart()
{
    const vec2 mouseWorldCoords = PointToWorldCoords(m_editInfo->mousePressPoint);
    m_model->GetStuffUnderCursor(mouseWorldCoords, m_editInfo->currTri, m_editInfo->currEdge);
    TryFillSelection(mouseWorldCoords);
    if(m_editInfo->selection.empty())
    {
        m_cameraMode = CAM_STILL;
        return;
    }

    if(m_editInfo->currTri)
    {
        CMesh::STriangle2D& trRef = *(m_editInfo->currTri);
        int currEdge = m_editInfo->currEdge;
        m_editInfo->currEdgeVec = normalize(trRef[(currEdge+1)%3] - trRef[currEdge]);
    }

    SAABBox2D bbox = m_editInfo->selection[0]->GetAABBox();
    m_editInfo->selectionOldRotations.clear();
    m_editInfo->selectionLastRotations.clear();
    m_editInfo->selectionOldPositions.clear();
    m_editInfo->selectionOldRotations.reserve(m_editInfo->selection.size());
    m_editInfo->selectionLastRotations.reserve(m_editInfo->selection.size());
    m_editInfo->selectionOldPositions.reserve(m_editInfo->selection.size());

    for(auto* grp : m_editInfo->selection)
    {
        bbox = bbox.Union(grp->GetAABBox());
        m_editInfo->selectionOldRotations.push_back(grp->GetRotation());
        m_editInfo->selectionLastRotations.push_back(grp->GetRotation());
        m_editInfo->selectionOldPositions.push_back(grp->GetPosition());
    }
    m_editInfo->rotationCenter = bbox.position;
}

//------------------------------------------------------------------------
void CRenWin2D::RotateUpdate()
{
    const vec2 startPos = PointToWorldCoords(m_editInfo->mousePressPoint) - m_editInfo->rotationCenter;
    const vec2 newPos = PointToWorldCoords(m_editInfo->mousePosition) - m_editInfo->rotationCenter;
    float angleRad = angleFromTo(startPos, newPos);
    float newAngle = degrees(angleRad);
    mat2 rotMx = rotation(angleRad);

    static const float snapDelta = 5.0f;
    if(m_editInfo->currTri)
    {
        const CMesh::STriangle2D& tri = *(m_editInfo->currTri);
        const vec2& triV1 = tri[m_editInfo->currEdge];
        const vec2& triV2 = tri[(m_editInfo->currEdge+1)%3];
        const vec2 edgeVec = triV2 - triV1;
        float angleOX = degrees(angleFromTo(vec2(1.0f, 0.0f), edgeVec));

        vec2 edgeVecRotated = rotMx * m_editInfo->currEdgeVec;
        float currAngleOX = degrees(angleFromTo(vec2(1.0f, 0.0f), edgeVecRotated));

        for(float snapAngle = -180.0f; snapAngle < 200.0f; snapAngle += 45.0f)
        {
            if(abs(snapAngle - currAngleOX) < snapDelta)
            {
                const auto* referenceGroup = m_editInfo->currTri->GetGroup();
                bool refFound = false;
                int i = 0;
                for(auto* grp : m_editInfo->selection)
                {
                    if(grp == referenceGroup)
                    {
                        refFound = true;
                        break;
                    }
                    i++;
                }
                assert(refFound);
                float refGroupLastRot = m_editInfo->selectionLastRotations[i];

                newAngle = referenceGroup->GetRotation() + snapAngle - angleOX - refGroupLastRot;
                angleRad = radians(newAngle);
                rotMx = rotation(angleRad);
                break;
            }
        }
    }

    int i = 0;
    for(auto* grp : m_editInfo->selection)
    {
        const vec2 grpShift = m_editInfo->selectionOldPositions[i] - m_editInfo->rotationCenter;
        const vec2 newPos = m_editInfo->rotationCenter + rotMx * grpShift;
        grp->SetPosition(newPos.x, newPos.y);
        grp->SetRotation(newAngle + m_editInfo->selectionLastRotations[i++]);
    }
}

//------------------------------------------------------------------------
void CRenWin2D::RotateEnd()
{
    m_model->NotifyGroupsTransformation(m_editInfo->selection,
                                        m_editInfo->selectionOldPositions,
                                        m_editInfo->selectionOldRotations);
    m_editInfo->currTri = nullptr;

    if(m_editInfo->selectionFilledOnSpot)
        ClearSelection();
}
