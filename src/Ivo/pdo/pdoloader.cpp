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
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <stdexcept>
#include <clocale>
#include <cstddef>
#include "settings/settings.h"
#include "mesh/mesh.h"
#include "interface/mainwindow.h"
#include "io/saferead.h"
#include "pdotools.h"

void CMainWindow::LoadFromPDOv2_0(const QString& filename)
{
    std::setlocale(LC_NUMERIC, "C");

    CSafeFile fi(filename.toStdString());
    fi.SafetyCheck();

    std::vector<glm::vec3>                      vertices3D;
    std::vector<glm::vec2>                      offsets;
    std::vector<PDO_Face>                       faces;
    std::vector<std::unique_ptr<PDO_Edge>>      edges;
    std::unordered_map<unsigned, std::string>   materialNames;
    std::unordered_map<unsigned, PDO_Part>      parts;

    fi.ReadLine();//# Pepakura Designer Work Info ver 2
    fi.ReadLine();//#
    fi.ReadLine();//
    fi.ReadLine();//version 2
    fi.ReadLine();//min_version 2
    fi.ReadLine();//
    fi.ReadLine();//model %f %f %f %f %f %f

    int solids = 0;
    fi.LineScanf("solids %d", &solids);
    for(int i=0; i<solids; i++)
    {
        std::size_t prevFacesSize = faces.size();
        std::size_t prevVerticesSize = vertices3D.size();

        bool skip = false;
        {
            int doNotSkip = 1;
            fi.ReadLine();//solid
            fi.ReadLine();//name
            fi.LineScanf("%d", &doNotSkip);
            skip = doNotSkip == 0;
        }

        int vertices = 0;
        fi.LineScanf("vertices %d", &vertices);
        for(int j=0; j<vertices; j++)
        {
            glm::vec3 vert(0.0f, 0.0f, 0.0f);
            fi.LineScanf("%f %f %f", &vert.x, &vert.y, &vert.z);
            //vert.y *= -1.0f;
            if(!skip)
                vertices3D.push_back(vert);
        }

        int numFaces = 0;
        fi.LineScanf("faces %d", &numFaces);
        for(int j=0; j<numFaces; j++)
        {
            int numvertices = 0;
            int matID = 0;
            int partID = 0;
            fi.LineScanf("%d %d %*f %*f %*f %*f %d", &matID, &partID, &numvertices);

            PDO_Face face;
            face.id = prevFacesSize + j;
            face.matIndex = matID;
            face.partIndex = partID;

            for(int k=0; k<numvertices; k++)
            {
                PDO_2DVertex vert;
                std::size_t vert3dindex;
                int hasFlap;
                fi.LineScanf("%zd %f %f %f %f %d %*d %f", &vert3dindex, &vert.pos.x, &vert.pos.y, &vert.uv.x, &vert.uv.y, &hasFlap, &vert.flapLength);
                vert.index3Dvert = vert3dindex + prevVerticesSize;
                vert.hasFlap = (hasFlap != 0);
                vert.pos.y *= -1.0f;

                if(!skip)
                    face.vertices.push_back(vert);
            }

            if(!skip && numvertices >= 3)
                faces.push_back(face);
        }

        int numEdges = 0;
        fi.LineScanf("edges %d", &numEdges);
        for(int j=0; j<numEdges; j++)
        {
            PDO_Edge edge;
            int snapped;
            fi.LineScanf("%d %d %zd %zd %*d %d", &edge.face1ID, &edge.face2ID, &edge.vtx1ID, &edge.vtx2ID, &snapped);
            edge.face1ID += static_cast<int>(prevFacesSize);
            if(edge.face2ID >= 0)
                edge.face2ID += static_cast<int>(prevFacesSize);
            edge.vtx1ID += prevVerticesSize;
            edge.vtx2ID += prevVerticesSize;
            edge.snapped = (snapped != 0);

            if(!skip)
            {
                edges.push_back(std::unique_ptr<PDO_Edge>(new PDO_Edge(edge)));

                faces[edge.face1ID].edges.push_back(edges.back().get());
                if(edge.face2ID >= 0)
                {
                    faces[edge.face2ID].edgesSecondary.push_back(edges.back().get());
                }
            }
        }
    }

    fi.ReadLine();//defaultmaterial
    fi.ReadLine();//material
    fi.ReadLine();//
    fi.ReadLine();//default material settings, ignore

    int materials = 0;
    fi.LineScanf("materials %d", &materials);
    for(int j=0; j<materials; j++)
    {
        fi.ReadLine();//material
        std::string matName = fi.ReadLine();
        if(matName.empty())
        {
            matName = std::string("<unnamed_material_") + std::to_string(j+1) + ">";
        }
        materialNames[j] = matName;

        int hasTexture = 0;
        float colR, colG, colB;
        fi.LineScanf("%*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %f %f %f %*d %d", &colR, &colG, &colB, &hasTexture);

        m_textures[j] = std::string("<imported_") + std::to_string(j+1) + ">";

        if(hasTexture != 0)
        {
            fi.ReadLine();//

            int texWidth=0;
            int texHeight=0;

            fi.LineScanf("%d %d", &texWidth, &texHeight);

            std::unique_ptr<unsigned char[]> imgBuffer(new unsigned char[texWidth * texHeight * 3]);
            fi.ReadBuffer(imgBuffer.get(), sizeof(unsigned char), texWidth * texHeight * 3);
            fi.ReadLine();

            m_textureImages[j].reset(new QImage(texWidth, texHeight, QImage::Format_RGB32));

            for(int x=0; x<texWidth; x++)
            for(int y=0; y<texHeight; y++)
            {
                unsigned char r = imgBuffer[y*texWidth*3 + x*3+0];
                unsigned char g = imgBuffer[y*texWidth*3 + x*3+1];
                unsigned char b = imgBuffer[y*texWidth*3 + x*3+2];
                m_textureImages[j]->setPixel(x, y, QColor(r, g, b).rgb());
            }
        } else {
            m_textureImages[j].reset(new QImage(1, 1, QImage::Format_RGB32));
            m_textureImages[j]->setPixel(0, 0, QColor(colR * 255, colG * 255, colB * 255).rgb());
        }
    }

    int numParts = 0;
    fi.LineScanf("parts %d", &numParts);
    for(int j=0; j<numParts; j++)
    {
        glm::vec2 offs;
        fi.LineScanf("%*d %f %f %*f %*f", &offs.x, &offs.y);
        offs.y *= -1.0f;

        offsets.push_back(offs);
    }

    int texts = 0;
    fi.LineScanf("text %d", &texts);
    for(int j=0; j<texts; ++j)
    {
        fi.ReadLine();//%d //???
        fi.ReadLine();//font name
        fi.ReadLine();//string
        fi.ReadLine();//params...
    }

    int pageType = 0;
    float scale2d = 1.0f;
    float scale3d = 1.0f;
    glm::vec2 margins(0.0f, 0.0f);

    fi.ReadLine();//info
    fi.ReadLine();//key
    fi.ReadLine();//iLlevel
    fi.LineScanf("dMag3d %f", &scale3d);
    fi.LineScanf("dMag2d %f", &scale2d);
    fi.ReadLine();//"dTenkaizuX %f
    fi.ReadLine();//"dTenkaizuY %f
    fi.ReadLine();//"dTenkaizuWidth %f"  - 2D pattern width
    fi.ReadLine();//"dTenkaizuHeight %f" - 2D pattern height
    fi.ReadLine();//"dTenkaizuMargin %f
    fi.ReadLine();//"bReverse %d
    fi.ReadLine();//"bFinished %d
    fi.ReadLine();//"iAngleEps %d
    fi.ReadLine();//"iTaniLineType %d
    fi.ReadLine();//"iYamaLineType %d
    fi.ReadLine();//"iCutLineType %d
    fi.ReadLine();//"bTextureCoodinates %d
    fi.ReadLine();//"bDrawFlap %d"           - show flaps
    fi.ReadLine();//"bDrawNumber %d"         - show edge ID
    fi.ReadLine();//"bUseMaterial %d
    fi.ReadLine();//"iEdgeNumberFontSize %d" - edge ID font size
    fi.ReadLine();//"bNorishiroReverse %d
    fi.ReadLine();//"bEdgeIdReverse %d"      - place edge ID outside face
    fi.ReadLine();//"bEnableLineAlpha %d
    fi.ReadLine();//"dTextureLineAlpha %f
    fi.ReadLine();//"bCullEdge %d
    fi.LineScanf("iPageType %d", &pageType);//"iPageType %d
    fi.LineScanf("dPageMarginSide %f", &margins.x);
    fi.LineScanf("dPageMarginTop %f", &margins.y);

    //pdo v2 likes to store quads and such...
    PdoTools::TriangulateFaces(faces, edges);

    glm::vec2 bbox(0.0f, 0.0f);
    glm::vec2 pageSize = PdoTools::GetPDOPaperSize(pageType);
    glm::vec2 borderSize = pageSize - margins*2.0f;

    //apply paper size
    CSettings& s = CSettings::GetInstance();
    s.SetPaperWidth((unsigned)(pageSize.x*10.0f));
    s.SetPaperHeight((unsigned)(pageSize.y*10.0f));
    s.SetMarginsHorizontal((unsigned)(margins.x*10.0f));
    s.SetMarginsVertical((unsigned)(margins.y*10.0f));

    //pre-apply scaling
    float real3Dscale = 1.0f;
    {
        const PDO_Face& f = faces.front();
        const PDO_2DVertex& v1 = f.vertices[0];
        const PDO_2DVertex& v2 = f.vertices[1];
        float len2D = glm::distance(v1.pos, v2.pos);
        float len3D = glm::distance(vertices3D[v1.index3Dvert], vertices3D[v2.index3Dvert]);
        real3Dscale = scale2d * len2D / len3D;
    }
    for(glm::vec3& v : vertices3D)
    {
        v *= real3Dscale;
        //v *= scale3d; //pepakura's scale makes only worse
    }
    for(PDO_Face& f : faces)
    {
        for(PDO_2DVertex& v : f.vertices)
        {
            v.pos += offsets[f.partIndex];
            v.pos *= scale2d;
            bbox.x = glm::max(bbox.x, v.pos.x);
            bbox.y = glm::min(bbox.y, v.pos.y);
        }
        parts[f.partIndex].AddFace(&f);
    }

    //find out how many sheets there can potentially be
    int sheetsHorizontal = 0;
    int sheetsVertical   = 0;

    if(bbox.x > 0.0f && bbox.y < 0.0f)
    {
        sheetsHorizontal = 1 + (int)glm::floor( bbox.x / borderSize.x);
        sheetsVertical   = 1 + (int)glm::floor(-bbox.y / borderSize.y);
    }

    m_model.reset(new CMesh());
    SetModelToWindows();

    for(int x=0; x<sheetsHorizontal; x++)
    for(int y=0; y<sheetsVertical; y++)
    {
        float sheetMinX = x*borderSize.x;
        float sheetMaxX = sheetMinX + borderSize.x;
        float sheetMaxY = -y*borderSize.y;
        float sheetMinY = sheetMaxY - borderSize.y;

        for(auto it=parts.begin(); it!=parts.end(); it++)
        {
            PDO_Part& p = it->second;
            if(p.offsetApplied)
                continue;

            bool atLeft   = p.maxX <= sheetMinX;
            bool atRight  = p.minX >= sheetMaxX;
            bool atTop    = p.minY >= sheetMaxY;
            bool atBottom = p.maxY <= sheetMinY;

            if(atLeft || atRight || atTop || atBottom)
                continue;

            //part is on sheet (x;y), offset it

            p.ApplyOffset(glm::vec2(x*margins.x*2.0f + margins.x, -y*margins.y*2.0f - margins.y));
        }
    }

    m_model->LoadFromPDO(faces, edges, vertices3D, parts);
    m_model->SetMaterials(materialNames);

    for(auto it=m_textureImages.begin(); it!=m_textureImages.end(); it++)
        if(it->second != nullptr)
            emit UpdateTexture(it->second.get(), it->first);
}
