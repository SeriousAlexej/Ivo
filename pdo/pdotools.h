#ifndef PDOTOOLS_H
#define PDOTOOLS_H
#include <glm/vec2.hpp>
#include <memory>
#include <limits>
#include <vector>
#include <list>

struct PDO_Face;
struct PDO_Edge;

namespace PdoTools
{
extern glm::vec2 GetPDOPaperSize(int paperType);
extern int       GetVersionPDO(const char *filename);
extern void      TriangulateFaces(std::vector<PDO_Face>&                  faces,
                                  std::vector<std::unique_ptr<PDO_Edge>>& edges);
}

struct PDO_Edge
{
    int  face1ID;
    int  face2ID;
    int  vtx1ID;
    int  vtx2ID;
    bool snapped;
};

struct PDO_2DVertex
{
    float       flapLength;
    bool        hasFlap;
    int         index3Dvert;
    glm::vec2   pos;
    glm::vec2   uv;
};

struct PDO_Face
{
    int                       id;
    unsigned                  matIndex;
    unsigned                  partIndex;
    std::vector<PDO_2DVertex> vertices;
    std::vector<PDO_Edge*>    edges;
    std::vector<PDO_Edge*>    edgesSecondary;
};

struct PDO_Part
{
    float                  minX = std::numeric_limits<float>::max();
    float                  maxX = std::numeric_limits<float>::lowest();
    float                  minY = std::numeric_limits<float>::max();
    float                  maxY = std::numeric_limits<float>::lowest();
    std::list<PDO_Face*>   faces;
    bool                   offsetApplied = false;

    void AddFace(PDO_Face* f);
    void ApplyOffset(const glm::vec2& off);
};

#endif // PDOTOOLS_H
