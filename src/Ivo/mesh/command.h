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
#ifndef IVO_COMMAND_H
#define IVO_COMMAND_H
#include <QUndoCommand>
#include <glm/vec2.hpp>
#include <list>
#include "mesh.h"

enum ECommandType
{
    CT_ROTATE,
    CT_MOVE,
    CT_JOIN_GROUPS,
    CT_BREAK_GROUP,
    CT_SNAP_EDGE,
    CT_BREAK_EDGE,
    CT_SCALE
};

class CAtomicCommand
{
public:
    explicit CAtomicCommand(ECommandType actionType);

    void SetTriangle(CMesh::STriangle2D* tr);
    void SetEdge(int e);
    void SetTranslation(const glm::vec2& trans);
    void SetRotation(float rot);
    void SetScale(float sca);

    void Redo() const;
    void Undo() const;

private:
    glm::vec2           m_translation;
    float               m_rotation;
    float               m_scale;
    CMesh::STriangle2D* m_triangle;
    int                 m_edge;
    ECommandType        m_type;
};

class CIvoCommand : public QUndoCommand
{
public:
    void AddAction(const CAtomicCommand& action);
    void AddAction(CIvoCommand&& cmd);

    virtual void undo() override;
    virtual void redo() override;

private:
    std::list<CAtomicCommand> m_actions;
};

#endif // IVO_COMMAND_H
