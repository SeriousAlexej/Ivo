#include "mesh/mesh.h"
#include "mesh/command.h"
#include "settings/settings.h"
#include "geometric/binPacking.h"
#include "geometric/obbox.h"

using glm::vec2;
using glm::max;
using glm::sqrt;

namespace
{
const float groupGap = 0.75f;

struct SGroupBBox : SAABBox2D
{
    vec2 grpCenter;
    CMesh::STriGroup* grp;

    vec2 GetFinalPosition() const
    {
        vec2 grpCenterOffset = grp->GetPosition() - grpCenter;
        return grpCenterOffset + position;
    }
};

struct SPaperDispencer
{
public:
    SPaperDispencer() : step(0), x(0), y(0)
    {
    }

    int GetX() const { return x; }
    int GetY() const { return y; }

    void NextSheet()
    {
        if(y < step)
            y++;
        else if(x > 0)
            x--;
        else
            NextStep();
    }

private:
    void NextStep()
    {
        step++;
        y = 0;
        x = step;
    }

    int step;
    int x;
    int y;
};

SOBBox GetGroupOBBox(const CMesh::STriGroup& group)
{
    const auto& tris = group.GetTriangles();
    std::vector<vec2> points;
    points.reserve(tris.size()*3);
    for(const auto* triangle : tris)
        for(int i=0; i<3; i++)
            points.push_back((*triangle)[i]);
    return GetMinOBBox(points);
}

} //namespace anonymous

void CMesh::PackGroups(bool undoable)
{
    CSettings& sett = CSettings::GetInstance();
    const float marginsH = static_cast<float>(sett.GetMarginsHorizontal())*0.1f;
    const float marginsV = static_cast<float>(sett.GetMarginsVertical())*0.1f;
    const float papWidth = static_cast<float>(sett.GetPaperWidth())*0.1f;
    const float papHeight = static_cast<float>(sett.GetPaperHeight())*0.1f;
    const float binWidth = papWidth - marginsH*2.0f;
    const float binHeight = papHeight - marginsV*2.0f;

    CIvoCommand* cmd = nullptr;
    CIvoCommand rotationCommand;
    if(undoable)
        cmd = new CIvoCommand();

    std::vector<std::shared_ptr<SAABBox2D>> bboxes;
    for(auto& grp : m_groups)
    {
        SOBBox groupOOBBox = GetGroupOBBox(grp);
        const float boxWidth = groupOOBBox.GetWidth();
        const float boxHeight = groupOOBBox.GetHeight();

        if((boxWidth > binWidth && boxWidth > binHeight) ||
           (boxHeight > binWidth && boxHeight > binHeight))
            continue;

        float rotationAngle = -groupOOBBox.GetRotation();
        if(boxWidth > binWidth || boxHeight > binHeight)
            rotationAngle += 90.0f;
        CAtomicCommand cmdRot(CT_ROTATE);
        cmdRot.SetTriangle(grp.m_tris.front());
        cmdRot.SetRotation(rotationAngle);
        cmdRot.Redo();

        if(grp.m_toRightDown.x - grp.m_toTopLeft.x + groupGap*2.0f > binWidth ||
           grp.m_toTopLeft.y - grp.m_toRightDown.y + groupGap*2.0f > binHeight)
        {
            cmdRot.Undo();
        }
        else if(undoable)
        {
            rotationCommand.AddAction(cmdRot);
        }

        SGroupBBox* bbox = new SGroupBBox();
        bbox->grp = &grp;
        bbox->width = grp.m_toRightDown.x - grp.m_toTopLeft.x + groupGap*2.0f;
        bbox->height = grp.m_toTopLeft.y - grp.m_toRightDown.y + groupGap*2.0f;
        bbox->grpCenter = (grp.m_toRightDown + grp.m_toTopLeft) * 0.5f;;
        bboxes.emplace_back(bbox);
    }

    if(undoable)
    {
        rotationCommand.undo();
        cmd->AddAction(std::move(rotationCommand));
    }

    unsigned prevSize = bboxes.size() + 1;
    SPaperDispencer paperDispencer;
    while(prevSize != bboxes.size())
    {
        prevSize = bboxes.size();
        std::vector<std::shared_ptr<SAABBox2D>> packed = BinPacking::PackFCNR(bboxes, binWidth, binHeight);

        for(auto& boxPtr : packed)
        {
            SGroupBBox& b = static_cast<SGroupBBox&>(*boxPtr);
            vec2 finalPos = b.GetFinalPosition();
            finalPos.x += paperDispencer.GetX() * papWidth + marginsH;
            finalPos.y -= (paperDispencer.GetY() + 1) * papHeight - marginsV;
            if(undoable)
            {
                CAtomicCommand cmdMov(CT_MOVE);
                cmdMov.SetTriangle(b.grp->m_tris.front());
                cmdMov.SetTranslation(finalPos - b.grp->GetPosition());
                cmd->AddAction(cmdMov);
            } else {
                b.grp->SetPosition(finalPos.x, finalPos.y);
            }
        }

        paperDispencer.NextSheet();
    }

    if(undoable)
        m_undoStack.push(cmd);
}
