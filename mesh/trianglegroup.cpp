#include <algorithm>
#include <list>
#include <unordered_set>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include "mesh/mesh.h"

float CMesh::STriGroup::ms_depthStep = 1.0f;

CMesh::STriGroup::STriGroup(CMesh *m) :
    m_position(glm::vec2(0.0f,0.0f)),
    m_rotation(0.0f),
    m_matrix(1),
    m_msh(m)
{
}

bool CMesh::STriGroup::AddTriangle(STriangle2D* tr, STriangle2D* referal)
{
    if(referal == nullptr)
    {
        m_tris.push_front(tr);
        tr->m_myGroup = this;
        glm::mat3 id(1);
        tr->SetRelMx(id);
        for(int v=0; v<3; ++v)
        {
            const glm::vec2 &vert = tr->m_vtxRT[v];
            m_toTopLeft[0] = glm::min(m_toTopLeft[0], vert[0]);
            m_toTopLeft[1] = glm::max(m_toTopLeft[1], vert[1]);
            m_toRightDown[0] = glm::max(m_toRightDown[0], vert[0]);
            m_toRightDown[1] = glm::min(m_toRightDown[1], vert[1]);
        }
        return true;
    }
    //find adjacent edges
    int e1=-1, e2=-1;
    for(int i=0; i<3; ++i)
    {
        if(tr->m_edges[i]->m_left == referal ||
           tr->m_edges[i]->m_right == referal)
            e1 = i;
        if(referal->m_edges[i]->m_left == tr ||
           referal->m_edges[i]->m_right == tr)
            e2 = i;
    }
    assert(e1 > -1 && e2 > -1);
    STriangle2D backup = *tr;
    //rotate and translate tr to match referal's edge
    float trNewRotation = referal->m_rotation + 180.0f + tr->m_angleOY[e1] - referal->m_angleOY[e2];
    tr->SetRotation(trNewRotation);
    glm::vec2 trNewPosition = referal->m_position - tr->m_vtxR[e1] + referal->m_vtxR[(e2+1)%3];
    tr->SetPosition(trNewPosition);

    //now check if tr overlaps any triangle in group
    for(auto it=m_tris.begin(); it!=m_tris.end(); ++it)
    {
        STriangle2D *toCheck = *it;
        if(toCheck == referal)
            continue;
        if(toCheck->Intersect(*tr))
        {
            *tr = backup; //cancel changes
            return false;
        }
    }
    tr->m_edges[e1]->m_snapped = true;
    m_tris.push_front(tr);
    glm::mat3 id(1);
    tr->SetRelMx(id);
    tr->m_myGroup = this;
    for(int v=0; v<3; ++v)
    {
        const glm::vec2 &vert = tr->m_vtxRT[v];
        m_toTopLeft[0] = glm::min(m_toTopLeft[0], vert[0]);
        m_toTopLeft[1] = glm::max(m_toTopLeft[1], vert[1]);
        m_toRightDown[0] = glm::max(m_toRightDown[0], vert[0]);
        m_toRightDown[1] = glm::min(m_toRightDown[1], vert[1]);
    }
    return true;
}

void CMesh::STriGroup::SetRotation(float angle)
{
    m_rotation = angle;
    while(m_rotation >= 360.0f)
        m_rotation -= 360.0f;
    while(m_rotation < 0.0f)
        m_rotation += 360.0f;
    float rotRAD = glm::radians(m_rotation);
    m_matrix[0] = glm::vec3(glm::cos(rotRAD), glm::sin(rotRAD), 0.0f);
    m_matrix[1] = glm::vec3(-glm::sin(rotRAD), glm::cos(rotRAD), 0.0f);
    for(STriangle2D *t : m_tris)
    {
        t->GroupHasTransformed(m_matrix);
    }
}

void CMesh::STriGroup::SetPosition(float x, float y)
{
    m_matrix[2][0] = m_position[0] = x;
    m_matrix[2][1] = m_position[1] = y;
    for(STriangle2D *t : m_tris)
    {
        t->GroupHasTransformed(m_matrix);
    }
}

