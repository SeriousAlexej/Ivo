#include "command.h"
#include "mesh.h"

CAtomicCommand::CAtomicCommand(ECommandType actionType) :
    m_translation(0.0f, 0.0f),
    m_rotation(0.0f),
    m_scale(1.0f),
    m_triangle(nullptr),
    m_edge(-1),
    m_type(actionType)
{
}

void CAtomicCommand::SetTranslation(const glm::vec2 &trans)
{
    m_translation = trans;
}

void CAtomicCommand::SetEdge(int e)
{
    m_edge = e;
}

void CAtomicCommand::SetRotation(float rot)
{
    m_rotation = rot;
}

void CAtomicCommand::SetScale(float sca)
{
    m_scale = sca;
}

void CAtomicCommand::SetTriangle(void *tr)
{
    m_triangle = tr;
}

void CAtomicCommand::Redo() const
{
    if(!m_triangle) return;

    CMesh::STriangle2D* tr = (CMesh::STriangle2D*)m_triangle;
    CMesh::STriGroup* grp = tr->GetGroup();
    CMesh* msh = CMesh::GetMesh();

    switch(m_type)
    {
        case CT_SCALE :
        {
            msh->ApplyScale(m_scale);
            break;
        }
        case CT_ROTATE :
        {
            grp->SetRotation(grp->GetRotation() + m_rotation);
            break;
        }
        case CT_MOVE :
        {
            glm::vec2 newPos = grp->GetPosition() + m_translation;
            grp->SetPosition(newPos.x, newPos.y);
            break;
        }
        case CT_SNAP_EDGE :
        {
            if(m_edge < 0 || m_edge > 2) return;

            tr->GetEdge(m_edge)->SetSnapped(true);
            break;
        }
        case CT_BREAK_EDGE :
        {
            if(m_edge < 0 || m_edge > 2) return;

            tr->GetEdge(m_edge)->SetSnapped(false);
            break;
        }
        case CT_JOIN_GROUPS :
        {
            if(m_edge < 0 || m_edge > 2) return;

            grp->AttachGroup(tr->GetEdge(m_edge)->GetOtherTriangle(tr), tr->GetEdge(m_edge)->GetOtherTriIndex(tr));
            break;
        }
        case CT_BREAK_GROUP :
        {
            if(m_edge < 0 || m_edge > 2) return;

            grp->BreakGroup(tr->GetEdge(m_edge)->GetOtherTriangle(tr), tr->GetEdge(m_edge)->GetOtherTriIndex(tr));
            break;
        }
        default : assert(false);
    }
}

void CAtomicCommand::Undo() const
{
    if(!m_triangle) return;

    CMesh::STriangle2D* tr = (CMesh::STriangle2D*)m_triangle;
    CMesh::STriGroup* grp = tr->GetGroup();
    CMesh* msh = CMesh::GetMesh();

    switch(m_type)
    {
        case CT_SCALE :
        {
            msh->ApplyScale(1.0f / m_scale);
            break;
        }
        case CT_ROTATE :
        {
            grp->SetRotation(grp->GetRotation() - m_rotation);
            break;
        }
        case CT_MOVE :
        {
            glm::vec2 newPos = grp->GetPosition() - m_translation;
            grp->SetPosition(newPos.x, newPos.y);
            break;
        }
        case CT_SNAP_EDGE :
        {
            if(m_edge < 0 || m_edge > 2) return;

            tr->GetEdge(m_edge)->SetSnapped(false);
            break;
        }
        case CT_BREAK_EDGE :
        {
            if(m_edge < 0 || m_edge > 2) return;

            tr->GetEdge(m_edge)->SetSnapped(true);
            break;
        }
        case CT_JOIN_GROUPS :
        {
            if(m_edge < 0 || m_edge > 2) return;

            grp->BreakGroup(tr->GetEdge(m_edge)->GetOtherTriangle(tr), tr->GetEdge(m_edge)->GetOtherTriIndex(tr));
            break;
        }
        case CT_BREAK_GROUP :
        {
            if(m_edge < 0 || m_edge > 2) return;

            grp->AttachGroup(tr->GetEdge(m_edge)->GetOtherTriangle(tr), tr->GetEdge(m_edge)->GetOtherTriIndex(tr));
            break;
        }
        default : assert(false);
    }
}

void CIvoCommand::AddAction(const CAtomicCommand &action)
{
    m_actions.push_back(action);
}

void CIvoCommand::AddAction(CIvoCommand&& cmd)
{
    m_actions.insert(m_actions.end(), cmd.m_actions.begin(), cmd.m_actions.end());
    cmd.m_actions.clear();
}

void CIvoCommand::undo()
{
    for(auto it = m_actions.rbegin(); it != m_actions.rend(); ++it)
    {
        (*it).Undo();
    }
}

void CIvoCommand::redo()
{
    for(auto it = m_actions.begin(); it != m_actions.end(); ++it)
    {
        (*it).Redo();
    }
}
