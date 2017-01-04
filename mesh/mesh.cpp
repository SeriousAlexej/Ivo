#include <fstream>
#include <algorithm>
#include <list>
#include <unordered_map>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include "mesh/mesh.h"
#include "mesh/command.h"
#include "settings/settings.h"
#include "io/saferead.h"

CMesh* CMesh::g_Mesh = nullptr;

CMesh::CMesh()
{
    m_undoStack.setUndoLimit(100);
}

CMesh::~CMesh()
{
}

bool CMesh::IsModified() const
{
    return m_undoStack.canRedo() || m_undoStack.canUndo();
}

void CMesh::Clear()
{
    m_undoStack.clear();
    m_vertices.clear();
    m_normals.clear();
    m_uvCoords.clear();
    m_triangles.clear();
    m_flatNormals.clear();
    m_tri2D.clear();
    m_groups.clear();
    m_materials.clear();
}

void CMesh::AddMeshesFromAIScene(const aiScene* scene, const aiNode* node)
{
    static unsigned unnamedMatIndex = 1u;
    for(unsigned n=0; n<node->mNumMeshes; ++n)
    {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[n]];

        unsigned matIndex = mesh->mMaterialIndex;
        if(m_materials.find(matIndex) == m_materials.end())
        {
            aiMaterial& mat = *(scene->mMaterials[matIndex]);
            aiString matName;
            mat.Get(AI_MATKEY_NAME, matName);
            std::string matNameStdStr = matName.C_Str();
            if(matNameStdStr.empty())
            {
                matNameStdStr = std::string("<unnamed_material_") + std::to_string(unnamedMatIndex++) + ">";
            }
            m_materials[matIndex] = matNameStdStr;
        }

        std::unordered_map<unsigned, unsigned> indicesRemap;

        for(unsigned f=0; f<mesh->mNumFaces; ++f)
        {
            const aiFace* face = &mesh->mFaces[f];

            assert(face->mNumIndices == 3);

            glm::uvec4 tr;
            for(int i=0; i<3; ++i)
            {
                unsigned newIndex = m_uvCoords.size();
                unsigned vertexIndex = face->mIndices[i];

                if(indicesRemap.find(vertexIndex) == indicesRemap.end())
                {
                    if(mesh->HasTextureCoords(0))
                    {
                        m_uvCoords.push_back(glm::vec2(mesh->mTextureCoords[0][vertexIndex].x, mesh->mTextureCoords[0][vertexIndex].y));
                    } else {
                        m_uvCoords.push_back(glm::vec2(0.0f, 0.0f));
                    }
                    m_normals.push_back(glm::vec3(mesh->mNormals[vertexIndex].x, mesh->mNormals[vertexIndex].y, mesh->mNormals[vertexIndex].z));
                    m_vertices.push_back(glm::vec3(mesh->mVertices[vertexIndex].x, mesh->mVertices[vertexIndex].y, mesh->mVertices[vertexIndex].z));

                    indicesRemap[vertexIndex] = newIndex;
                } else {
                    newIndex = indicesRemap[vertexIndex];
                }

                tr[i] = newIndex;
            }
            tr[3] = matIndex;
            m_triangles.push_back(tr);
        }
    }

    for(unsigned n=0; n<node->mNumChildren; ++n)
    {
        AddMeshesFromAIScene(scene, node->mChildren[n]);
    }
    unnamedMatIndex = 1u;
}

void CMesh::LoadMesh(const std::string& path)
{
    Clear();

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path,   aiProcess_JoinIdenticalVertices |
                                                     aiProcess_Triangulate |
                                                     aiProcess_GenSmoothNormals |
                                                     aiProcess_PreTransformVertices |
                                                     aiProcess_GenUVCoords |
                                                     aiProcess_FlipUVs);
    if(!scene)
    {
        throw std::logic_error(importer.GetErrorString());
    }

    AddMeshesFromAIScene(scene, scene->mRootNode);

    if(m_vertices.size() == 0)
    {
        throw std::logic_error("File contains no 3D geometry");
    }

    CalculateFlatNormals(); //this function goes first!
    FillAdjTri_Gen2DTri();
    GroupTriangles((float)CSettings::GetInstance().GetDetachAngle());
    PackGroups(false);
    CalculateAABBox();
    g_Mesh = this;
}

