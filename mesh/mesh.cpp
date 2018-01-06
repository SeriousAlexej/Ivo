#include <fstream>
#include <algorithm>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <limits>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <QObject>
#include "mesh/mesh.h"
#include "mesh/command.h"
#include "settings/settings.h"
#include "io/saferead.h"
#include "notification/hub.h"

using glm::uvec4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;
using glm::dot;
using glm::distance;
using glm::clamp;
using glm::degrees;
using glm::inverse;
using glm::max;
using glm::min;
using glm::sqrt;
using glm::length;
using glm::cross;
using glm::normalize;

CMesh* CMesh::g_Mesh = nullptr;

CMesh::CMesh()
{
    m_undoStack.setUndoLimit(100);

    QObject::connect(&m_undoStack, &QUndoStack::canRedoChanged, [this](){ NOTIFY(UndoRedoChanged); });
    QObject::connect(&m_undoStack, &QUndoStack::canUndoChanged, [this](){ NOTIFY(UndoRedoChanged); });
}

CMesh::~CMesh()
{
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
    ClearPickedTriangles();
}

void CMesh::ClearPickedTriangles()
{
    m_pickTriIndices.clear();
}

void CMesh::SetTriangleAsPicked(int index)
{
    m_pickTriIndices.insert(index);
}

void CMesh::SetTriangleAsUnpicked(int index)
{
    auto foundPos = m_pickTriIndices.find(index);
    if(foundPos != m_pickTriIndices.end())
    {
        m_pickTriIndices.erase(foundPos);
    }
}

