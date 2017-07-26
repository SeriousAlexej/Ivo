#ifndef IRENWIN_H
#define IRENWIN_H
#include <QOpenGLWidget>

class CMesh;

class IRenWin : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit IRenWin(QWidget *parent = nullptr);
    virtual ~IRenWin() = default;

    virtual void SetModel(CMesh *mdl) = 0;

    virtual void ZoomFit() = 0;

public slots:
    virtual void LoadTexture(const QImage *img, unsigned index) = 0;
    virtual void ClearTextures() = 0;

signals:
    void         RequestFullRedraw();

protected:
    CMesh*       m_model;
};

#endif // IRENWIN_H
