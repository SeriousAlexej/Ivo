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
#ifndef MODE2D_H
#define MODE2D_H
#include <functional>
#include <vector>
#include "interface/editinfo2d.h"

class IMode2D
{
public:
    IMode2D() = default;
    virtual ~IMode2D() = default;

protected:
    void         Move();
    bool         LBPress();
    bool         MBPress();
    bool         RBPress();
    bool         BRelease();
    bool         Wheel(int delta);

    virtual bool MouseLBPress();
    virtual bool MouseLBRelease();
    virtual bool MouseMBPress();
    virtual bool MouseMBRelease();
    virtual bool MouseRBPress();
    virtual bool MouseRBRelease();
    virtual void MouseMove();
    virtual bool MouseWheel(int delta);

    void         TryFillSelection();
    void         Deactivate();
    SEditInfo&   EditInfo();

private:
    SEditInfo*                         m_editInfo = nullptr;
    bool                               m_passive = false;
    std::vector<std::function<bool()>> m_releaseCallbacks;

    friend class CRenWin2D;
};

#endif // MODE2D_H
