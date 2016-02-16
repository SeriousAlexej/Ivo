#include <fstream>
#include <algorithm>
#include <list>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include "mesh/mesh.h"
#include "modelLoader/objParser.h"

CMesh::CMesh()
{
}

CMesh::~CMesh()
{
}

void CMesh::Clear()
{
    m_vertices.clear();
    m_normals.clear();
    m_uvCoords.clear();
    m_triangles.clear();
    m_flatNormals.clear();
    m_tri2D.clear();
    m_groups.clear();
}

bool CMesh::LoadMesh(std::string path)
{
    std::ifstream in;
    in.open(path.c_str(), std::ios::in);

    if(!in.is_open())
        return false;

    Clear();
    ObjParser p(in, &m_vertices, &m_normals, &m_uvCoords, &m_triangles);

    if(!p.load())
    {
        Clear();
        in.close();
        return false;
    }

    in.close();
    CalculateFlatNormals(); //this function goes first!
    FillAdjTri_Gen2DTri();
    GroupTriangles(70.0f);
    PackGroups();
    CalculateAABBox();
    return true;
}

void CMesh::FillAdjTri_Gen2DTri()
{
    STriangle2D dummy;
    for(int i=0; i<3; ++i) dummy.m_edges[i] = nullptr;
    m_tri2D.resize(m_triangles.size(), dummy);

    for(int i = m_triangles.size()-1; i>=0; --i)
    {
        const Triangle &t = m_triangles[i];
        const glm::vec3 *v1[3] = { &m_vertices[t.vertex[0][0]-1], &m_vertices[t.vertex[1][0]-1], &m_vertices[t.vertex[2][0]-1] };
        const glm::vec3 *n1[3] = { &m_normals[t.vertex[0][2]-1],  &m_normals[t.vertex[1][2]-1],  &m_normals[t.vertex[2][2]-1]  };

        float v1v2cos = glm::dot(*v1[2]-*v1[0], *v1[1]-*v1[0])/(glm::distance(*v1[2], *v1[0])*glm::distance(*v1[1], *v1[0]));
        m_tri2D[i].m_vtx[0] = glm::vec2(0.0f, 0.0f);
        m_tri2D[i].m_vtx[1] = glm::vec2(glm::sin(glm::acos(v1v2cos)), v1v2cos)*glm::distance(*v1[1], *v1[0]);
        m_tri2D[i].m_vtx[2] = glm::vec2(0.0f, glm::distance(*v1[0], *v1[2]));
        m_tri2D[i].m_angleOY[0] = glm::degrees(glm::acos(v1v2cos));
        m_tri2D[i].m_angleOY[1] = -glm::degrees(glm::acos(glm::dot(*v1[0]-*v1[2], *v1[1]-*v1[2])/(glm::distance(*v1[2],*v1[0])*glm::distance(*v1[2],*v1[1]))));
        m_tri2D[i].m_angleOY[2] = 180.0f;
        m_tri2D[i].Init();
        m_tri2D[i].m_id = i;

        for(int j = m_triangles.size()-1; j>=0; --j)
        {
            if(j == i) //triangle cannot be adjacent to itself :P
                continue;

            if(m_tri2D[i].m_edges[0] != nullptr &&
               m_tri2D[i].m_edges[1] != nullptr &&
               m_tri2D[i].m_edges[2] != nullptr)
                break;

            const Triangle &t2 = m_triangles[j];
            const glm::vec3 *v2[3] = { &m_vertices[t2.vertex[0][0]-1], &m_vertices[t2.vertex[1][0]-1], &m_vertices[t2.vertex[2][0]-1] };
            const glm::vec3 *n2[3] = { &m_normals[t2.vertex[0][2]-1], &m_normals[t2.vertex[1][2]-1], &m_normals[t2.vertex[2][2]-1]  };

            //9 cases edges of 2 triangles can be adjacent:
            for(int e1=0; e1<3; ++e1)
                if(m_tri2D[i].m_edges[e1] == nullptr)
                    for(int e2=0; e2<3; ++e2)
                        if(*v1[e1] == *v2[(e2+1)%3] && *v1[(e1+1)%3] == *v2[e2] && //vertices and
                           *n1[e1] == *n2[(e2+1)%3] && *n1[(e1+1)%3] == *n2[e2] && //normals are equal <=> triangles are adjacent
                           m_tri2D[j].m_edges[e2] == nullptr)
                            DetermineFoldParams(i, j, e1, e2);
        }
        for(int j=0; j<3; ++j)
        {
            if(m_tri2D[i].m_edges[j] == nullptr)
            {
                m_edges.push_back(SEdge());
                SEdge &edg = m_edges.back();
                m_tri2D[i].m_edges[j] = &edg;
                edg.m_left = &m_tri2D[i];
                edg.m_leftIndex = j;
                edg.m_rightIndex = -1;
                edg.m_right = nullptr;
                edg.m_angle = 0.0f;
                edg.m_snapped = false;
                edg.m_foldType = SEdge::FT_FLAT;
                edg.m_flapPosition = SEdge::FP_NONE;
            }
        }
    }
}

