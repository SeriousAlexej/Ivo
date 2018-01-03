#ifndef RENWIN2D_EDIT_INFO
#define RENWIN2D_EDIT_INFO
#include <QPointF>
#include <glm/vec2.hpp>
#include "interface/renwin2d.h"
#include "mesh/mesh.h"

struct CRenWin2D::SEditInfo
{
    QPointF             mousePressPoint;
    CRenWin2D::EditMode editMode = CRenWin2D::EditMode::Move;
    CMesh::STriGroup*   currGroup = nullptr;
    CMesh::STriangle2D* currTri = nullptr;
    int                 currEdge = -1;
    glm::vec2           currEdgeVec;
    glm::vec2           currGroupOldPos = glm::vec2(0.0f, 0.0f);
    float               currGroupOldRot = 0.0f;
    glm::vec2           fromCurrGroupCenter = glm::vec2(0.0f,0.0f);
    float               currGroupLastRot = 0.0f;
    QPointF             mousePosition;
};

#endif
