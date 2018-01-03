#include "interface/selectioninfo.h"
#include "interface/renwin2dEditInfo.h"

using glm::vec2;

//------------------------------------------------------------------------
void CRenWin2D::SnapStart()
{
    const vec2 mouseWorldCoords = PointToWorldCoords(m_editInfo->mousePressPoint);
    CMesh::STriangle2D* trUnderCursor = nullptr;
    int edgeUnderCursor = 0;
    m_model->GetStuffUnderCursor(mouseWorldCoords, trUnderCursor, edgeUnderCursor);
    if(trUnderCursor)
    {
        CMesh::STriGroup* grp = trUnderCursor->GetGroup();
        if(trUnderCursor->GetEdge(edgeUnderCursor)->IsSnapped())
        {
            grp->BreakEdge(trUnderCursor, edgeUnderCursor);
        } else {
            grp->JoinEdge(trUnderCursor, edgeUnderCursor);
        }
    }
    m_cameraMode = CAM_STILL;
}

//------------------------------------------------------------------------
void CRenWin2D::SnapUpdate()
{
}

//------------------------------------------------------------------------
void CRenWin2D::SnapEnd()
{
}