void CMesh::LoadFromPDO(const std::vector<PDO_Face>&                  faces,
                        const std::vector<std::unique_ptr<PDO_Edge>>& edges,
                        const std::vector<glm::vec3>&                 vertices3D,
                        const std::unordered_map<unsigned, PDO_Part>& parts)
{
    Clear();

    m_tri2D.resize(faces.size());
    m_triangles.resize(faces.size());

    for(unsigned f=0; f<faces.size(); f++)
    {
        const PDO_Face& face = faces[f];
        STriangle2D&    tr2D = m_tri2D[f];
        glm::uvec4&     tr = m_triangles[f];

        glm::vec2 averageTri2DPos(0.0f, 0.0f);
        for(int i=0; i<3; ++i)
        {
            const PDO_2DVertex& vertex = face.vertices[i];

            unsigned newIndex = m_uvCoords.size();

            m_uvCoords.push_back(vertex.uv);
            m_normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
            m_vertices.push_back(vertices3D[vertex.index3Dvert]);

            tr[i] = newIndex;

            averageTri2DPos += vertex.pos;
        }
        tr[3] = face.matIndex;

        averageTri2DPos /= 3.0f;

        tr2D.m_id = f;
        tr2D.m_position = averageTri2DPos;
        tr2D.m_rotation = 0.0f;
        tr2D.m_myGroup = nullptr;
        tr2D.m_relativeMx = glm::mat3(1);
        for(int i=0; i<3; i++)
        {
            tr2D.m_edges[i] = nullptr;
            tr2D.m_flapSharp[i] = false;
            tr2D.m_vtx[i] = tr2D.m_vtxR[i] = face.vertices[i].pos - averageTri2DPos;
            tr2D.m_vtxRT[i] = face.vertices[i].pos;
        }
        for(int i=0; i<3; i++)
        {
            tr2D.m_edgeLen[i] = glm::distance(tr2D.m_vtx[i], tr2D.m_vtx[(i+1)%3]);

            glm::vec2 vecAngle = tr2D.m_vtx[(i+1)%3] - tr2D.m_vtx[i];
            float sgnX = (vecAngle.x < 0.0f ? -1.0f : 1.0f);
            tr2D.m_angleOY[i] = sgnX * glm::degrees(glm::acos(glm::clamp(vecAngle.y / glm::length(vecAngle), -1.0f, 1.0f)));
        }
        tr2D.ComputeNormals();
    }

    CalculateFlatNormals();

    for(const std::unique_ptr<PDO_Edge>& e : edges)
    {
        const PDO_Edge& edge = *e;

        m_edges.push_back(SEdge());
        SEdge &edg = m_edges.back();

        edg.m_left = &m_tri2D[edge.face1ID];
        for(int i=0; i<3; i++)
        {
            if(edge.vtx1ID == faces[edge.face1ID].vertices[i].index3Dvert)
            {
                edg.m_leftIndex = i;
                break;
            }
        }
        edg.m_left->m_edges[edg.m_leftIndex] = &edg;

        if(edge.face2ID >= 0)
        {
            edg.m_right = &m_tri2D[edge.face2ID];
            for(int i=0; i<3; i++)
            {
                if(edge.vtx2ID == faces[edge.face2ID].vertices[i].index3Dvert)
                {
                    edg.m_rightIndex = i;
                    break;
                }
            }
            edg.m_right->m_edges[edg.m_rightIndex] = &edg;
            edg.m_angle = glm::degrees(glm::acos(glm::clamp(glm::dot(m_flatNormals[edge.face1ID], m_flatNormals[edge.face2ID]), -1.0f, 1.0f)));

            const PDO_2DVertex& v1 = faces[edge.face1ID].vertices[edg.m_leftIndex];
            const PDO_2DVertex& v2 = faces[edge.face2ID].vertices[edg.m_rightIndex];
            edg.m_flapPosition = (SEdge::EFlapPosition)((v1.hasFlap ? 1 : 0) + (v2.hasFlap ? 2 : 0));
        }
        else
        {
            edg.m_right = nullptr;
            edg.m_rightIndex = -1;
            edg.m_angle = 0.0f;
            edg.m_flapPosition = SEdge::FP_NONE;
        }

        edg.m_snapped = edge.snapped;
        SetFoldType(edg);
    }

    for(auto it=parts.begin(); it!=parts.end(); it++)
    {
        const PDO_Part& part = it->second;
        m_groups.push_back(STriGroup());
        STriGroup& grp = m_groups.back();

        for(const PDO_Face* fc : part.faces)
        {
            grp.m_tris.push_back(&m_tri2D[fc->id]);
            m_tri2D[fc->id].m_myGroup = &grp;
        }

        grp.CentrateOrigin();
        grp.SetRotation(0.0f);
    }

    UpdateGroupDepth();
    CalculateAABBox();
    g_Mesh = this;
}

