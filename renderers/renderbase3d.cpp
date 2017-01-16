#include "renderbase3d.h"

void IRenderer3D::SetModel(const CMesh* mdl)
{
    IAbstractRenderer::SetModel(mdl);
}

void IRenderer3D::ToggleLighting(bool enable)
{
    m_lighting = enable;
}

void IRenderer3D::ToggleGrid(bool enable)
{
    m_grid = enable;
}

void IRenderer3D::PreDraw() const
{
    //nothing
}

void IRenderer3D::PostDraw() const
{
    //nothing
}
