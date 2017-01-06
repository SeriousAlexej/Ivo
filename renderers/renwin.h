#ifndef IRENWIN_H
#define IRENWIN_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class CMesh;

class IRenWin : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit IRenWin(QWidget *parent = nullptr);
    virtual ~IRenWin();

    virtual void SetModel(CMesh *mdl) = 0;
    virtual void ReserveTextureID(unsigned id) = 0;

public slots:
    virtual void LoadTexture(QImage *img, unsigned index) = 0;
    virtual void ClearTextures() = 0;

protected:
    CMesh*  m_model;
};

#endif // IRENWIN_H
