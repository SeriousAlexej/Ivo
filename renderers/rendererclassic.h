#ifndef RENDERERCLASSIC_H
#define RENDERERCLASSIC_H

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/matrix.hpp>
#include <unordered_set>
#include <QImage>
#include <QOpenGLTexture>

class CMesh;

class Renderer3D
{
public:
    void SetModel(CMesh *mdl);
    void Init();
    void Init2D();
    void ResizeView(int w, int h);
    void ToggleLighting(bool enable);

    void DrawModel(std::unordered_set<int> pickTriIndices);
    void DrawBackground();
    void DrawGrid(glm::vec3 cameraPosition);
    void DrawAxis(glm::mat4 rotMx);
    void UpdateViewMatrix(glm::mat4 viewMatrix);
    void SetLightPosition(glm::vec3 position);

    QImage* RefreshPickingTexture();

    void DrawFlaps() const;
    void DrawEdges();
    void DrawPaperSheet(glm::vec2 m_position, glm::vec2 m_widthHeight);
    void DrawGroups();

    //private
    void LoadTexture(QImage *img, unsigned index);
    void BindTexture(unsigned id);
    void UnbindTexture();
    void CreateFoldTextures();
    void RenderFlap(void *tr, int edge) const;
    void RenderEdge(void *tr, int edge, int foldType) const;

    CMesh* m_model;
    std::unordered_map<unsigned, std::unique_ptr<QOpenGLTexture>> m_textures;
    int m_boundTextureID;
    std::unique_ptr<QOpenGLTexture> m_texFolds;

    glm::mat4       m_viewMatrix = glm::mat4(1);
    float           m_fovy = 70.0f;
    glm::vec4       m_lightPosition;
    bool            m_lighting = true;
    QImage          m_pickingTexture;
    bool            m_pickTexValid = false;
    unsigned        m_width = 800;
    unsigned        m_height = 600;
};

#endif // RENDERERCLASSIC_H
