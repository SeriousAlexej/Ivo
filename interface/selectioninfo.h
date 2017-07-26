#ifndef SELECTION_INFO_H
#define SELECTION_INFO_H
#include "interface/renwin2d.h"
#include "mesh/mesh.h"

struct SSelectionInfo
{
    glm::vec2           m_mouseWorldPos;
    CRenWin2D::EditMode m_editMode;
    CMesh::STriGroup*   m_group;
    CMesh::STriangle2D* m_triangle;
    int                 m_edge;
};

#endif
