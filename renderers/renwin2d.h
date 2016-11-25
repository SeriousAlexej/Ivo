#ifndef RENWIN2D_H
#define RENWIN2D_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <memory>
#include <QMutex>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <QString>
#include <cstdio>

class CMesh;

class CRenWin2D : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    enum EditMode
    {
        EM_MOVE,
        EM_ROTATE,
        EM_SNAP,
        EM_CHANGE_FLAPS,
        EM_MOVE_SHEET,
        EM_ADD_SHEET,
        EM_REM_SHEET
    };

    CRenWin2D(QWidget *parent = nullptr);
    ~CRenWin2D();

    void SetModel(CMesh *mdl);
    void SetMode(EditMode m);
    void ExportSheets(const QString baseName);
    void UpdateSheetsSize();
    void SerializeSheets(FILE* f) const;
    void DeserializeSheets(FILE* f);
    void ZoomFit();

public slots:
    void LoadTexture(QImage* img);
    void ClearTexture();

protected:
    virtual void initializeGL() override final;
    virtual void paintGL() override final;
    virtual void resizeGL(int w, int h) override final;
    virtual bool event(QEvent *e) override final;

private:
    void CreateFoldTextures();
    void RenderPaperSheets();
    void RenderSelection();
    void RenderGroups() const;
    void RenderFlaps() const;
    void RenderEdges();
    void RenderFlap(void *tr, int edge) const;
    void RenderEdge(void *tr, int edge, int foldType) const;

    void RecalcProjection();
    void ModeLMB();
    void ModeUpdate(QPointF &mpos);
    void ModeEnd();
    glm::vec2 PointToWorldCoords(QPointF &pt) const;

private:
    struct SPaperSheet
    {
        glm::vec2 m_position;
        glm::vec2 m_widthHeight;
    };

    CMesh*                          m_model;
    std::unique_ptr<QOpenGLTexture> m_texture;
    std::unique_ptr<QOpenGLTexture> m_texValleyFold;
    std::unique_ptr<QOpenGLTexture> m_texMountainFold;
    std::unique_ptr<QOpenGLTexture> m_texPitchBlack;
    glm::vec3                       m_cameraPosition; //3rd component - zoom coeff.
    float                           m_w;
    float                           m_h;
    QPointF                         m_mousePressPoint;
    enum
    { CAM_TRANSLATE,
      CAM_STILL,
      CAM_ZOOM,
      CAM_MODE }                    m_cameraMode = CAM_STILL;
    EditMode                        m_editMode = EM_MOVE;
    void*                           m_currGroup = nullptr;
    void*                           m_currTri = nullptr;
    int                             m_currEdge = -1;
    glm::vec2                       m_currEdgeVec;
    glm::vec2                       m_currGroupOldPos = glm::vec2(0.0f, 0.0f);
    float                           m_currGroupOldRot = 0.0f;
    glm::vec2                       m_fromCurrGroupCenter = glm::vec2(0.0f,0.0f);
    float                           m_currGroupLastRot = 0.0f;
    QPointF                         m_mousePosition;
    std::list<SPaperSheet>          m_sheets;
    SPaperSheet*                    m_currSheet = nullptr;
};

#endif // RENWIN2D_H
