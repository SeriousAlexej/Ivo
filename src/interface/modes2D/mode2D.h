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
#include "interface/selectioninfo.h"

class IMode2D
{
public:
    IMode2D() = default;
    virtual ~IMode2D() = default;

protected:
    virtual void MouseMove() = 0;
    virtual void MouseLBPress() = 0;
    virtual void MouseLBRelease() = 0;

    void         TryFillSelection();
    void         Deactivate();
    SEditInfo&   EditInfo();

private:
    SEditInfo* m_editInfo = nullptr;
    bool       m_active = true;

    friend class CRenWin2D;
};

#endif // MODE2D_H