void CMesh::FillAdjTri_Gen2DTri()
{
    STriangle2D dummy;
    for(int i=0; i<3; ++i) dummy.m_edges[i] = nullptr;
    m_tri2D.resize(m_triangles.size(), dummy);

    for(int i = m_triangles.size()-1; i>=0; --i)
    {
        const glm::uvec4 &t = m_triangles[i];
        const glm::vec3 *v1[3] = { &m_vertices[t[0]], &m_vertices[t[1]], &m_vertices[t[2]] };
        const glm::vec3 *n1[3] = { &m_normals[t[0]],  &m_normals[t[1]],  &m_normals[t[2]] };

        float v1v2cos = glm::clamp(glm::dot(*v1[2]-*v1[0], *v1[1]-*v1[0])/(glm::distance(*v1[2], *v1[0])*glm::distance(*v1[1], *v1[0])), -1.0f, 1.0f);
        m_tri2D[i].m_vtx[0] = glm::vec2(0.0f, 0.0f);
        m_tri2D[i].m_vtx[1] = glm::vec2(glm::sin(glm::acos(v1v2cos)), v1v2cos)*glm::distance(*v1[1], *v1[0]);
        m_tri2D[i].m_vtx[2] = glm::vec2(0.0f, glm::distance(*v1[0], *v1[2]));

        m_tri2D[i].m_angleOY[0] = glm::degrees(glm::acos(v1v2cos));
        m_tri2D[i].m_angleOY[1] = -glm::degrees(glm::acos(glm::clamp(glm::dot(*v1[0]-*v1[2], *v1[1]-*v1[2])/(glm::distance(*v1[2],*v1[0])*glm::distance(*v1[2],*v1[1])), -1.0f, 1.0f) ));
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

            const glm::uvec4 &t2 = m_triangles[j];
            const glm::vec3 *v2[3] = { &m_vertices[t2[0]], &m_vertices[t2[1]], &m_vertices[t2[2]] };
            const glm::vec3 *n2[3] = { &m_normals[t2[0]], &m_normals[t2[1]], &m_normals[t2[2]] };

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

void CMesh::SetFoldType(SEdge &edg)
{
    if(!edg.HasTwoTriangles())
    {
        edg.m_foldType = SEdge::FT_FLAT;
        return;
    }

    int i = edg.m_left->m_id;
    int j = edg.m_right->m_id;
    glm::vec3 &v0 = m_vertices[m_triangles[i][0]];
    glm::vec3 &v1 = m_vertices[m_triangles[i][1]];
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
    switch(edg.m_rightIndex)
    {
        case 0: //edge with vtx 1 and 2
            toCheck = &m_vertices[m_triangles[j][2]]; //choose 3
            break;
        case 1: //2 and 3
            toCheck = &m_vertices[m_triangles[j][0]]; //choose 1
            break;
        case 2: //3 and 1
            toCheck = &m_vertices[m_triangles[j][1]]; //choose 2
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

    float angle = glm::degrees(glm::acos(glm::clamp(glm::dot(m_flatNormals[i], m_flatNormals[j]), -1.0f, 1.0f))); //length of normal = 1, skip division...
    edg.m_angle = angle;

    m_tri2D[j].m_id = j;
    SetFoldType(edg);
}

void CMesh::CalculateFlatNormals()
{
    for(const glm::uvec4 &t : m_triangles)
    {
        const glm::vec3 &vertex1 = m_vertices[t[0]];
        const glm::vec3 &vertex2 = m_vertices[t[1]];
        const glm::vec3 &vertex3 = m_vertices[t[2]];

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
        m_groups.push_back(STriGroup());
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

                    tr2 = tr.m_edges[n]->GetOtherTriangle(&tr);

                    if(tr2) //if edge n has neighbour...
                    if(tr.m_edges[n]->m_angle <= maxAngleDeg) //angle between neighbour is in valid range
                    if(tr2->m_myGroup == nullptr) //neighbour is groupless
                    {
                        //skip first iterator, because it is to be removed from list
                        auto itNextLargerAngle = candidates.begin();
                        auto itNLA_NoRefls = candNoRefls.begin();
                        itNextLargerAngle++;
                        itNLA_NoRefls++;

                        const float myAngle = tr.m_edges[n]->m_angle;

                        bool inserted = false;
                        do
                        {
                            float otherAngle = -999.0f;

                            if(itNextLargerAngle != candidates.end())
                            {
                                const STriangle2D &othTr = m_tri2D[(*itNextLargerAngle).first];

                                if((*itNextLargerAngle).second > -1)
                                {
                                    for(int oInd=0; oInd<3; ++oInd)
                                    {
                                        const STriangle2D *othTr2 = othTr.m_edges[oInd]->GetOtherTriangle(&othTr);
                                        if(othTr2 && othTr2->ID() == (*itNextLargerAngle).second)
                                        {
                                            otherAngle = othTr.m_edges[oInd]->m_angle;
                                            break;
                                        }
                                    }

                                    inserted = myAngle <= otherAngle;
                                }
                            } else {
                                inserted = true;
                            }

                            if(inserted)
                            {
                                candidates.insert(itNextLargerAngle, std::make_pair(tr2->m_id, c.first));
                                candNoRefls.insert(itNLA_NoRefls, tr2->m_id);

                                while(itNextLargerAngle != candidates.end())
                                {
                                    if((*itNextLargerAngle).first == tr2->m_id)
                                    {
                                        candidates.erase(itNextLargerAngle);
                                        candNoRefls.erase(itNLA_NoRefls);
                                        break;
                                    }
                                    itNextLargerAngle++;
                                    itNLA_NoRefls++;
                                }
                            } else {
                                itNextLargerAngle++;
                                itNLA_NoRefls++;
                            }

                        } while(!inserted);
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

void CMesh::PackGroups(bool undoable)
{
    struct SGroupBBox
    {
        glm::vec2 topLeft;
        glm::vec2 rightDown;
        glm::vec2 position;
        float area;
        float width;
        float height;
        CMesh::STriGroup *grp;

        glm::vec2 GetFinalPosition(glm::vec2 offset)
        {
            const float groupGap1x = 0.75f;//grp->aabbHSide*0.1f;
            position += offset;
            glm::vec2 grpCenterOffset(grp->GetPosition().x - topLeft.x, grp->GetPosition().y - rightDown.y);
            return grpCenterOffset + glm::vec2(position[0]+groupGap1x, position[1]+groupGap1x);
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
    float binHeight = 0.0f;
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

    //make a few passes of FCNR for better results
    for(int pass=0; pass<2; ++pass)
    {
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

        binHeight = levels.back().floorHeight + levels.back().ceilHeight;
        //adjust width for the next pass of FCNR
        binWidth = glm::max(maxWidth, (binWidth + binHeight) * 0.5f);
    }

    CIvoCommand* cmd = nullptr;
    if(undoable)
    {
        cmd = new CIvoCommand();
    }

    for(SGroupBBox &b : bboxes)
    {
        glm::vec2 finalPos = b.GetFinalPosition(glm::vec2(-binWidth*0.5f, -binHeight*0.5f));
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

    if(undoable)
    {
        m_undoStack.push(cmd);
    }
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

    m_bSphereRadius = 0.0f;
    glm::vec3 toCenter = GetAABBoxCenter();
    toCenter.y = m_aabbox[0].y;
    for(int i=0; i<7; i++)
    {
        m_aabbox[i] -= toCenter;
        m_bSphereRadius = glm::max(glm::length(glm::vec3(m_aabbox[i].x, m_aabbox[i].y*0.5f, m_aabbox[i].z)), m_bSphereRadius);
    }
    for(glm::vec3& v : m_vertices)
        v -= toCenter;
}

glm::vec3 CMesh::GetAABBoxCenter() const
{
    return 0.5f * (m_aabbox[0] + m_aabbox[6]);
}

glm::vec3 CMesh::GetSizeMillimeters() const
{
    glm::vec3 sizeMm;
    sizeMm.x = m_aabbox[6].x - m_aabbox[0].x;
    sizeMm.y = m_aabbox[6].y - m_aabbox[0].y;
    sizeMm.z = m_aabbox[6].z - m_aabbox[0].z;
    sizeMm *= 10.0f;
    return sizeMm;
}

void CMesh::Undo()
{
    if(m_undoStack.canUndo())
    {
        m_undoStack.undo();
        UpdateGroupDepth();
    }
}

void CMesh::Redo()
{
    if(m_undoStack.canRedo())
    {
        m_undoStack.redo();
        UpdateGroupDepth();
    }
}

void CMesh::NotifyGroupMovement(STriGroup &grp, const glm::vec2& oldPos)
{
    CAtomicCommand cmdMov(CT_MOVE);
    cmdMov.SetTriangle(grp.m_tris.front());
    cmdMov.SetTranslation(grp.GetPosition() - oldPos);

    grp.SetPosition(oldPos.x, oldPos.y);

    CIvoCommand* cmd = new CIvoCommand();
    cmd->AddAction(cmdMov);
    m_undoStack.push(cmd);
}

void CMesh::NotifyGroupRotation(STriGroup &grp, float oldRot)
{
    CAtomicCommand cmdRot(CT_ROTATE);
    cmdRot.SetTriangle(grp.m_tris.front());
    cmdRot.SetRotation(grp.GetRotation() - oldRot);

    grp.SetRotation(oldRot);

    CIvoCommand* cmd = new CIvoCommand();
    cmd->AddAction(cmdRot);
    m_undoStack.push(cmd);
}

void CMesh::Serialize(FILE *f) const
{
    int sizeVar = 0;

    sizeVar = m_uvCoords.size();
    std::fwrite(&sizeVar, sizeof(sizeVar), 1, f);
    std::fwrite(m_uvCoords.data(), sizeof(glm::vec2), sizeVar, f);

    sizeVar = m_normals.size();
    std::fwrite(&sizeVar, sizeof(sizeVar), 1, f);
    std::fwrite(m_normals.data(), sizeof(glm::vec3), sizeVar, f);

    sizeVar = m_vertices.size();
    std::fwrite(&sizeVar, sizeof(sizeVar), 1, f);
    std::fwrite(m_vertices.data(), sizeof(glm::vec3), sizeVar, f);

    sizeVar = m_triangles.size();
    std::fwrite(&sizeVar, sizeof(sizeVar), 1, f);
    std::fwrite(m_triangles.data(), sizeof(glm::uvec4), sizeVar, f);

    sizeVar = m_tri2D.size();
    std::fwrite(&sizeVar, sizeof(sizeVar), 1, f);
    for(const STriangle2D& tr2d : m_tri2D)
    {
        tr2d.Serialize(f);
    }

    sizeVar = m_edges.size();
    std::fwrite(&sizeVar, sizeof(sizeVar), 1, f);
    for(const SEdge& e : m_edges)
    {
        e.Serialize(f);
    }

    sizeVar = m_groups.size();
    std::fwrite(&sizeVar, sizeof(sizeVar), 1, f);
    for(const STriGroup &g : m_groups)
    {
        g.Serialize(f);
    }

    {
        sizeVar = m_edges.size();
        std::vector<int> edgptrInd(sizeVar * 2);
        auto eIter = m_edges.begin();
        for(int i=0; i<sizeVar*2; i+=2, eIter++)
        {
            const SEdge& e = *eIter;
            if(e.m_left)
            {
                edgptrInd[i] = (int)(e.m_left - &m_tri2D[0]);
            } else {
                edgptrInd[i] = -1;
            }
            if(e.m_right)
            {
                edgptrInd[i + 1] = (int)(e.m_right - &m_tri2D[0]);
            } else {
                edgptrInd[i + 1] = -1;
            }
        }
        std::fwrite(edgptrInd.data(), sizeof(int), sizeVar * 2, f);
    }
}

void CMesh::Deserialize(FILE *f)
{
    Clear();
    g_Mesh = this;

    int sizeVar;

    SAFE_FREAD(&sizeVar, sizeof(sizeVar), 1, f);
    for(int i=0; i<sizeVar; i++)
    {
        glm::vec2 uv;
        SAFE_FREAD(&uv, sizeof(glm::vec2), 1, f);
        m_uvCoords.push_back(uv);
    }

    SAFE_FREAD(&sizeVar, sizeof(sizeVar), 1, f);
    for(int i=0; i<sizeVar; i++)
    {
        glm::vec3 norm;
        SAFE_FREAD(&norm, sizeof(glm::vec3), 1, f);
        m_normals.push_back(norm);
    }

    SAFE_FREAD(&sizeVar, sizeof(sizeVar), 1, f);
    for(int i=0; i<sizeVar; i++)
    {
        glm::vec3 vert;
        SAFE_FREAD(&vert, sizeof(glm::vec3), 1, f);
        m_vertices.push_back(vert);
    }

    SAFE_FREAD(&sizeVar, sizeof(sizeVar), 1, f);
    for(int i=0; i<sizeVar; i++)
    {
        glm::uvec4 tri;
        SAFE_FREAD(&tri, sizeof(glm::uvec4), 1, f);
        m_triangles.push_back(tri);
    }

    SAFE_FREAD(&sizeVar, sizeof(sizeVar), 1, f);
    for(int i=0; i<sizeVar; i++)
    {
        m_tri2D.push_back(STriangle2D());
        m_tri2D.back().Deserialize(f);
    }

    SAFE_FREAD(&sizeVar, sizeof(sizeVar), 1, f);
    for(int i=0; i<sizeVar; ++i)
    {
        m_edges.push_back(SEdge());
        m_edges.back().Deserialize(f);
    }

    SAFE_FREAD(&sizeVar, sizeof(sizeVar), 1, f);
    for(int i=0; i<sizeVar; i++)
    {
        m_groups.push_back(STriGroup());
        m_groups.back().Deserialize(f);
    }

    {
        sizeVar = m_edges.size();
        std::vector<int> edgptrInd(sizeVar * 2);
        SAFE_FREAD(edgptrInd.data(), sizeof(int), sizeVar * 2, f);
        auto eIter = m_edges.begin();
        for(int i=0; i<sizeVar; ++i, eIter++)
        {
            SEdge &e = *eIter;

            int leftTriInd = edgptrInd[i*2];
            int rightTriInd = edgptrInd[i*2 + 1];
            if(leftTriInd >= (int)m_tri2D.size() || rightTriInd >= (int)m_tri2D.size())
            {
                throw std::logic_error("Model file is corrupted");
            }

            if(leftTriInd >= 0)
            {
                e.m_left = &m_tri2D[leftTriInd];
                e.m_left->m_edges[e.m_leftIndex] = &e;
            } else {
                e.m_left = nullptr;
            }
            if(rightTriInd >= 0)
            {
                e.m_right = &m_tri2D[rightTriInd];
                e.m_right->m_edges[e.m_rightIndex] = &e;
            } else {
                e.m_right = nullptr;
            }
        }
    }

    CalculateFlatNormals();
    CalculateAABBox();
    UpdateGroupDepth();
}

void CMesh::ApplyScale(float scale)
{
    for(glm::vec3& vtx : m_vertices)
        vtx *= scale;
    for(STriGroup& grp : m_groups)
        grp.Scale(scale);

    CalculateAABBox();
    UpdateGroupDepth();
}

void CMesh::Scale(float scale)
{
    CAtomicCommand cmdSca(CT_SCALE);
    cmdSca.SetScale(scale);
    cmdSca.SetTriangle(m_groups.front().m_tris.front());

    CIvoCommand* cmd = new CIvoCommand();
    cmd->AddAction(cmdSca);
    m_undoStack.push(cmd);
}
