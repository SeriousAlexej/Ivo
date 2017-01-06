#ifndef RENDERBASE3D_H
#define RENDERBASE3D_H
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/matrix.hpp>
#include <memory>
#include <unordered_map>
#include <QImage>
#include <QOpenGLTexture>

class CMesh;

class IRenderer3D
{
public:
    IRenderer3D();
    virtual ~IRenderer3D();

    virtual void    SetModel(const CMesh* mdl);
    virtual void    Init() = 0;
    virtual void    ResizeView(int w, int h, float fovy) = 0;
    virtual void    ToggleLighting(bool enable) = 0;
    virtual void    ToggleGrid(bool enable) = 0;

    virtual void    PreDraw();
    virtual void    DrawScene() = 0;
    virtual void    PostDraw();

    virtual void    UpdateViewMatrix(const glm::mat4& viewMatrix) = 0;
    virtual QImage  GetPickingTexture() const = 0;
    virtual void    ReserveTextureID(unsigned id);
    virtual void    LoadTexture(QImage* img, unsigned index);
    virtual void    ClearTextures();

protected:
    std::unordered_map<unsigned, std::unique_ptr<QOpenGLTexture>> m_textures;
    const CMesh*    m_model = nullptr;
    glm::mat4       m_viewMatrix = glm::mat4(1);
    glm::vec3       m_cameraPosition;
    bool            m_lighting = true;
    bool            m_grid = true;
    unsigned        m_width = 800;
    unsigned        m_height = 600;
};

#endif // RENDERERCLASSIC_H
