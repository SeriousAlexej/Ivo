#ifndef RENDERBASE2D_H
#define RENDERBASE2D_H
#include <unordered_map>
#include <memory>
#include <QOpenGLTexture>
#include <QImage>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class CMesh;

struct SSelectionInfo
{
    glm::vec2   m_mouseWorldPos;
    int         m_editMode;
    void*       m_group;
    void*       m_triangle;
    int         m_edge;
};

class IRenderer2D
{
public:
    IRenderer2D();
    virtual ~IRenderer2D();

    virtual void    SetModel(const CMesh* mdl);
    virtual void    Init() = 0;
    virtual void    ResizeView(int w, int h) = 0;

    virtual void    PreDraw();
    virtual void    DrawScene() = 0;
    virtual void    DrawSelection(const SSelectionInfo& sinfo) const = 0;
    virtual void    DrawPaperSheet(const glm::vec2 &position, const glm::vec2 &size) const = 0;
    virtual void    PostDraw();

    virtual void    UpdateCameraPosition(const glm::vec3& camPos);
    virtual void    RecalcProjection() = 0;
    virtual void    ReserveTextureID(unsigned id);
    virtual void    LoadTexture(QImage* img, unsigned index);
    virtual void    ClearTextures();

    virtual QImage  DrawImageFromSheet(const glm::vec2& pos) = 0;

protected:
    void            CreateFoldTextures();

    std::unique_ptr<QOpenGLTexture> m_texFolds;
    std::unordered_map<unsigned, std::unique_ptr<QOpenGLTexture>> m_textures;
    const CMesh*    m_model = nullptr;
    glm::vec3       m_cameraPosition;
    unsigned        m_width = 800;
    unsigned        m_height = 600;
};

#endif // RENDERBASE2D_H
