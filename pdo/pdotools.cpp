#include <glm/common.hpp>
#include <cstring>
#include "pdotools.h"


void PDO_Part::AddFace(PDO_Face *f)
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

void PDO_Part::ApplyOffset(const glm::vec2 &off)
{
    offsetApplied = true;
    for(PDO_Face* f : faces)
    {
        for(PDO_2DVertex& v : f->vertices)
        {
            v.pos += off;
        }
    }
}

static bool VertexIsEar(const std::vector<PDO_2DVertex>& verts, int index)
{
    int sz = verts.size();
    index += sz;
    const glm::vec2& vA = verts[(index-1) % sz].pos;
    const glm::vec2& vB = verts[(index+0) % sz].pos;
    const glm::vec2& vC = verts[(index+1) % sz].pos;
    glm::vec2 v1 = vB - vA;
    glm::vec2 v2 = vC - vB;
    glm::vec2 v3 = vA - vC;

    if(v1.x * v2.y - v2.x * v1.y < 0.0f) //path along vertices vA-vB-vC is a clockwise turn?
        return false; //then it's not a valid triangle with counter-clockwise indexes!

    for(int i=sz; i<sz*2; i++)
    {
        if(i == index || i == index+1 || i == index-1)
            continue;

        const glm::vec2& vP = verts[i%sz].pos;

        glm::vec2 vi1 = vP - vA;
        glm::vec2 vi2 = vP - vB;
        glm::vec2 vi3 = vP - vC;

        bool b1 = v1.x * vi1.y - vi1.x * v1.y > 0.0f;
        bool b2 = v2.x * vi2.y - vi2.x * v2.y > 0.0f;
        bool b3 = v3.x * vi3.y - vi3.x * v3.y > 0.0f;

        if(b1 && b2 && b3) //point is inside triangle we want to cut?
            return false; //don't cut!
    }

    return true;
}

namespace PdoTools
{

int GetVersionPDO(const char *filename)
{
    FILE* f = std::fopen(filename, "rb");
    if(!f)
        return -1;

    char head[36];
    head[35] = '\0';
    size_t read = std::fread(head, sizeof(char), 35, f);
    std::fclose(f);

    if(read != 35)
    {
        return -1;
    }

    if(std::strcmp(head, "# Pepakura Designer Work Info ver 2") == 0)
    {
        return 20;
    }

    head[10] = '\0';
    if(std::strcmp(head, "version 3\n") == 0)
    {
        return 30;
    }

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
    const int sz = faces.size(); //only old faces can be not-triangles
    for(int i=0; i<sz; i++)
    {
        int vertSz = faces[i].vertices.size();
        while(vertSz > 3)
        {
            for(int j=0; j<vertSz; j++)
            {
                if(VertexIsEar(faces[i].vertices, j))
                {
                    const int v0 = (j+vertSz-1)%vertSz;
                    const int v1 = j;
                    const int v2 = (j+vertSz+1)%vertSz;
                    const int v03D = faces[i].vertices[v0].index3Dvert;
                    const int v13D = faces[i].vertices[v1].index3Dvert;
                    const int v23D = faces[i].vertices[v2].index3Dvert;

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
                                edge->face1ID = faces.size() - 1;//index of pushed face
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
                                edge->face2ID = faces.size() - 1;
                                newFace.edgesSecondary.push_back(edge);
                                faces[i].edgesSecondary.erase(faces[i].edgesSecondary.begin() + edg);
                                break;
                            }
                        }
                    }

                    PDO_Edge newEdge;
                    newEdge.face1ID = i;
                    newEdge.face2ID = faces.size() - 1;
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
