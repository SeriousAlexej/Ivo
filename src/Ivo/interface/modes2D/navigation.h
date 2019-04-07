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
#ifndef MODE_NAVIGATION_2D_H
#define MODE_NAVIGATION_2D_H
#include "interface/modes2D/mode2D.h"

class CModeNavigation2D : public IMode2D
{
protected:
    bool MouseRBPress() override;
    bool MouseRBRelease() override;
    bool MouseMBPress() override;
    bool MouseMBRelease() override;
    void MouseMove() override;
    bool MouseWheel(int delta) override;

private:
    enum class Navigation
    {
        Zoom,
        Pan,
        None
    };

    Navigation m_type = Navigation::None;
    glm::vec3  m_oldCameraPos;
};

#endif // MODE_NAVIGATION_2D_H