// i, j - indices of triangles in range [ 0, triangles.size() )
// e1, e2 - indices of edges in range [ 0, 2 ]
void CMesh::DetermineFoldParams(int i, int j, int e1, int e2)
{
    m_edges.push_back(SEdge());
    SEdge &edg = m_edges.back();
    edg.m_left = &m_tri2D[i];
    edg.m_right = &m_tri2D[j];
    edg.m_leftIndex = e1;
    edg.m_rightIndex = e2;
    edg.m_snapped = false;
    edg.m_flapPosition = SEdge::FP_LEFT;
    m_tri2D[i].m_edges[e1] = &edg;
    m_tri2D[j].m_edges[e2] = &edg;

    float angle = glm::degrees(glm::acos(glm::dot(m_flatNormals[i], m_flatNormals[j]))); //length of normal = 1, skip division...
    edg.m_angle = angle;

    glm::vec3 &v0 = m_vertices[m_triangles[i].vertex[0][0]-1];
    glm::vec3 &v1 = m_vertices[m_triangles[i].vertex[1][0]-1];
    glm::vec3 &up = m_flatNormals[i];
    glm::vec3 front = glm::normalize(v0 - v1);
    glm::vec3 right = glm::cross(front, up);
    glm::mat4 triBasis;
    triBasis[0] = glm::vec4(right[0], right[1], right[2], 0.0f);
    triBasis[1] = glm::vec4(up[0],    up[1],    up[2],    0.0f);
    triBasis[2] = glm::vec4(front[0], front[1], front[2], 0.0f);
    triBasis[3] = glm::vec4(v1[0],    v1[1],    v1[2],    1.0f);
    glm::vec3 *toCheck = nullptr;
    //pick vertex of second triangle, that does not belong to the edge between triangles i and j
    switch(e2)
    {
        case 0: //edge with vtx 1 and 2
            toCheck = &m_vertices[m_triangles[j].vertex[2][0]-1]; //choose 3
            break;
        case 1: //2 and 3
            toCheck = &m_vertices[m_triangles[j].vertex[0][0]-1]; //choose 1
            break;
        case 2: //3 and 1
            toCheck = &m_vertices[m_triangles[j].vertex[1][0]-1]; //choose 2
            break;
        default: exit(42);
    }
    float pHeight = ( glm::inverse(triBasis) * glm::vec4((*toCheck)[0], (*toCheck)[1], (*toCheck)[2], 1.0f) )[1];
    static const float epsilon = 0.0001f;
    //if third vertex from triangle j in basis of triangle i is above xz plane, then this is a valley-fold
    if(pHeight > epsilon)
    {
        edg.m_foldType = SEdge::FT_VALLEY;
    } else //if third vtx is below xz plane, it's a mountain-fold
    if(pHeight < -epsilon)
    {
        edg.m_foldType = SEdge::FT_MOUNTAIN;
    } else
    {
        edg.m_foldType = SEdge::FT_FLAT;
    }
}

