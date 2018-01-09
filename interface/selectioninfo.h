#ifndef SELECTION_INFO_H
#define SELECTION_INFO_H
#include "interface/renwin2d.h"
#include "mesh/mesh.h"

struct SSelectionInfo
{
    bool                        m_modeIsActive;
    glm::vec2                   m_mouseWorldPosStart;
    glm::vec2                   m_mouseWorldPos;
    CRenWin2D::EditMode         m_editMode;
    CMesh::STriangle2D*         m_triangle;
    int                         m_edge;
    std::vector
        <CMesh::STriGroup*>     m_selection;
};

#endif