bool CMesh::IsTrianglePicked(int index) const
{
    return m_pickTriIndices.find(index) != m_pickTriIndices.end();
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

            uvec4 tr;
            for(int i=0; i<3; ++i)
            {
                unsigned newIndex = m_uvCoords.size();
                unsigned vertexIndex = face->mIndices[i];

                if(indicesRemap.find(vertexIndex) == indicesRemap.end())
                {
                    if(mesh->HasTextureCoords(0))
                    {
                        m_uvCoords.push_back(vec2(mesh->mTextureCoords[0][vertexIndex].x, mesh->mTextureCoords[0][vertexIndex].y));
                    } else {
                        m_uvCoords.push_back(vec2(0.0f, 0.0f));
                    }
                    m_normals.push_back(vec3(mesh->mNormals[vertexIndex].x, mesh->mNormals[vertexIndex].y, mesh->mNormals[vertexIndex].z));
                    m_vertices.push_back(vec3(mesh->mVertices[vertexIndex].x, mesh->mVertices[vertexIndex].y, mesh->mVertices[vertexIndex].z));

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
                        const std::vector<vec3>&                      vertices3D,
                        const std::unordered_map<unsigned, PDO_Part>& parts)
{
    Clear();

    m_tri2D.resize(faces.size());
    m_triangles.resize(faces.size());

    for(unsigned f=0; f<faces.size(); f++)
    {
        const PDO_Face& face = faces[f];
        STriangle2D&    tr2D = m_tri2D[f];
        uvec4&     tr = m_triangles[f];

        vec2 averageTri2DPos(0.0f, 0.0f);
        for(int i=0; i<3; ++i)
        {
            const PDO_2DVertex& vertex = face.vertices[i];

            unsigned newIndex = m_uvCoords.size();

            m_uvCoords.push_back(vertex.uv);
            m_normals.push_back(vec3(0.0f, 1.0f, 0.0f));
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
        tr2D.m_relativeMx = mat3(1);
        for(int i=0; i<3; i++)
        {
            tr2D.m_edges[i] = nullptr;
            tr2D.m_flapSharp[i] = false;
            tr2D.m_vtx[i] = tr2D.m_vtxR[i] = face.vertices[i].pos - averageTri2DPos;
            tr2D.m_vtxRT[i] = face.vertices[i].pos;
        }
        for(int i=0; i<3; i++)
        {
            tr2D.m_edgeLen[i] = distance(tr2D.m_vtx[i], tr2D.m_vtx[(i+1)%3]);

            vec2 vecAngle = tr2D.m_vtx[(i+1)%3] - tr2D.m_vtx[i];
            float sgnX = (vecAngle.x < 0.0f ? -1.0f : 1.0f);
            tr2D.m_angleOY[i] = sgnX * degrees(acos(clamp(vecAngle.y / length(vecAngle), -1.0f, 1.0f)));
        }
        tr2D.ComputeNormals();
    }

    CalculateFlatNormals();

    for(const std::unique_ptr<PDO_Edge>& e : edges)
    {
        const PDO_Edge& edge = *e;

        m_edges.emplace_back();
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
            edg.m_angle = degrees(acos(clamp(dot(m_flatNormals[edge.face1ID], m_flatNormals[edge.face2ID]), -1.0f, 1.0f)));

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
        m_groups.emplace_back();
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
        const uvec4 &t = m_triangles[i];
        const vec3* v1[3] = { &m_vertices[t[0]], &m_vertices[t[1]], &m_vertices[t[2]] };
        const vec3* n1[3] = { &m_normals[t[0]],  &m_normals[t[1]],  &m_normals[t[2]] };

        float v1v2cos = clamp(dot(*v1[2]-*v1[0], *v1[1]-*v1[0])/(distance(*v1[2], *v1[0])*distance(*v1[1], *v1[0])), -1.0f, 1.0f);
        m_tri2D[i].m_vtx[0] = vec2(0.0f, 0.0f);
        m_tri2D[i].m_vtx[1] = vec2(sin(acos(v1v2cos)), v1v2cos)*distance(*v1[1], *v1[0]);
        m_tri2D[i].m_vtx[2] = vec2(0.0f, distance(*v1[0], *v1[2]));

        m_tri2D[i].m_angleOY[0] = degrees(acos(v1v2cos));
        m_tri2D[i].m_angleOY[1] = -degrees(acos(clamp(dot(*v1[0]-*v1[2], *v1[1]-*v1[2])/(distance(*v1[2],*v1[0])*distance(*v1[2],*v1[1])), -1.0f, 1.0f) ));
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

            const uvec4 &t2 = m_triangles[j];
            const vec3* v2[3] = { &m_vertices[t2[0]], &m_vertices[t2[1]], &m_vertices[t2[2]] };
            const vec3* n2[3] = { &m_normals[t2[0]], &m_normals[t2[1]], &m_normals[t2[2]] };

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
                m_edges.emplace_back();
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
    vec3 &v0 = m_vertices[m_triangles[i][0]];
    vec3 &v1 = m_vertices[m_triangles[i][1]];
    vec3 &up = m_flatNormals[i];
    vec3 front = normalize(v0 - v1);
    vec3 right = cross(front, up);
    mat4 triBasis;
    triBasis[0] = vec4(right[0], right[1], right[2], 0.0f);
    triBasis[1] = vec4(up[0],    up[1],    up[2],    0.0f);
    triBasis[2] = vec4(front[0], front[1], front[2], 0.0f);
    triBasis[3] = vec4(v1[0],    v1[1],    v1[2],    1.0f);
    vec3 *toCheck = nullptr;
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
    float pHeight = ( inverse(triBasis) * vec4((*toCheck)[0], (*toCheck)[1], (*toCheck)[2], 1.0f) )[1];
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
    m_edges.emplace_back();
    SEdge &edg = m_edges.back();
    edg.m_left = &m_tri2D[i];
    edg.m_right = &m_tri2D[j];
    edg.m_leftIndex = e1;
    edg.m_rightIndex = e2;
    edg.m_snapped = false;
    edg.m_flapPosition = SEdge::FP_LEFT;
    m_tri2D[i].m_edges[e1] = &edg;
    m_tri2D[j].m_edges[e2] = &edg;

    float angle = degrees(acos(clamp(dot(m_flatNormals[i], m_flatNormals[j]), -1.0f, 1.0f))); //length of normal = 1, skip division...
    edg.m_angle = angle;

    m_tri2D[j].m_id = j;
    SetFoldType(edg);
}

void CMesh::CalculateFlatNormals()
{
    for(const uvec4 &t : m_triangles)
    {
        const vec3 &vertex1 = m_vertices[t[0]];
        const vec3 &vertex2 = m_vertices[t[1]];
        const vec3 &vertex3 = m_vertices[t[2]];

        vec3 faceNormal = normalize(cross((vertex2-vertex1), (vertex3-vertex1)));
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
        m_groups.emplace_back();
        //list of candidates contains triangles that might get in group
        std::list<std::pair<int, int>> candidates;
        candidates.push_front(std::make_pair(i, -1));
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
                        itNextLargerAngle++;

                        const float myAngle = tr.m_edges[n]->m_angle;

                        //insertion to 'sorted' list
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

                                //remove possible duplicates
                                while(itNextLargerAngle != candidates.end())
                                {
                                    if((*itNextLargerAngle).first == tr2->m_id)
                                    {
                                        candidates.erase(itNextLargerAngle);
                                        break;
                                    }
                                    itNextLargerAngle++;
                                }
                            } else {
                                itNextLargerAngle++;
                            }

                        } while(!inserted);
                    }
                }
            }
            //this candidate has been processed
            candidates.pop_front();
        }
        grp.CentrateOrigin();
    }
    UpdateGroupDepth();
}

