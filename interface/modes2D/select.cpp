#include <glm/common.hpp>
#include "interface/renwin2dEditInfo.h"
#include "geometric/aabbox.h"

using glm::max;
using glm::min;
using glm::vec2;

void CRenWin2D::TryFillSelection(const glm::vec2& pos)
{
    m_editInfo->selectionFilledOnSpot = false;
    if(!m_editInfo->selection.empty())
        return;

    CMesh::STriGroup* grp = m_model->GroupUnderCursor(pos);
    if(grp)
    {
        m_editInfo->selection.push_back(grp);
        m_editInfo->selectionFilledOnSpot = true;
    }
}

//------------------------------------------------------------------------
void CRenWin2D::SelectStart()
{
    m_editInfo->selection.clear();
    TryFillSelection(PointToWorldCoords(m_editInfo->mousePressPoint));
}

//------------------------------------------------------------------------
void CRenWin2D::SelectUpdate()
{
    const vec2 pos_1 = PointToWorldCoords(m_editInfo->mousePressPoint);
    const vec2 pos_2 = PointToWorldCoords(m_editInfo->mousePosition);
    const vec2 rb(max(pos_1.x, pos_2.x), min(pos_1.y, pos_2.y));
    const vec2 lt(min(pos_1.x, pos_2.x), max(pos_1.y, pos_2.y));
    const SAABBox2D selectionBox(rb, lt);
    m_editInfo->selection = m_model->GetGroupsInRange(selectionBox);
}

//------------------------------------------------------------------------
void CRenWin2D::SelectEnd()
{
}
