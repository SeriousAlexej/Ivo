#ifndef RENDERBASE2D_H
#define RENDERBASE2D_H
#include <QImage>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "abstractrenderer.h"

class CMesh;
struct SSelectionInfo;

class IRenderer2D : public IAbstractRenderer
{
public:
    IRenderer2D() = default;
    virtual ~IRenderer2D();

    virtual void    SetModel(const CMesh* mdl) override;
    virtual void    Init() override = 0;
    virtual void    ResizeView(int w, int h) = 0;

    virtual void    PreDraw() const override;
    virtual void    DrawScene() const override = 0;
    virtual void    DrawSelection(const SSelectionInfo& sinfo) const = 0;
    virtual void    DrawPaperSheets(unsigned numHorizontal, unsigned numVertical) const = 0;
    virtual void    PostDraw() const override;

    virtual void    UpdateCameraPosition(const glm::vec3& camPos);
    virtual void    RecalcProjection() = 0;

    virtual QImage  DrawImageFromSheet(const glm::vec2& pos) const = 0;

protected:
    void            CreateFoldTextures();

    glm::vec3       m_cameraPosition;
    mutable std::unique_ptr<QOpenGLTexture> m_texFolds;
};

#endif // RENDERBASE2D_H
