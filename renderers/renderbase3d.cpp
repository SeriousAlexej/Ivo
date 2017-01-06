#include "renderbase3d.h"

IRenderer3D::IRenderer3D()
{
}

IRenderer3D::~IRenderer3D()
{
    ClearTextures();
}

void IRenderer3D::SetModel(const CMesh* mdl)
{
    m_model = mdl;
}

void IRenderer3D::ToggleLighting(bool enable)
{
    m_lighting = enable;
}

void IRenderer3D::ToggleGrid(bool enable)
{
    m_grid = enable;
}

void IRenderer3D::ReserveTextureID(unsigned id)
{
    if(m_textures.find(id) == m_textures.end())
    {
        m_textures[id] = nullptr;
    }
}

void IRenderer3D::PreDraw() const
{
}

void IRenderer3D::PostDraw() const
{
}

void IRenderer3D::LoadTexture(const QImage *img, unsigned index)
{
    if(!img)
    {
        m_textures[index].reset(nullptr);
    } else {
        if(m_textures[index])
            m_textures[index]->destroy();
        m_textures[index].reset(new QOpenGLTexture(*img));
        m_textures[index]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_textures[index]->setMagnificationFilter(QOpenGLTexture::Linear);
        m_textures[index]->setWrapMode(QOpenGLTexture::Repeat);
    }
}

void IRenderer3D::ClearTextures()
{
    for(auto it=m_textures.begin(); it!=m_textures.end(); it++)
    {
        std::unique_ptr<QOpenGLTexture>& texPtr = it->second;
        if(texPtr)
        {
            texPtr->destroy();
            texPtr.reset(nullptr);
        }
    }
    m_textures.clear();
}
