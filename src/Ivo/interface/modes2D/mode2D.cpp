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
#include "interface/modes2D/mode2D.h"

void IMode2D::TryFillSelection()
{
    m_editInfo->selectionFilledOnSpot = false;
    if(!m_editInfo->selection.empty())
        return;

    CMesh::STriGroup* grp = m_editInfo->mesh->GroupUnderCursor(m_editInfo->mousePressPoint);
    if(grp)
    {
        m_editInfo->selection.push_back(grp);
        m_editInfo->selectionFilledOnSpot = true;
    }
}

bool IMode2D::BRelease()
{
    bool result = false;
    for(const auto& cb : m_releaseCallbacks)
        result = result || cb();
    m_moved = false;
    m_releaseCallbacks.clear();
    m_editInfo->modeIsActive = false;
    return result;
}

void IMode2D::Move()
{
    m_moved = true;
    MouseMove();
}

bool IMode2D::Moved() const
{
    return m_moved;
}

bool IMode2D::LBPress()
{
    m_moved = false;
    m_releaseCallbacks.emplace_back([this] { return MouseLBRelease(); });

    if(!m_passive)
        m_editInfo->modeIsActive = true;
    bool res = MouseLBPress();
    if(!m_passive)
        m_editInfo->modeIsActive = res;
    return res;
}

bool IMode2D::MBPress()
{
    m_moved = false;
    m_releaseCallbacks.emplace_back([this] { return MouseMBRelease(); });

    if(!m_passive)
        m_editInfo->modeIsActive = true;
    bool res = MouseMBPress();
    if(!m_passive)
        m_editInfo->modeIsActive = res;
    return res;
}

bool IMode2D::RBPress()
{
    m_moved = false;
    m_releaseCallbacks.emplace_back([this] { return MouseRBRelease(); });

    if(!m_passive)
        m_editInfo->modeIsActive = true;
    bool res = MouseRBPress();
    if(!m_passive)
        m_editInfo->modeIsActive = res;
    return res;
}

bool IMode2D::Wheel(int delta)
{
    return MouseWheel(delta);
}

void IMode2D::MouseMove()
{
}

bool IMode2D::MouseLBPress()
{
    return false;
}

bool IMode2D::MouseLBRelease()
{
    return false;
}

bool IMode2D::MouseMBPress()
{
    return false;
}

bool IMode2D::MouseMBRelease()
{
    return false;
}

bool IMode2D::MouseRBPress()
{
    return false;
}

bool IMode2D::MouseRBRelease()
{
    return false;
}

bool IMode2D::MouseWheel(int delta)
{
    (void)delta;
    return false;
}

void IMode2D::Deactivate()
{
    m_moved = false;
    m_editInfo->modeIsActive = false;
    m_releaseCallbacks.clear();
}

SEditInfo& IMode2D::EditInfo()
{
    return *m_editInfo;
}
