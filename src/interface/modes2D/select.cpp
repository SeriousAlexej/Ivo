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
#include <unordered_set>
#include <glm/common.hpp>
#include <QGuiApplication>
#include "interface/renwin2dEditInfo.h"
#include "geometric/aabbox.h"

using glm::max;
using glm::min;
using glm::vec2;
using TSelection = std::vector<CMesh::STriGroup*>;

static bool IsAdditive()
{
    return QGuiApplication::keyboardModifiers() & Qt::ControlModifier;
}

static bool IsSubtractive()
{
    return QGuiApplication::keyboardModifiers() & Qt::AltModifier;
}

static TSelection MergeSelection(const TSelection& s1, const TSelection& s2)
{
    std::unordered_set<CMesh::STriGroup*> uniqueSelection;
    uniqueSelection.insert(s1.begin(), s1.end());
    uniqueSelection.insert(s2.begin(), s2.end());
    return TSelection(uniqueSelection.begin(), uniqueSelection.end());
}

static TSelection SubtractSelection(const TSelection& s1, const TSelection& s2)
{
    std::unordered_set<CMesh::STriGroup*> subtracted;
    subtracted.insert(s1.begin(), s1.end());
    for(auto* grp : s2)
        subtracted.erase(grp);
    return TSelection(subtracted.begin(), subtracted.end());
}

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
//------------------------------------------------------------------------
void CRenWin2D::SelectStart()
{
    decltype(m_editInfo->selection) selectionBackup;
    const bool additive = IsAdditive();
    const bool subtractive = IsSubtractive();
    if(additive || subtractive)
        selectionBackup = m_editInfo->selection;

    m_editInfo->selection.clear();
    TryFillSelection(PointToWorldCoords(m_editInfo->mousePressPoint));

    if(additive)
        m_editInfo->selection = MergeSelection(m_editInfo->selection, selectionBackup);
    else if(subtractive)
        m_editInfo->selection = SubtractSelection(selectionBackup, m_editInfo->selection);
}

//------------------------------------------------------------------------
void CRenWin2D::SelectUpdate()
{
    const vec2 pos_1 = PointToWorldCoords(m_editInfo->mousePressPoint);
    const vec2 pos_2 = PointToWorldCoords(m_editInfo->mousePosition);
    const vec2 rb(max(pos_1.x, pos_2.x), min(pos_1.y, pos_2.y));
    const vec2 lt(min(pos_1.x, pos_2.x), max(pos_1.y, pos_2.y));
    const SAABBox2D selectionBox(rb, lt);

    auto newSelection = m_model->GetGroupsInRange(selectionBox);
    if(IsAdditive())
        m_editInfo->selection = MergeSelection(m_editInfo->selection, newSelection);
    else if(IsSubtractive())
        m_editInfo->selection = SubtractSelection(m_editInfo->selection, newSelection);
    else
        m_editInfo->selection = newSelection;
}

//------------------------------------------------------------------------
void CRenWin2D::SelectEnd()
{
    m_editInfo->selectionFilledOnSpot = false;
}