void CMesh::GroupPickedTriangles()
{
    //this functions similar to CMesh::GroupTriangles but with some differences
    CIvoCommand* cmd = new CIvoCommand();

    //first, break all picked triangles
    for(int i : m_pickTriIndices)
    {
        for(int e=0; e<3; e++)
        {
            if(m_tri2D[i].m_edges[e]->IsSnapped())
            {
                CIvoCommand* breakCmd = m_tri2D[i].m_myGroup->GetBreakEdgeCmd(&m_tri2D[i], e);
                if(breakCmd)
                {
                    breakCmd->redo();
                    cmd->AddAction(std::move(*breakCmd));
                    delete breakCmd;
                }
            }
        }
    }

    std::unordered_set<int> processedTris;

    //then, try to group them
    for(int i : m_pickTriIndices)
    {
        if(processedTris.find(i) != processedTris.end())
            continue;
        //list of candidates contains triangles that might get in group
        std::list<std::pair<int, int>> candidates;
        candidates.push_front(std::make_pair(i, -1));

        std::function<void(STriangle2D&)> addNeighbours = [this, &processedTris, &candidates](STriangle2D& tr)
        {
            STriangle2D *tr2 = nullptr;
            for(int n=0; n<3; ++n)
            {
                if(!tr.m_edges[n]->HasTwoTriangles())
                    continue;

                tr2 = tr.m_edges[n]->GetOtherTriangle(&tr);

                if(tr2)
                if(m_pickTriIndices.find(tr2->ID()) != m_pickTriIndices.end() && processedTris.find(tr2->ID()) == processedTris.end())
                {
                    //skip first iterator, because it is to be removed from list
                    auto itNextLargerAngle = candidates.begin();
                    itNextLargerAngle++;

                    const float myAngle = tr.m_edges[n]->m_angle;

                    //insertion to 'sorted' list
                    bool inserted = false;
                    do
                    {
                        if(itNextLargerAngle != candidates.end())
                        {
                            const STriangle2D &othTr = m_tri2D[(*itNextLargerAngle).first];

                            if((*itNextLargerAngle).second > -1)
                            {
                                inserted = myAngle <= othTr.m_edges[(*itNextLargerAngle).second]->m_angle;
                            }
                        } else {
                            inserted = true;
                        }

                        if(inserted)
                        {
                            candidates.insert(itNextLargerAngle, std::make_pair(tr2->m_id, tr.m_edges[n]->GetOtherTriIndex(&tr)));

                            //remove possible duplicates
                            while(itNextLargerAngle != candidates.end())
                            {
                                if((*itNextLargerAngle).first == tr2->m_id)
                                {
                                    candidates.erase(itNextLargerAngle);
                                    break;
                                }
                                itNextLargerAngle++;
                            }
                        } else {
                            itNextLargerAngle++;
                        }

                    } while(!inserted);
                }
            }
        };

        //because this method does not create groups directly
        //and instead uses 'join' and 'break' commands,
        //use triangle's existing group. Add it's neighbours
        //and remove self, because it is already in it's group
        processedTris.insert(i);
        addNeighbours(m_tri2D[i]);
        candidates.pop_front();

        while(!candidates.empty())
        {
            std::pair<int, int> &c = candidates.front();

            CIvoCommand* joinCmd = m_tri2D[c.first].m_myGroup->GetJoinEdgeCmd(&m_tri2D[c.first], c.second);
            if(joinCmd)
            {
                joinCmd->redo();
                cmd->AddAction(std::move(*joinCmd));
                delete joinCmd;
            }

            processedTris.insert(c.first);

            addNeighbours(m_tri2D[c.first]);

            candidates.pop_front();
        }
    }

    cmd->undo();

    CMesh::g_Mesh->m_undoStack.push(cmd);
    UpdateGroupDepth();
    ClearPickedTriangles();
}

CMesh::STriGroup* CMesh::GroupUnderCursor(const vec2& curPos)
{
    std::list<STriGroup*> possibleGroups;
    for(STriGroup& grp : m_groups)
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

void CMesh::GetStuffUnderCursor(const vec2& curPos, CMesh::STriangle2D*& tr, int &e) const
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
    float lowestX  = std::numeric_limits<float>::max();
    float highestX = std::numeric_limits<float>::lowest();
    float lowestY  = lowestX;
    float highestY = highestX;
    float lowestZ  = lowestX;
    float highestZ = highestX;
    for(const vec3& v : m_vertices)
    {
        lowestX = min(lowestX, v.x);
        highestX = max(highestX, v.x);
        lowestY = min(lowestY, v.y);
        highestY = max(highestY, v.y);
        lowestZ = min(lowestZ, v.z);
        highestZ = max(highestZ, v.z);
    }
    m_aabbox[0] = vec3(lowestX, lowestY, lowestZ);
    m_aabbox[1] = vec3(lowestX, lowestY, highestZ);
    m_aabbox[2] = vec3(highestX, lowestY, highestZ);
    m_aabbox[3] = vec3(highestX, lowestY, lowestZ);
    m_aabbox[4] = vec3(lowestX, highestY, lowestZ);
    m_aabbox[5] = vec3(lowestX, highestY, highestZ);
    m_aabbox[6] = vec3(highestX, highestY, highestZ);
    m_aabbox[7] = vec3(highestX, highestY, lowestZ);

    m_bSphereRadius = 0.0f;
    vec3 toCenter = GetAABBoxCenter();
    toCenter.y = m_aabbox[0].y;
    for(int i=0; i<7; i++)
    {
        m_aabbox[i] -= toCenter;
        m_bSphereRadius = max(length(vec3(m_aabbox[i].x, m_aabbox[i].y*0.5f, m_aabbox[i].z)), m_bSphereRadius);
    }
    for(vec3& v : m_vertices)
        v -= toCenter;
}

