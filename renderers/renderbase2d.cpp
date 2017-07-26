#include <QColor>
#include "mesh/mesh.h"
#include "renderbase2d.h"

IRenderer2D::~IRenderer2D()
{
    m_texFolds.reset(nullptr);
}

void IRenderer2D::SetModel(const CMesh *mdl)
{
    IAbstractRenderer::SetModel(mdl);
}

void IRenderer2D::PreDraw() const
{
    //nothing
}

void IRenderer2D::PostDraw() const
{
    //nothing
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
        imgFolds.setPixel(i, 1, QColor(0,0,0,0).rgba());
        imgFolds.setPixel(i, 2, QColor(0,0,0,0).rgba());
    }
    for(int i=0; i<16; ++i)
    {
        imgFolds.setPixel(i, 3, QColor(255, 255, 255, 255).rgba());
    }
    imgFolds.setPixel(3, 2, QColor(0,0,0,255).rgba());

    m_texFolds.reset(new QOpenGLTexture(imgFolds));
    m_texFolds->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    m_texFolds->setWrapMode(QOpenGLTexture::Repeat);
}
