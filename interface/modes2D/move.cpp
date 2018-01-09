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
