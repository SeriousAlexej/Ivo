#ifndef RENWIN2D_EDIT_INFO
#define RENWIN2D_EDIT_INFO
#include <QPointF>
#include <glm/vec2.hpp>
#include "interface/renwin2d.h"
#include "mesh/mesh.h"

struct CRenWin2D::SEditInfo
{
    QPointF                        mousePressPoint;
    QPointF                        mousePosition;
    CRenWin2D::EditMode            editMode = CRenWin2D::EditMode::Move;
    CMesh::STriangle2D*            currTri = nullptr;
    int                            currEdge = -1;
    glm::vec2                      currEdgeVec;
    glm::vec2                      rotationCenter = glm::vec2(0.0f,0.0f);
    bool                           selectionFilledOnSpot = false;
    std::vector<glm::vec2>         selectionOldPositions;
    std::vector<float>             selectionOldRotations;
    std::vector<float>             selectionLastRotations;
    std::vector<CMesh::STriGroup*> selection;
};

#endif
