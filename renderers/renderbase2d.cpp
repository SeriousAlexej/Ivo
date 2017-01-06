#include <QColor>
#include "mesh/mesh.h"
#include "renderbase2d.h"

IRenderer2D::IRenderer2D()
{
}

IRenderer2D::~IRenderer2D()
{
    ClearTextures();
    if(m_texFolds)
        m_texFolds->destroy();
}

void IRenderer2D::SetModel(const CMesh *mdl)
{
    m_model = mdl;
}

void IRenderer2D::ReserveTextureID(unsigned id)
{
    if(m_textures.find(id) == m_textures.end())
    {
        m_textures[id] = nullptr;
    }
}

void IRenderer2D::PreDraw() const
{
}

void IRenderer2D::PostDraw() const
{
}

void IRenderer2D::UpdateCameraPosition(const glm::vec3 &camPos)
{
    m_cameraPosition = camPos;
}

void IRenderer2D::CreateFoldTextures()
{
    QImage imgFolds(16, 4, QImage::Format_ARGB32); //row 0 - black, row 1 - valley, row 2 - mountain, row 3 - white
    imgFolds.fill(QColor(0,0,0,255));

    for(int i=0; i<6; ++i)
    {
        imgFolds.setPixelColor(i, 1, QColor(0,0,0,0));
        imgFolds.setPixelColor(i, 2, QColor(0,0,0,0));
    }
    for(int i=0; i<16; ++i)
    {
        imgFolds.setPixelColor(i, 3, QColor(255, 255, 255, 255));
    }
    imgFolds.setPixelColor(3, 2, QColor(0,0,0,255));

    m_texFolds.reset(new QOpenGLTexture(imgFolds));
    m_texFolds->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    m_texFolds->setWrapMode(QOpenGLTexture::Repeat);
}

void IRenderer2D::LoadTexture(const QImage *img, unsigned index)
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

void IRenderer2D::ClearTextures()
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
