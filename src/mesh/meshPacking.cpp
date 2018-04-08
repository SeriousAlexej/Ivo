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
const float groupGap = 0.5f;

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

SOBBox GetGroupOBBox(const CMesh::STriGroup& group, std::function<float(const SAABBox2D&)> price)
{
    const auto& tris = group.GetTriangles();
    std::vector<vec2> points;
    points.reserve(tris.size()*3);
    for(const auto* triangle : tris)
        for(int i=0; i<3; i++)
            points.push_back((*triangle)[i] - group.GetPosition()); //subtract position to prevent integer overflow in algorithm
    return GetMinOBBox(points, price);
}

} //namespace anonymous

bool CMesh::PackGroups(bool undoable)
{
    CSettings& sett = CSettings::GetInstance();
    const float marginsH = static_cast<float>(sett.GetMarginsHorizontal())*0.1f;
    const float marginsV = static_cast<float>(sett.GetMarginsVertical())*0.1f;
    const float papWidth = static_cast<float>(sett.GetPaperWidth())*0.1f;
    const float papHeight = static_cast<float>(sett.GetPaperHeight())*0.1f;
    const float binWidth = papWidth - marginsH*2.0f;
    const float binHeight = papHeight - marginsV*2.0f;

    bool allPacked = true;

    auto bboxPrice = [binWidth, binHeight](const SAABBox2D& b) -> float
    {
        if(b.width + groupGap*2.0f > binWidth || b.height + groupGap*2.0f > binHeight)
            return std::numeric_limits<float>::max();

        if(binWidth < binHeight)
            return b.width;
        return b.height;
    };

    std::unique_ptr<CIvoCommand> cmd(new CIvoCommand());
    CIvoCommand rotationCommand;

    std::vector<std::shared_ptr<SAABBox2D>> bboxes;
    for(auto& grp : m_groups)
    {
        const SOBBox groupOOBBox = GetGroupOBBox(grp, bboxPrice);

        float rotationAngle = -groupOOBBox.GetRotation();

        CAtomicCommand cmdRot(CT_ROTATE);
        cmdRot.SetTriangle(grp.m_tris.front());
        cmdRot.SetRotation(rotationAngle);
        cmdRot.Redo();

        const float boxWidth = grp.m_toRightDown.x - grp.m_toTopLeft.x + groupGap*2.0f;
        const float boxHeight = grp.m_toTopLeft.y - grp.m_toRightDown.y + groupGap*2.0f;
        if(boxWidth > binWidth || boxHeight > binHeight)
        {
            cmdRot.Undo();
            const SAABBox2D grpBBox = grp.GetAABBox();
            const glm::vec2 grpCenter(grp.m_toRightDown.x*0.5f + grp.m_toTopLeft.x*0.5f,
                                      grp.m_toTopLeft.y*0.5f   + grp.m_toRightDown.y*0.5f);
            const glm::vec2 grpPos = grp.GetPosition();
            const glm::vec2 centerOffset(grpPos.x - grpCenter.x, grpPos.y - grpCenter.y);
            CAtomicCommand cmdMoveAway(CT_MOVE);
            cmdMoveAway.SetTriangle(grp.m_tris.front());
            cmdMoveAway.SetTranslation(glm::vec2(-grpBBox.width*0.5f - groupGap, -grpBBox.height*0.5f) + centerOffset - grp.GetPosition());
            cmd->AddAction(cmdMoveAway);

            allPacked = false;
            continue;
        }

        rotationCommand.AddAction(cmdRot);

        SGroupBBox* bbox = new SGroupBBox();
        bbox->grp = &grp;
        bbox->width = boxWidth;
        bbox->height = boxHeight;
        bbox->grpCenter = (grp.m_toRightDown + grp.m_toTopLeft) * 0.5f;;
        bboxes.emplace_back(bbox);
    }

    rotationCommand.undo();
    cmd->AddAction(std::move(rotationCommand));

    std::size_t prevSize = bboxes.size() + 1;
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

            CAtomicCommand cmdMov(CT_MOVE);
            cmdMov.SetTriangle(b.grp->m_tris.front());
            cmdMov.SetTranslation(finalPos - b.grp->GetPosition());
            cmd->AddAction(cmdMov);
        }

        paperDispencer.NextSheet();
    }

    if(undoable)
    {
        m_undoStack.push(cmd.release());
    } else {
        cmd->redo();
        m_undoStack.clear();
    }

    return allPacked && bboxes.empty();
}
