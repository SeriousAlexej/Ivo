#ifndef RENWIN2D_H
#define RENWIN2D_H
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <QString>
#include <memory>
#include <cstdio>
#include "renwin.h"

class IRenderer2D;

class CRenWin2D : public IRenWin
{
    Q_OBJECT

public:
    enum EditMode
    {
        EM_MOVE,
        EM_ROTATE,
        EM_SNAP,
        EM_CHANGE_FLAPS
    };

    explicit CRenWin2D(QWidget *parent = nullptr);
    virtual ~CRenWin2D();

    void         SetModel(CMesh *mdl) override final;
    void         SetMode(EditMode m);
    void         ExportSheets(const QString baseName);
    void         ZoomFit() override final;

public slots:
    void         LoadTexture(const QImage *img, unsigned index) override;
    void         ClearTextures() override;
    void         ClearSelection();

protected:
    virtual void initializeGL() override final;
    virtual void paintGL() override final;
    virtual void resizeGL(int w, int h) override final;
    virtual bool event(QEvent *e) override final;

private:
    void         RecalcProjection();
    void         ModeLMB();
    void         ModeUpdate(QPointF &mpos);
    void         ModeEnd();
    glm::vec2    PointToWorldCoords(QPointF &pt) const;
    void         FillOccupiedSheetsSize(unsigned& horizontal, unsigned& vertical) const;

private:
    struct SEditInfo;

    enum
    { CAM_TRANSLATE,
      CAM_STILL,
      CAM_ZOOM,
      CAM_MODE }                    m_cameraMode = CAM_STILL;
    glm::vec3                       m_cameraPosition; //3rd component - zoom coeff.
    float                           m_w;
    float                           m_h;
    std::unique_ptr<SEditInfo>      m_editInfo;
    std::unique_ptr<IRenderer2D>    m_renderer;
};

#endif // RENWIN2D_H
