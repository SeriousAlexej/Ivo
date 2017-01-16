#ifndef ABSTRACTRENDERER_H
#define ABSTRACTRENDERER_H
#include <unordered_map>
#include <memory>
#include <QOpenGLTexture>

class QImage;
class CMesh;

class IAbstractRenderer
{
public:
    IAbstractRenderer() = default;
    virtual ~IAbstractRenderer();

    virtual void    SetModel(const CMesh* mdl) = 0;
    virtual void    Init() = 0;

    virtual void    PreDraw() const = 0;
    virtual void    DrawScene() const = 0;
    virtual void    PostDraw() const = 0;

    virtual void    LoadTexture(const QImage* img, unsigned index);
    virtual void    ClearTextures();

protected:
    mutable std::unordered_map<unsigned, std::unique_ptr<QOpenGLTexture>> m_textures;
    const CMesh*    m_model = nullptr;
    unsigned        m_width = 800;
    unsigned        m_height = 600;
};


#endif // ABSTRACTRENDERER_H
