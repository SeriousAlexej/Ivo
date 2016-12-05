#ifndef IRENWIN_H
#define IRENWIN_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <memory>
#include <unordered_map>

class CMesh;

class IRenWin : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit IRenWin(QWidget *parent = nullptr);
    ~IRenWin();

    virtual void SetModel(CMesh *mdl) = 0;
    void         ReserveTextureID(unsigned id);

public slots:
    void LoadTexture(QImage *img, unsigned index);
    void ClearTextures();

protected:
    void BindTexture(unsigned id);
    void UnbindTexture();

    std::unordered_map<unsigned, std::unique_ptr<QOpenGLTexture>> m_textures;
    int                             m_boundTextureID;

    CMesh*                          m_model;
};

#endif // IRENWIN_H
