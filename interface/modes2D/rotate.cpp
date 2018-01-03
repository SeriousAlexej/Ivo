#include <glm/mat2x2.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include "interface/selectioninfo.h"
#include "interface/renwin2dEditInfo.h"

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

//------------------------------------------------------------------------
void CRenWin2D::RotateStart()
{
    const vec2 mouseWorldCoords = PointToWorldCoords(m_editInfo->mousePressPoint);
    CMesh::STriangle2D* trUnderCursor = nullptr;
    m_model->GetStuffUnderCursor(mouseWorldCoords, trUnderCursor, m_editInfo->currEdge);

    CMesh::STriGroup* tGroup = m_model->GroupUnderCursor(mouseWorldCoords);
    m_editInfo->currGroup = tGroup;
    if(!tGroup)
    {
        m_cameraMode = CAM_STILL;
        return;
    }
    m_editInfo->currTri = trUnderCursor;
    if(trUnderCursor)
    {
        CMesh::STriangle2D& trRef = *trUnderCursor;
        m_editInfo->currEdgeVec = normalize(trRef[(m_editInfo->currEdge+1)%3] - trRef[m_editInfo->currEdge]);
    }
    m_editInfo->fromCurrGroupCenter = mouseWorldCoords - tGroup->GetPosition();
    m_editInfo->currGroupLastRot = tGroup->GetRotation();
    m_editInfo->currGroupOldRot = tGroup->GetRotation();
}

//------------------------------------------------------------------------
void CRenWin2D::RotateUpdate()
{
    if(!m_editInfo->currGroup)
        return;
    CMesh::STriGroup* tGroup = m_editInfo->currGroup;

    vec2 mNewPos = PointToWorldCoords(m_editInfo->mousePosition) - tGroup->GetPosition();
    float newAngle = dot(mNewPos, m_editInfo->fromCurrGroupCenter) / (length(mNewPos) * length(m_editInfo->fromCurrGroupCenter));
    newAngle = degrees(acos(newAngle));

    vec2& vA = m_editInfo->fromCurrGroupCenter;
    vec2& vB = mNewPos;

    if(vA[0]*vB[1] - vB[0]*vA[1] < 0.0f)
    {
        newAngle *= -1.0f;
    }

    static const float snapDelta = 5.0f;
    if(m_editInfo->currTri)
    {
        const CMesh::STriangle2D& tri = *(m_editInfo->currTri);
        const vec2& triV1 = tri[m_editInfo->currEdge];
        const vec2& triV2 = tri[(m_editInfo->currEdge+1)%3];
        vec2 edgeVec = normalize(triV2 - triV1);
        float angleOX = acos(clamp(edgeVec.x, -1.0f, 1.0f));
        if(edgeVec.y < 0.0f)
        {
            angleOX *= -1.0f;
        }
        angleOX = degrees(angleOX);

        const float angleRad = radians(newAngle);
        const mat2 rotMx(vec2(cos(angleRad), sin(angleRad)),
                         vec2(-sin(angleRad), cos(angleRad)));
        vec2 edgeVecRotated = rotMx * m_editInfo->currEdgeVec;
        float currAngleOX = acos(clamp(edgeVecRotated.x, -1.0f, 1.0f));
        if(edgeVecRotated.y < 0.0f)
        {
            currAngleOX *= -1.0f;
        }
        currAngleOX = degrees(currAngleOX);
        for(float snapAngle = -180.0f; snapAngle < 200.0f; snapAngle += 45.0f)
        {
            if(abs(snapAngle - currAngleOX) < snapDelta)
            {
                newAngle = tGroup->GetRotation() + snapAngle - angleOX - m_editInfo->currGroupLastRot;
                break;
            }
        }
    }

    tGroup->SetRotation(newAngle + m_editInfo->currGroupLastRot);
}

//------------------------------------------------------------------------
void CRenWin2D::RotateEnd()
{
    CMesh::STriGroup& tGroup = *(m_editInfo->currGroup);
    m_model->NotifyGroupRotation(tGroup, m_editInfo->currGroupOldRot);
    m_editInfo->currTri = nullptr;
}