void CMesh::CalculateFlatNormals()
{
    for(Triangle &t : m_triangles)
    {
        const glm::uvec3 &vertexInfo1 = t.vertex[0];
        const glm::uvec3 &vertexInfo2 = t.vertex[1];
        const glm::uvec3 &vertexInfo3 = t.vertex[2];

        const glm::vec3 &vertex1 = m_vertices[vertexInfo1[0]-1];
        const glm::vec3 &vertex2 = m_vertices[vertexInfo2[0]-1];
        const glm::vec3 &vertex3 = m_vertices[vertexInfo3[0]-1];

        glm::vec3 faceNormal = glm::normalize(glm::cross((vertex2-vertex1), (vertex3-vertex1)));
        m_flatNormals.push_back(faceNormal);
    }
}

void CMesh::GroupTriangles(float maxAngleDeg)
{
    for(int i=m_tri2D.size()-1; i>=0; --i)
    {
        if(m_tri2D[i].m_myGroup != nullptr)
            continue;
        //we found first ungrouped triangle! Create new group
        STriGroup t(this);
        m_groups.push_back(t);
        //list of candidates contains triangles that might get in group
        std::list<std::pair<int, int>> candidates;
        std::list<int> candNoRefls;
        candidates.push_front(std::make_pair(i, -1));
        candNoRefls.push_front(i);
        STriGroup &grp = m_groups.back();

        while(!candidates.empty())
        {
            std::pair<int, int> &c = candidates.front();

            //triangle is added if it does not overlap existing triangles in group
            if(grp.AddTriangle(&m_tri2D[c.first], (c.second > -1 ? &m_tri2D[c.second] : nullptr)) )
            {
                //if this triangle is added, then it's neighbours are potential candidates
                STriangle2D &tr = m_tri2D[c.first];
                STriangle2D *tr2 = nullptr;
                for(int n=0; n<3; ++n)
                {
                    if(!tr.m_edges[n]->HasTwoTriangles())
                        continue;
                    tr2 = (tr.m_edges[n]->m_left == &tr ?
                           tr.m_edges[n]->m_right :
                           tr.m_edges[n]->m_left);

                    if(tr2) //if edge n has neighbour...
                    if(tr.m_edges[n]->m_angle <= maxAngleDeg) //angle between neighbour is in valid range
                    if(tr2->m_myGroup == nullptr) //neighbour is groupless
                    //and not already in the list of candidates, then
                    if(std::find(candNoRefls.begin(), candNoRefls.end(), tr2->m_id) == candNoRefls.end())
                    {
                        candidates.push_back(std::make_pair(tr2->m_id, c.first)); //it's a candidate!
                        candNoRefls.push_back(tr2->m_id);
                    }
                }
            }
            //this candidate has been processed
            candidates.pop_front();
            candNoRefls.pop_front();
        }
        grp.CentrateOrigin();
    }
    UpdateGroupDepth();
}