void CMesh::STriGroup::CentrateOrigin()
{
    m_position = glm::vec2((m_toRightDown[0]+m_toTopLeft[0])*0.5f, (m_toTopLeft[1]+m_toRightDown[1])*0.5f);
    m_toTopLeft -= m_position;
    m_toRightDown -= m_position;
    float hw = 0.5f*(m_toRightDown[0]-m_toTopLeft[0]);
    float hh = 0.5f*(m_toTopLeft[1]-m_toRightDown[1]);
    m_aabbHSide = glm::sqrt(hw*hw+hh*hh);

    float rotRAD = glm::radians(m_rotation);
    m_matrix[0] = glm::vec3(glm::cos(rotRAD), glm::sin(rotRAD), 0.0f);
    m_matrix[1] = glm::vec3(-glm::sin(rotRAD), glm::cos(rotRAD), 0.0f);
    m_matrix[2] = glm::vec3(m_position[0], m_position[1], 1.0f);
    glm::mat3 pinv = glm::inverse(m_matrix);
    for(STriangle2D *t : m_tris)
        t->SetRelMx(pinv);
}

void CMesh::STriGroup::JoinEdge(STriangle2D *tr, int e)
{
    assert(e >= 0 && e < 3 && tr);

    if(!tr->m_edges[e]->HasTwoTriangles()) return;
    //get triangle at the other side of edge 'e'
    STriangle2D *tr2 = (tr->m_edges[e]->m_left == tr ?
                       tr->m_edges[e]->m_right :
                       tr->m_edges[e]->m_left);
    if(!tr2) return;
    //get index of the other side of edge 'e'
    int e2 = (tr->m_edges[e]->m_left == tr ?
              tr->m_edges[e]->m_rightIndex :
              tr->m_edges[e]->m_leftIndex);//-1;
    assert(e2 > -1);
    if(std::find(m_tris.begin(), m_tris.end(), tr2) != m_tris.end())
        return; //can't join group to itself

    STriGroup *grp = tr2->m_myGroup;

    tr->m_edges[e]->m_snapped = true;

    float oldRot = m_rotation;
    SetRotation(0.0f);
    grp->SetRotation(0.0f);

    float newRotation = tr->m_rotation - tr->m_angleOY[e] + 180.0f - tr2->m_rotation + tr2->m_angleOY[e2];
    grp->SetRotation(newRotation);
    glm::vec2 newPos = tr->m_vtxRT[e] - tr2->m_vtxRT[(e2+1)%3] + grp->m_position;
    grp->SetPosition(newPos[0], newPos[1]);

    m_tris.insert(m_tris.end(), grp->m_tris.begin(), grp->m_tris.end());

    m_toTopLeft = glm::vec2(99999999999999.0f, -99999999999999.0f);
    m_toRightDown = glm::vec2(-99999999999999.0f, 99999999999999.0f);
    for(STriangle2D*& t : m_tris)
    {
        t->m_myGroup = this;
        for(int v=0; v<3; ++v)
        {
            const glm::vec2 &vert = t->m_vtxRT[v];
            m_toTopLeft[0] = glm::min(m_toTopLeft[0], vert[0]);
            m_toTopLeft[1] = glm::max(m_toTopLeft[1], vert[1]);
            m_toRightDown[0] = glm::max(m_toRightDown[0], vert[0]);
            m_toRightDown[1] = glm::min(m_toRightDown[1], vert[1]);
        }
    }
    CentrateOrigin();
    SetRotation(oldRot);

    for(auto it = m_msh->m_groups.begin(); it != m_msh->m_groups.end(); ++it)
    {
        if(&(*it) == grp)
        {
            m_msh->m_groups.erase(it);
            break;
        }
    }
    m_msh->UpdateGroupDepth();
}

