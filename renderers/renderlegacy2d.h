#ifndef RENDERLEGACY2D_H
#define RENDERLEGACY2D_H
#include <QOpenGLFunctions_2_0>
#include "renderbase2d.h"

class CRenderer2DLegacy : public IRenderer2D
{
public:
    CRenderer2DLegacy(QOpenGLFunctions_2_0& gl);
    virtual ~CRenderer2DLegacy();

    void    Init() override;
    void    ResizeView(int w, int h) override;

    void    PreDraw() const override;
    void    DrawScene() const override;
    void    DrawSelection(const SSelectionInfo& sinfo) const override;
    void    DrawPaperSheets(unsigned numHorizontal, unsigned numVertical) const override;

    void    RecalcProjection() override;

    QImage  DrawImageFromSheet(const glm::vec2 &pos) const override;

    void    ClearTextures() override;

private:
    void    DrawParts() const;
    void    DrawFlaps() const;
    void    DrawGroups() const;
    void    DrawEdges() const;
    void    RenderFlap(void *tr, int edge) const;
    void    RenderEdge(void *tr, int edge, int foldType) const;

    void    BindTexture(unsigned id) const;
    void    UnbindTexture() const;

    mutable int             m_boundTextureID = -1;
    QOpenGLFunctions_2_0&   m_gl;
};

#endif // RENDERLEGACY2D_H
