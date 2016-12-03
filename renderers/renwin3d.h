#ifndef RENWIN3D_H
#define RENWIN3D_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <memory>
#include <unordered_map>
#include <QMutex>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/matrix.hpp>

class CMesh;

class CRenWin3D : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    CRenWin3D(QWidget *parent = nullptr);
    ~CRenWin3D();
    void SetModel(CMesh *mdl);
    void ZoomFit();
    void ReserveTextureID(unsigned id);

public slots:
    void LoadTexture(QImage* img, unsigned index);
    void ClearTextures();

protected:
    virtual void initializeGL() override final;
    virtual void paintGL() override final;
    virtual void resizeGL(int w, int h) override final;
    bool event(QEvent *e) override final;

private:
    void DrawGrid();
    void DrawAxis();
    void UpdateViewAngles();
    void UpdateViewMatrix();

    std::unordered_map<unsigned, std::unique_ptr<QOpenGLTexture>> m_textures;
    int                             m_boundTextureID;

    CMesh*                          m_model;
    QPointF                         m_mousePressPoint;
    enum
    { CAM_TRANSLATE,
      CAM_ROTATE,
      CAM_STILL,
      CAM_ZOOM }    m_cameraMode = CAM_STILL;
    glm::vec3       m_cameraPosition = glm::vec3(0.0f,0.0f,0.0f);
    glm::vec2       m_cameraRotation = glm::vec2(0.0f,0.0f);
    glm::mat4       m_viewMatrix = glm::mat4(1);
    glm::vec3       m_right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3       m_front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3       m_up = glm::vec3(0.0f, 1.0f, 0.0f);
    float           m_fovy = 70.0f;
};

#endif // RENWIN3D_H