void CMesh::STriGroup::BreakEdge(STriangle2D *tr, int e)
{
    assert(e >= 0 && e < 3 && tr);
    tr->m_edges[e]->m_snapped = false;

    if(!tr->m_edges[e]->HasTwoTriangles()) return;
    //get triangle at the other side of edge 'e'
    STriangle2D *tr2 = (tr->m_edges[e]->m_left == tr ?
                       tr->m_edges[e]->m_right :
                       tr->m_edges[e]->m_left);
    if(!tr2) return;
    //get index of the other side of edge 'e'
    int e2 = (tr->m_edges[e]->m_left == tr ?
              tr->m_edges[e]->m_rightIndex :
              tr->m_edges[e]->m_leftIndex);//-1;
    assert(e2 > -1);

    //all triangles, connected to tr without tr2
    std::list<STriangle2D*> trAndCompany;
    trAndCompany.push_back(tr);

    std::unordered_set<STriangle2D*> currLevel; //of breadth-first search
    for(int i=0; i<3; ++i)
    {
        if(i != e && tr->m_edges[i]->m_snapped && tr->m_edges[i]->HasTwoTriangles())
            currLevel.insert((tr->m_edges[i]->m_left==tr ?
                              tr->m_edges[i]->m_right :
                              tr->m_edges[i]->m_left));//&(msh->tri2D[trNeighbours[i]]));
    }

    //if cutOff == false, then we can reach tr2 from tr by series of edges without 'e', and we cannot split the group yet
    bool cutOff = true;
    while(!currLevel.empty())
    {
        std::unordered_set<STriangle2D*> nextLevel;
        for(STriangle2D* t : currLevel)
        {
            for(int i=0; i<3; ++i)
            {
                STriangle2D *nbs = nullptr;
                if(t->m_edges[i]->HasTwoTriangles())
                    nbs = (t->m_edges[i]->m_left == t ?
                           t->m_edges[i]->m_right :
                           t->m_edges[i]->m_left);
                if(nbs &&
                   t->m_edges[i]->m_snapped &&
                   currLevel.find(nbs) == currLevel.end() &&
                   std::find(trAndCompany.begin(), trAndCompany.end(), nbs) == trAndCompany.end())
                    nextLevel.insert(nbs);
            }
        }
        if(nextLevel.find(tr2) != nextLevel.end())
        {
            cutOff = false;
            break;
        }
        trAndCompany.insert(trAndCompany.begin(), currLevel.begin(), currLevel.end());
        currLevel = nextLevel;
    }

    if(cutOff)
    {
        glm::vec2 oldTR2V0 = tr2->m_vtxRT[0];
        m_msh->m_groups.push_back(STriGroup(m_msh));
        STriGroup &newGroup = m_msh->m_groups.back();

        newGroup.m_toTopLeft = glm::vec2(99999999999999.0f, -99999999999999.0f);
        newGroup.m_toRightDown = glm::vec2(-99999999999999.0f, 99999999999999.0f);
        for(STriangle2D*& t : m_tris)
        {
            if(std::find(trAndCompany.begin(), trAndCompany.end(), t) == trAndCompany.end())
            {
                newGroup.AddTriangle(t, nullptr);
            }
        }
        newGroup.CentrateOrigin();
        newGroup.SetPosition(newGroup.m_position.x, newGroup.m_position.y);
        glm::vec2 newPos = newGroup.m_position + tr->m_normR[e] + (oldTR2V0 - tr2->m_vtxRT[0]);
        newGroup.SetPosition(newPos.x, newPos.y);

        glm::vec2 oldTRV0 = tr->m_vtxRT[0];
        m_toTopLeft = glm::vec2(99999999999999.0f, -99999999999999.0f);
        m_toRightDown = glm::vec2(-99999999999999.0f, 99999999999999.0f);
        newPos = -tr->m_normR[e];
        m_tris.clear();
        for(STriangle2D*& t : trAndCompany)
            AddTriangle(t, nullptr);
        CentrateOrigin();
        SetPosition(newPos.x, newPos.y);
        newPos += m_position + (oldTRV0 - tr->m_vtxRT[0]);
        SetPosition(newPos.x, newPos.y);

        m_msh->UpdateGroupDepth();
        tr->m_edges[e]->m_flapPosition = SEdge::FP_LEFT;
    }
}

const float& CMesh::STriGroup::GetDepth() const
{
    return m_depth;
}

float CMesh::STriGroup::GetDepthStep()
{
    return ms_depthStep;
}

const float& CMesh::STriGroup::GetAABBHalfSide() const
{
    return m_aabbHSide;
}

const std::list<CMesh::STriangle2D*>& CMesh::STriGroup::GetTriangles() const
{
    return m_tris;
}
