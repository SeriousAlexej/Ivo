#include <QImage>
#include "abstractrenderer.h"

IAbstractRenderer::IAbstractRenderer()
{
}

IAbstractRenderer::~IAbstractRenderer()
{
    ClearTextures();
}

void IAbstractRenderer::SetModel(const CMesh *mdl)
{
    m_model = mdl;
}

void IAbstractRenderer::LoadTexture(const QImage *img, unsigned index)
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

void IAbstractRenderer::ClearTextures()
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
