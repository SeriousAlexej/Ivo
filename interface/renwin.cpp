#include "renwin.h"

IRenWin::IRenWin(QWidget* parent) :
    QOpenGLWidget(parent), m_model(nullptr)
{
    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setVersion(2, 0);
    format.setSamples(6);
    setFormat(format);
}
