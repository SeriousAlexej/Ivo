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
#include <cassert>
#include <glm/geometric.hpp>
#include "interface/modes2D/navigation.h"

bool CModeNavigation2D::MouseRBPress()
{
    if(m_type == Navigation::None)
    {
        m_oldCameraPos = EditInfo().cameraPosition;
        m_type = Navigation::Zoom;
    }
    return true;
}

bool CModeNavigation2D::MouseMBPress()
{
    if(m_type == Navigation::None)
    {
        m_oldCameraPos = EditInfo().cameraPosition;
        m_type = Navigation::Pan;
    }
    return true;
}

bool CModeNavigation2D::MouseMBRelease()
{
    m_type = Navigation::None;
    return Moved();
}

bool CModeNavigation2D::MouseRBRelease()
{
    m_type = Navigation::None;
    return Moved();
}

void CModeNavigation2D::MouseMove()
{
    switch(m_type)
    {
        case Navigation::Zoom:
            EditInfo().cameraPosition[2] =
                    glm::clamp(m_oldCameraPos[2] - (EditInfo().mousePressPointOrig.y - EditInfo().mousePositionOrig.y)*0.1f, 0.1f, 1000000.0f);
            break;

        case Navigation::Pan:
            EditInfo().cameraPosition[0] = m_oldCameraPos[0] + (EditInfo().mousePositionOrig.x - EditInfo().mousePressPointOrig.x)*0.0025f*EditInfo().cameraPosition[2];
            EditInfo().cameraPosition[1] = m_oldCameraPos[1] - (EditInfo().mousePositionOrig.y - EditInfo().mousePressPointOrig.y)*0.0025f*EditInfo().cameraPosition[2];
            break;

        case Navigation::None:
            break;

        default:
            assert(false);
    }
}

bool CModeNavigation2D::MouseWheel(int delta)
{
    EditInfo().cameraPosition[2] = glm::clamp(float(EditInfo().cameraPosition[2] + 0.01f*delta), 0.1f, 1000000.0f);
    return true;
}