vec3 CMesh::GetAABBoxCenter() const
{
    return 0.5f * (m_aabbox[0] + m_aabbox[6]);
}

vec3 CMesh::GetSizeMillimeters() const
{
    vec3 sizeMm;
    sizeMm.x = m_aabbox[6].x - m_aabbox[0].x;
    sizeMm.y = m_aabbox[6].y - m_aabbox[0].y;
    sizeMm.z = m_aabbox[6].z - m_aabbox[0].z;
    sizeMm *= 10.0f;
    return sizeMm;
}

bool CMesh::CanRedo() const
{
    return m_undoStack.canRedo();
}

bool CMesh::CanUndo() const
{
    return m_undoStack.canUndo();
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

void CMesh::NotifyGroupMovement(STriGroup &grp, const vec2& oldPos)
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
    std::fwrite(m_uvCoords.data(), sizeof(vec2), sizeVar, f);

    sizeVar = m_normals.size();
    std::fwrite(&sizeVar, sizeof(sizeVar), 1, f);
    std::fwrite(m_normals.data(), sizeof(vec3), sizeVar, f);

    sizeVar = m_vertices.size();
    std::fwrite(&sizeVar, sizeof(sizeVar), 1, f);
    std::fwrite(m_vertices.data(), sizeof(vec3), sizeVar, f);

    sizeVar = m_triangles.size();
    std::fwrite(&sizeVar, sizeof(sizeVar), 1, f);
    std::fwrite(m_triangles.data(), sizeof(uvec4), sizeVar, f);

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
        vec2 uv;
        SAFE_FREAD(&uv, sizeof(vec2), 1, f);
        m_uvCoords.push_back(uv);
    }

    SAFE_FREAD(&sizeVar, sizeof(sizeVar), 1, f);
    for(int i=0; i<sizeVar; i++)
    {
        vec3 norm;
        SAFE_FREAD(&norm, sizeof(vec3), 1, f);
        m_normals.push_back(norm);
    }

    SAFE_FREAD(&sizeVar, sizeof(sizeVar), 1, f);
    for(int i=0; i<sizeVar; i++)
    {
        vec3 vert;
        SAFE_FREAD(&vert, sizeof(vec3), 1, f);
        m_vertices.push_back(vert);
    }

    SAFE_FREAD(&sizeVar, sizeof(sizeVar), 1, f);
    for(int i=0; i<sizeVar; i++)
    {
        uvec4 tri;
        SAFE_FREAD(&tri, sizeof(uvec4), 1, f);
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
        m_edges.emplace_back();
        m_edges.back().Deserialize(f);
    }

    SAFE_FREAD(&sizeVar, sizeof(sizeVar), 1, f);
    for(int i=0; i<sizeVar; i++)
    {
        m_groups.emplace_back();
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

void CMesh::ApplyScale(const float scale)
{
    for(vec3& vtx : m_vertices)
        vtx *= scale;
    for(STriGroup& grp : m_groups)
        grp.Scale(scale);

    CalculateAABBox();
    UpdateGroupDepth();
}

void CMesh::Scale(const float scale)
{
    CAtomicCommand cmdSca(CT_SCALE);
    cmdSca.SetScale(scale);
    cmdSca.SetTriangle(m_groups.front().m_tris.front());

    CIvoCommand* cmd = new CIvoCommand();
    cmd->AddAction(cmdSca);
    m_undoStack.push(cmd);
}

SAABBox2D CMesh::GetAABBox2D() const
{
    SAABBox2D bbox(m_groups.front().m_toRightDown, m_groups.front().m_toTopLeft);

    for(const STriGroup& grp : m_groups)
    {
        bbox = bbox.Union(SAABBox2D(grp.m_toRightDown, grp.m_toTopLeft));
    }

    return bbox;
}

bool CMesh::Intersects(const SAABBox2D &bbox) const
{
    for(const STriGroup& grp : m_groups)
    {
        if(bbox.Intersects(SAABBox2D(grp.m_toRightDown, grp.m_toTopLeft)))
            return true;
    }
    return false;
}
