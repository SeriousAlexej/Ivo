#ifndef RENDERLEGACY3D_H
#define RENDERLEGACY3D_H
#include "renderbase3d.h"

class CMesh;

class CRenderer3DLegacy : public IRenderer3D
{
public:
    CRenderer3DLegacy();
    virtual ~CRenderer3DLegacy();

    void    Init() override;
    void    ResizeView(int w, int h, float fovy) override;
    void    ToggleLighting(bool enable) override;
    void    ToggleGrid(bool enable) override;

    void    DrawScene() const override;

    void    UpdateViewMatrix(const glm::mat4& viewMatrix) override;

    QImage  GetPickingTexture() const override;

    void    ClearTextures() override;

private:
    void    DrawModel() const;
    void    DrawBackground() const;
    void    DrawGrid() const;
    void    DrawAxis() const;

    void    BindTexture(unsigned id) const;
    void    UnbindTexture() const;

    mutable int m_boundTextureID = -1;
};

#endif // RENDERLEGACY3D_H
