#include "renderbase3d.h"

IRenderer3D::IRenderer3D()
{
}

IRenderer3D::~IRenderer3D()
{
}

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
}

void IRenderer3D::PostDraw() const
{
}
