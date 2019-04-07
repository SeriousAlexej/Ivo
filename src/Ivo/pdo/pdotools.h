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
#ifndef PDOTOOLS_H
#define PDOTOOLS_H
#include <QString>
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
extern int       GetVersionPDO(const QString& filename);
extern void      TriangulateFaces(std::vector<PDO_Face>&                  faces,
                                  std::vector<std::unique_ptr<PDO_Edge>>& edges);
}

struct PDO_Edge
{
    int         face1ID;
    int         face2ID;
    std::size_t vtx1ID;
    std::size_t vtx2ID;
    bool        snapped;
};

struct PDO_2DVertex
{
    float       flapLength;
    bool        hasFlap;
    std::size_t index3Dvert;
    glm::vec2   pos;
    glm::vec2   uv;
};

struct PDO_Face
{
    std::size_t               id;
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
