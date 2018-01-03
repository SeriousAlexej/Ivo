#include "interface/selectioninfo.h"
#include "interface/renwin2dEditInfo.h"

using glm::vec2;

//------------------------------------------------------------------------
void CRenWin2D::MoveStart()
{
    const vec2 mouseWorldCoords = PointToWorldCoords(m_editInfo->mousePressPoint);
    CMesh::STriGroup* tGroup = m_model->GroupUnderCursor(mouseWorldCoords);
    m_editInfo->currGroup = tGroup;
    if(!tGroup)
    {
        m_cameraMode = CAM_STILL;
        return;
    }
    m_editInfo->fromCurrGroupCenter = mouseWorldCoords - tGroup->GetPosition();
    m_editInfo->currGroupOldPos = tGroup->GetPosition();
}

//------------------------------------------------------------------------
void CRenWin2D::MoveUpdate()
{
    const vec2 mNewPos = PointToWorldCoords(m_editInfo->mousePosition) - m_editInfo->fromCurrGroupCenter;
    if(m_editInfo->currGroup)
        m_editInfo->currGroup->SetPosition(mNewPos[0], mNewPos[1]);
}

//------------------------------------------------------------------------
void CRenWin2D::MoveEnd()
{
    CMesh::STriGroup& tGroup = *(m_editInfo->currGroup);
    m_model->NotifyGroupMovement(tGroup, m_editInfo->currGroupOldPos);
}
