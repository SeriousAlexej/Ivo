#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <stdexcept>
#include "settings/settings.h"
#include "mesh/mesh.h"
#include "renderers/renwin.h"
#include "renderers/renwin2d.h"
#include "interface/mainwindow.h"
#include "io/saferead.h"
#include "pdotools.h"

void CMainWindow::LoadFromPDOv2_0(const char *filename)
{
    FILE* fi = std::fopen(filename, "rb");
    if(!fi)
        BadFile(fi);

    std::vector<glm::vec3>                      vertices3D;
    std::vector<glm::vec2>                      offsets;
    std::vector<PDO_Face>                       faces;
    std::vector<std::unique_ptr<PDO_Edge>>      edges;
    std::unordered_map<unsigned, std::string>   materialNames;
    std::unordered_map<unsigned, PDO_Part>      parts;

    ReadLine(fi);//# Pepakura Designer Work Info ver 2
    ReadLine(fi);//#
    ReadLine(fi);//
    ReadLine(fi);//version 2
    ReadLine(fi);//min_version 2
    ReadLine(fi);//
    ReadLine(fi);//model %f %f %f %f %f %f

    int solids = 0;
    SAFE_FLSCANF(fi, "solids %d", &solids);
    for(int i=0; i<solids; i++)
    {
        int prevFacesSize = faces.size();
        int prevVerticesSize = vertices3D.size();

        bool skip = false;
        {
            int doNotSkip = 1;
            ReadLine(fi);//solid
            ReadLine(fi);//name
            SAFE_FLSCANF(fi, "%d", &doNotSkip);
            skip = doNotSkip == 0;
        }

        int vertices = 0;
        SAFE_FLSCANF(fi, "vertices %d", &vertices);
        for(int j=0; j<vertices; j++)
        {
            glm::vec3 vert(0.0f, 0.0f, 0.0f);
            SAFE_FLSCANF(fi, "%f %f %f", &vert.x, &vert.y, &vert.z);
            //vert.y *= -1.0f;
            if(!skip)
                vertices3D.push_back(vert);
        }

        int numFaces = 0;
        SAFE_FLSCANF(fi, "faces %d", &numFaces);
        for(int j=0; j<numFaces; j++)
        {
            int numvertices = 0;
            int matID = 0;
            int partID = 0;
            SAFE_FLSCANF(fi, "%d %d %*f %*f %*f %*f %d", &matID, &partID, &numvertices);

            PDO_Face face;
            face.id = prevFacesSize + j;
            face.matIndex = matID;
            face.partIndex = partID;

            for(int k=0; k<numvertices; k++)
            {
                PDO_2DVertex vert;
                int vert3dindex;
                int hasFlap;
                SAFE_FLSCANF(fi, "%d %f %f %f %f %d %*d %f", &vert3dindex, &vert.pos.x, &vert.pos.y, &vert.uv.x, &vert.uv.y, &hasFlap, &vert.flapLength);
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
        SAFE_FLSCANF(fi, "edges %d", &numEdges);
        for(int j=0; j<numEdges; j++)
        {
            PDO_Edge edge;
            int snapped;
            SAFE_FLSCANF(fi, "%d %d %d %d %*d %d", &edge.face1ID, &edge.face2ID, &edge.vtx1ID, &edge.vtx2ID, &snapped);
            edge.face1ID += prevFacesSize;
            if(edge.face2ID >= 0)
                edge.face2ID += prevFacesSize;
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

    ReadLine(fi);//defaultmaterial
    ReadLine(fi);//material
    ReadLine(fi);//
    ReadLine(fi);//default material settings, ignore

    int materials = 0;
    SAFE_FLSCANF(fi, "materials %d", &materials);
    for(int j=0; j<materials; j++)
    {
        ReadLine(fi);//material
        std::string matName = ReadLine(fi);
        if(matName.empty())
        {
            matName = std::string("<unnamed_material_") + std::to_string(j+1) + ">";
        }
        materialNames[j] = matName;

        int hasTexture = 0;
        float colR, colG, colB;
        SAFE_FLSCANF(fi, "%*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %f %f %f %*d %d", &colR, &colG, &colB, &hasTexture);

        m_textures[j] = std::string("<imported_") + std::to_string(j+1) + ">";
        m_rw2->ReserveTextureID(j);
        ((IRenWin*)m_rw3)->ReserveTextureID(j);

        if(hasTexture != 0)
        {
            ReadLine(fi);//

            int texWidth=0;
            int texHeight=0;

            SAFE_FLSCANF(fi, "%d %d", &texWidth, &texHeight);

            std::unique_ptr<unsigned char[]> imgBuffer(new unsigned char[texWidth * texHeight * 3]);
            SAFE_FREAD(imgBuffer.get(), sizeof(unsigned char), texWidth * texHeight * 3, fi);

            ReadLine(fi);//

            m_textureImages[j].reset(new QImage(texWidth, texHeight, QImage::Format_RGB32));

            for(int x=0; x<texWidth; x++)
            for(int y=0; y<texHeight; y++)
            {
                unsigned char r = imgBuffer[y*texWidth*3 + x*3+0];
                unsigned char g = imgBuffer[y*texWidth*3 + x*3+1];
                unsigned char b = imgBuffer[y*texWidth*3 + x*3+2];
                m_textureImages[j]->setPixelColor(x, y, QColor(r, g, b));
            }
        } else {
            m_textureImages[j].reset(new QImage(1, 1, QImage::Format_RGB32));
            m_textureImages[j]->setPixelColor(0, 0, QColor(colR * 255, colG * 255, colB * 255));
        }
    }

    int numParts = 0;
    SAFE_FLSCANF(fi, "parts %d", &numParts);
    for(int j=0; j<numParts; j++)
    {
        glm::vec2 offs;
        SAFE_FLSCANF(fi, "%*d %f %f %*f %*f", &offs.x, &offs.y);
        offs.y *= -1.0f;

        offsets.push_back(offs);
    }

    int texts = 0;
    SAFE_FLSCANF(fi, "text %d", &texts);
    for(int j=0; j<texts; ++j)
    {
        ReadLine(fi);//%d //???
        ReadLine(fi);//font name
        ReadLine(fi);//string
        ReadLine(fi);//params...
    }

    int pageType = 0;
    float scale2d = 1.0f;
    float scale3d = 1.0f;
    glm::vec2 margins(0.0f, 0.0f);

    ReadLine(fi);//info
    ReadLine(fi);//key
    ReadLine(fi);//iLlevel
    SAFE_FLSCANF(fi, "dMag3d %f", &scale3d);
    SAFE_FLSCANF(fi, "dMag2d %f", &scale2d);
    ReadLine(fi);//"dTenkaizuX %f
    ReadLine(fi);//"dTenkaizuY %f
    ReadLine(fi);//"dTenkaizuWidth %f"  - 2D pattern width
    ReadLine(fi);//"dTenkaizuHeight %f" - 2D pattern height
    ReadLine(fi);//"dTenkaizuMargin %f
    ReadLine(fi);//"bReverse %d
    ReadLine(fi);//"bFinished %d
    ReadLine(fi);//"iAngleEps %d
    ReadLine(fi);//"iTaniLineType %d
    ReadLine(fi);//"iYamaLineType %d
    ReadLine(fi);//"iCutLineType %d
    ReadLine(fi);//"bTextureCoodinates %d
    ReadLine(fi);//"bDrawFlap %d"           - show flaps
    ReadLine(fi);//"bDrawNumber %d"         - show edge ID
    ReadLine(fi);//"bUseMaterial %d
    ReadLine(fi);//"iEdgeNumberFontSize %d" - edge ID font size
    ReadLine(fi);//"bNorishiroReverse %d
    ReadLine(fi);//"bEdgeIdReverse %d"      - place edge ID outside face
    ReadLine(fi);//"bEnableLineAlpha %d
    ReadLine(fi);//"dTextureLineAlpha %f
    ReadLine(fi);//"bCullEdge %d
    SAFE_FLSCANF(fi, "iPageType %d", &pageType);//"iPageType %d
    SAFE_FLSCANF(fi, "dPageMarginSide %f", &margins.x);
    SAFE_FLSCANF(fi, "dPageMarginTop %f", &margins.y);

    std::fclose(fi);

    //pdo v2 likes to store quads and such...
    TriangulateFaces(faces, edges);

    glm::vec2 bbox(0.0f, 0.0f);
    glm::vec2 pageSize = GetPDOPaperSize(pageType);
    glm::vec2 borderSize = pageSize - margins*2.0f;

    //apply paper size
    CSettings& s = CSettings::GetInstance();
    s.SetPaperWidth((unsigned)(pageSize.x*10.0f));
    s.SetPaperHeight((unsigned)(pageSize.y*10.0f));

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

    m_model = new CMesh();
    m_rw2->SetModel(m_model);
    ((IRenWin*)m_rw3)->SetModel(m_model);

    for(int x=0; x<sheetsHorizontal; x++)
    for(int y=0; y<sheetsVertical; y++)
    {
        bool sheetAdded = false;

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

            p.ApplyOffset(glm::vec2(x*margins.x*2.0f, -y*margins.y*2.0f));
            sheetAdded = true;
        }
        if(sheetAdded)
            m_rw2->AddSheet(glm::vec2(x*pageSize.x - margins.x, -(y+1)*pageSize.y + margins.y), pageSize);
    }

    m_model->LoadFromPDO(faces, edges, vertices3D, parts);
    m_model->SetMaterials(materialNames);

    for(auto it=m_textureImages.begin(); it!=m_textureImages.end(); it++)
    {
        if(it->second != nullptr)
        {
            emit UpdateTexture(it->second.get(), it->first);
        }
    }
}
