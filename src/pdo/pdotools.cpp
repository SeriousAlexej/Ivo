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
#include <QString>
#include <glm/common.hpp>
#include <cstring>
#include "pdotools.h"
#include "geometric/compgeom.h"

using glm::leftTurn;
using glm::rightTurn;

void PDO_Part::AddFace(PDO_Face* f)
{
    faces.push_back(f);
    for(unsigned i=0; i<f->vertices.size(); i++)
    {
        minX = glm::min(minX, f->vertices[i].pos.x);
        maxX = glm::max(maxX, f->vertices[i].pos.x);
        minY = glm::min(minY, f->vertices[i].pos.y);
        maxY = glm::max(maxY, f->vertices[i].pos.y);
    }
}

void PDO_Part::ApplyOffset(const glm::vec2& off)
{
    offsetApplied = true;
    for(PDO_Face* f : faces)
        for(PDO_2DVertex& v : f->vertices)
            v.pos += off;
}

static bool VertexIsEar(const std::vector<PDO_2DVertex>& verts, std::size_t index)
{
    std::size_t sz = verts.size();
    index += sz;
    const glm::vec2& vA = verts[(index-1) % sz].pos;
    const glm::vec2& vB = verts[(index+0) % sz].pos;
    const glm::vec2& vC = verts[(index+1) % sz].pos;
    glm::vec2 v1 = vB - vA;
    glm::vec2 v2 = vC - vB;
    glm::vec2 v3 = vA - vC;

    if(rightTurn(v1, v2)) //path along vertices vA-vB-vC is a clockwise turn?
        return false; //then it's not a valid triangle with counter-clockwise indexes!

    for(std::size_t i=sz; i<sz*2; i++)
    {
        if(i == index || i == index+1 || i == index-1)
            continue;

        const glm::vec2& vP = verts[i%sz].pos;

        glm::vec2 vi1 = vP - vA;
        glm::vec2 vi2 = vP - vB;
        glm::vec2 vi3 = vP - vC;

        bool b1 = leftTurn(v1, vi1);
        bool b2 = leftTurn(v2, vi2);
        bool b3 = leftTurn(v3, vi3);

        if(b1 && b2 && b3) //point is inside triangle we want to cut?
            return false; //don't cut!
    }

    return true;
}

namespace PdoTools
{

int GetVersionPDO(const QString& filename)
{
    FILE* f = std::fopen(filename.toStdString().c_str(), "rb");
    if(!f)
        return -1;

    char head[36];
    head[35] = '\0';
    size_t read = std::fread(head, sizeof(char), 35, f);
    std::fclose(f);

    if(read != 35)
        return -1;

    if(std::strcmp(head, "# Pepakura Designer Work Info ver 2") == 0)
        return 20;

    head[10] = '\0';
    if(std::strcmp(head, "version 3\n") == 0)
        return 30;

    return -1;
}

glm::vec2 GetPDOPaperSize(int paperType)
{
    switch(paperType)
    {
    case 1: return glm::vec2(29.7f, 21.0f);
    case 2: return glm::vec2(29.7f, 42.0f);
    case 3: return glm::vec2(42.0f, 29.7f);
    case 4: return glm::vec2(18.2f, 25.7f);
    case 5: return glm::vec2(25.7f, 18.2f);
    case 6: return glm::vec2(25.7f, 36.4f);
    case 7: return glm::vec2(36.4f, 25.7f);
    default:return glm::vec2(21.0f, 29.7f);
    }
}

void TriangulateFaces(std::vector<PDO_Face>& faces, std::vector<std::unique_ptr<PDO_Edge>>& edges)
{
    const std::size_t sz = faces.size(); //only old faces can be not-triangles
    for(std::size_t i=0; i<sz; i++)
    {
        std::size_t vertSz = faces[i].vertices.size();
        while(vertSz > 3)
        {
            for(std::size_t j=0; j<vertSz; j++)
            {
                if(VertexIsEar(faces[i].vertices, j))
                {
                    const std::size_t v0 = (j+vertSz-1)%vertSz;
                    const std::size_t v1 = j;
                    const std::size_t v2 = (j+vertSz+1)%vertSz;
                    const std::size_t v03D = faces[i].vertices[v0].index3Dvert;
                    const std::size_t v13D = faces[i].vertices[v1].index3Dvert;
                    const std::size_t v23D = faces[i].vertices[v2].index3Dvert;

                    faces.push_back(PDO_Face());
                    PDO_Face& newFace = faces.back();
                    newFace.id = faces.size() - 1;
                    newFace.matIndex = faces[i].matIndex;
                    newFace.partIndex = faces[i].partIndex;
                    newFace.vertices.push_back(faces[i].vertices[v0]);
                    newFace.vertices.push_back(faces[i].vertices[v1]);
                    newFace.vertices.push_back(faces[i].vertices[v2]);

                    for(int k=0; k<2; k++)
                    {
                        for(unsigned edg=0; edg<faces[i].edges.size(); edg++)
                        {
                            PDO_Edge* edge = faces[i].edges[edg];
                            if((edge->vtx1ID == v03D && edge->vtx2ID == v13D) ||
                               (edge->vtx1ID == v13D && edge->vtx2ID == v23D))
                            {
                                edge->face1ID = static_cast<int>(faces.size() - 1);//index of pushed face
                                newFace.edges.push_back(edge);
                                faces[i].edges.erase(faces[i].edges.begin() + edg);
                                break;
                            }
                        }
                        for(unsigned edg=0; edg<faces[i].edgesSecondary.size(); edg++)
                        {
                            PDO_Edge* edge = faces[i].edgesSecondary[edg];
                            if((edge->vtx1ID == v13D && edge->vtx2ID == v03D) ||
                               (edge->vtx1ID == v23D && edge->vtx2ID == v13D))
                            {
                                edge->face2ID = static_cast<int>(faces.size() - 1);
                                newFace.edgesSecondary.push_back(edge);
                                faces[i].edgesSecondary.erase(faces[i].edgesSecondary.begin() + edg);
                                break;
                            }
                        }
                    }

                    PDO_Edge newEdge;
                    newEdge.face1ID = static_cast<int>(i);
                    newEdge.face2ID = static_cast<int>(faces.size() - 1);
                    newEdge.snapped = true;
                    newEdge.vtx1ID = faces[i].vertices[v0].index3Dvert;
                    newEdge.vtx2ID = faces[i].vertices[v2].index3Dvert;
                    edges.push_back(std::unique_ptr<PDO_Edge>(new PDO_Edge(newEdge)));
                    faces[i].edges.push_back(edges.back().get());
                    newFace.edgesSecondary.push_back(edges.back().get());

                    faces[i].vertices.erase(faces[i].vertices.begin() + j);
                    vertSz--;
                    break;
                }
            }
        }
    }
}

}//namespace PdoTools