void CMesh::PackGroups()
{
    struct SGroupBBox
    {
        glm::vec2 topLeft;
        glm::vec2 rightDown;
        glm::vec2 position;
        float area;
        float width;
        float height;
        STriGroup *grp;

        void ApplyPosition(glm::vec2 offset)
        {
            const float groupGap1x = 0.75f;//grp->aabbHSide*0.1f;
            position += offset;
            //because origin is in left down corner and TriGroup is initially at (0,0),
            //sub the vector to the leftmost and downmost position (coords of this vec are <= 0)
            position -= glm::vec2(topLeft[0], rightDown[1]);
            grp->SetPosition(position[0]+groupGap1x, position[1]+groupGap1x);
        }
        void FillArea()
        {
            const float groupGap2x = 1.5f;//grp->aabbHSide*0.2f;
            width = rightDown[0]-topLeft[0]+groupGap2x;
            height = topLeft[1]-rightDown[1]+groupGap2x;
            area = width*height;
        }
        bool Intersect(const SGroupBBox &other) const
        {
            const float &oX = other.position[0];
            const float &oY = other.position[1];
            const float &oWidth = other.width;
            const float &oHeight = other.height;
            //this is enough for FCNR algo
            return PointInside(oX, oY) ||
                   PointInside(oX+oWidth, oY) ||
                   PointInside(oX+oWidth, oY+oHeight) ||
                   PointInside(oX, oY+oHeight) ||
                   other.PointInside(position[0], position[1]) ||
                   other.PointInside(position[0]+width, position[1]) ||
                   other.PointInside(position[0]+width, position[1]+height) ||
                   other.PointInside(position[0], position[1]+height);
        }

    private:
        bool PointInside(float x, float y) const
        {
            return x >= position[0] && x <= position[0]+width &&
                   y >= position[1] && y <= position[1]+height;
        }
    };

    SGroupBBox dummy;
    dummy.topLeft = dummy.rightDown = dummy.position = glm::vec2(0.0f, 0.0f);
    std::vector<SGroupBBox> bboxes(m_groups.size(), dummy);
    float binWidth = 0.0f;
    float maxWidth = 0.0f;

    int i=0;
    for(auto &grp : m_groups)
    {
        SGroupBBox &bbox = bboxes[i];
        bbox.grp = &grp;
        bbox.topLeft = grp.m_toTopLeft;
        bbox.rightDown = grp.m_toRightDown;
        bbox.FillArea();
        binWidth += bbox.area;
        maxWidth = glm::max(bbox.width, maxWidth);
        ++i;
    }
    binWidth = glm::max(glm::sqrt(binWidth), maxWidth);

    std::sort(bboxes.begin(), bboxes.end(),
         [](const SGroupBBox &i, const SGroupBBox &j) { return i.height < j.height; } );

    //FCNR
    struct FCNR_level
    {
        std::list<SGroupBBox*> floor;
        std::list<SGroupBBox*> ceiling;
        float ceilHeight;
        float floorHeight;
    };

    std::list<FCNR_level> levels;
    for(int i=bboxes.size()-1; i>=0; --i)
    {
        SGroupBBox &bbx = bboxes[i];
        bool added = false;
        for(auto &lvl : levels)
        {
            SGroupBBox *rmost_fl = lvl.floor.back();
            float x = rmost_fl->position[0] + rmost_fl->width;

            if(binWidth - x >= bbx.width)
            {
                bbx.position = glm::vec2(x, lvl.floorHeight);
                bool intersects = false;
                for(auto &ceilBBX : lvl.ceiling)
                {
                    if(ceilBBX->Intersect(bbx))
                    {
                        intersects = true;
                        break;
                    }
                }
                if(!intersects)
                {
                    lvl.floor.push_back(&bbx);
                    added = true;
                    break;
                }
            }

            if(!lvl.ceiling.empty())
            {
                SGroupBBox *lmost_cl = lvl.ceiling.back();
                x = lmost_cl->position[0];
            } else {
                x = binWidth;
            }

            if(x >= bbx.width)
            {
                bbx.position = glm::vec2(x - bbx.width, lvl.floorHeight + lvl.ceilHeight - bbx.height);
                bool intersects = false;
                for(auto &floorBBX : lvl.floor)
                {
                    if(floorBBX->Intersect(bbx))
                    {
                        intersects = true;
                        break;
                    }
                }
                if(!intersects)
                {
                    lvl.ceiling.push_back(&bbx);
                    added = true;
                    break;
                }
            }
        }

        if(!added)
        {
            FCNR_level *prev = nullptr;
            if(!levels.empty())
                prev = &levels.back();
            levels.push_back(FCNR_level());
            FCNR_level &lvl = levels.back();

            lvl.ceilHeight = bbx.height;
            lvl.floorHeight = (prev ? prev->ceilHeight + prev->floorHeight : 0.0f);
            lvl.floor.push_front(&bbx);
            bbx.position = glm::vec2(0.0f, lvl.floorHeight);
        }
    }

    float binHeight = levels.back().floorHeight + levels.back().ceilHeight;

    for(SGroupBBox &b : bboxes)
        b.ApplyPosition(glm::vec2(-binWidth*0.5f, -binHeight*0.5f));
}

