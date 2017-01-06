#ifndef RENDERLEGACY2D_H
#define RENDERLEGACY2D_H
#include "renderbase2d.h"

class CRenderer2DLegacy : public IRenderer2D
{
public:
    CRenderer2DLegacy();
    virtual ~CRenderer2DLegacy();

    void    Init() override;
    void    ResizeView(int w, int h) override;

    void    PreDraw() override;
    void    DrawScene() override;
    void    DrawSelection(const SSelectionInfo& sinfo) const override;
    void    DrawPaperSheet(const glm::vec2 &position, const glm::vec2 &size) const override;

    void    RecalcProjection() override;

    QImage  DrawImageFromSheet(const glm::vec2 &pos) override;

    void    ClearTextures() override;

private:
    void    DrawParts();
    void    DrawFlaps() const;
    void    DrawGroups();
    void    DrawEdges() const;
    void    RenderFlap(void *tr, int edge) const;
    void    RenderEdge(void *tr, int edge, int foldType) const;

    void    BindTexture(unsigned id);
    void    UnbindTexture();

    int     m_boundTextureID = -1;
};

#endif // RENDERLEGACY2D_H
