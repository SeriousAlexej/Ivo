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