const CMesh::STriGroup* CMesh::GroupUnderCursor(glm::vec2 &curPos) const
{
    std::list<const STriGroup*> possibleGroups;
    for(auto &grp : m_groups)
    {
        float gLX = grp.m_position[0] - grp.m_aabbHSide;
        float gRX = grp.m_position[0] + grp.m_aabbHSide;
        float gTY = grp.m_position[1] + grp.m_aabbHSide;
        float gDY = grp.m_position[1] - grp.m_aabbHSide;
        if(curPos[0] >= gLX && curPos[0] <= gRX &&
           curPos[1] >= gDY && curPos[1] <= gTY)
            possibleGroups.push_back(&grp);
    }
    for(auto &grp : possibleGroups)
    {
        for(auto &tri : grp->m_tris)
        {
            if(tri->PointInside(curPos))
                return grp;
        }
    }
    return nullptr;
}

void CMesh::GetStuffUnderCursor(glm::vec2 &curPos, CMesh::STriangle2D *&tr, int &e) const
{
    std::list<const STriGroup*> possibleGroups;
    for(auto &grp : m_groups)
    {
        float gLX = grp.m_position[0] - grp.m_aabbHSide;
        float gRX = grp.m_position[0] + grp.m_aabbHSide;
        float gTY = grp.m_position[1] + grp.m_aabbHSide;
        float gDY = grp.m_position[1] - grp.m_aabbHSide;
        if(curPos[0] >= gLX && curPos[0] <= gRX &&
           curPos[1] >= gDY && curPos[1] <= gTY)
            possibleGroups.push_back(&grp);
    }
    struct SClosestEdge
    {
        CMesh::STriangle2D *t;
        int e;
        float score;
    };
    std::list<SClosestEdge> chart;

    for(auto &grp : possibleGroups)
    {
        for(auto &tri : grp->m_tris)
        {
            float score = 0.0f;

            for(int i=0; i<3; ++i)
            if(tri->PointIsNearEdge(curPos, i, score))
            {
                chart.push_back(SClosestEdge{tri, i, score});
            }
        }
    }
    if(chart.empty())
        return;
    chart.sort([](const SClosestEdge &i, const SClosestEdge &j) { return i.score < j.score; } );
    tr = chart.front().t;
    e = chart.front().e;
}

void CMesh::UpdateGroupDepth()
{
    float total = static_cast<float>(m_groups.size());
    float f = 1.0f;
    for(auto &g : m_groups)
    {
        g.m_depth = (f/total)*500.0f;
        f += 1.0f;
    }
    STriGroup::ms_depthStep = 500.0f/total;
}

void CMesh::CalculateAABBox()
{
    float lowestX  = 999999999999.0f,
          highestX =-999999999999.0f,
          lowestY  = 999999999999.0f,
          highestY =-999999999999.0f,
          lowestZ  = 999999999999.0f,
          highestZ =-999999999999.0f;
    for(const glm::vec3& v : m_vertices)
    {
        lowestX = glm::min(lowestX, v.x);
        highestX = glm::max(highestX, v.x);
        lowestY = glm::min(lowestY, v.y);
        highestY = glm::max(highestY, v.y);
        lowestZ = glm::min(lowestZ, v.z);
        highestZ = glm::max(highestZ, v.z);
    }
    m_aabbox[0] = glm::vec3(lowestX, lowestY, lowestZ);
    m_aabbox[1] = glm::vec3(lowestX, lowestY, highestZ);
    m_aabbox[2] = glm::vec3(highestX, lowestY, highestZ);
    m_aabbox[3] = glm::vec3(highestX, lowestY, lowestZ);
    m_aabbox[4] = glm::vec3(lowestX, highestY, lowestZ);
    m_aabbox[5] = glm::vec3(lowestX, highestY, highestZ);
    m_aabbox[6] = glm::vec3(highestX, highestY, highestZ);
    m_aabbox[7] = glm::vec3(highestX, highestY, lowestZ);
}
