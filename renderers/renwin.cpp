#include "renwin.h"
#include "settings/settings.h"

IRenWin::IRenWin(QWidget* parent) :
    QOpenGLWidget(parent), m_model(nullptr)
{
    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setMajorVersion(2);
    format.setMinorVersion(0);
    format.setSamples(6);
    setFormat(format);
    m_boundTextureID = -1;
}

IRenWin::~IRenWin()
{
    makeCurrent();
    for(auto it=m_textures.begin(); it!=m_textures.end(); it++)
    {
        if(it->second)
        {
            it->second->destroy();
        }
    }
    doneCurrent();
}

void IRenWin::ReserveTextureID(unsigned id)
{
    if(m_textures.find(id) == m_textures.end())
    {
        m_textures[id] = nullptr;
    }
}

void IRenWin::LoadTexture(QImage *img, unsigned index)
{
    makeCurrent();
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
    update();
}

void IRenWin::ClearTextures()
{
    makeCurrent();
    for(auto it=m_textures.begin(); it!=m_textures.end(); it++)
    {
        if(it->second)
        {
            it->second->destroy();
            it->second.reset(nullptr);
        }
    }
    m_textures.clear();
    m_boundTextureID = -1;
    update();
}

void IRenWin::BindTexture(unsigned id)
{
    const bool renTexture = CSettings::GetInstance().GetRenderFlags() & CSettings::R_TEXTR;
    if(renTexture && m_boundTextureID != (int)id)
    {
        glEnd();
        if(m_textures[id])
        {
            m_textures[id]->bind();
        } else if(m_boundTextureID >= 0 && m_textures[m_boundTextureID])
        {
            m_textures[m_boundTextureID]->release();
        }
        glBegin(GL_TRIANGLES);
        m_boundTextureID = id;
    }
}

void IRenWin::UnbindTexture()
{
    const bool renTexture = CSettings::GetInstance().GetRenderFlags() & CSettings::R_TEXTR;
    if(renTexture)
    {
        for(auto it=m_textures.begin(); it!=m_textures.end(); it++)
        {
            if(m_textures[it->first] && m_textures[it->first]->isBound())
            {
                m_textures[it->first]->release();
                break;
            }
        }
        m_boundTextureID = -1;
    }
}
