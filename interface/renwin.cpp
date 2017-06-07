#include "renwin.h"

IRenWin::IRenWin(QWidget* parent) :
    QGLWidget(parent), m_model(nullptr)
{
    QGLFormat format;
    format.setDepthBufferSize(16);
    format.setVersion(2, 0);
    format.setSamples(6);
    setFormat(format);
}
