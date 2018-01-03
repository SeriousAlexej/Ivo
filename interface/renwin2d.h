#ifndef RENWIN2D_H
#define RENWIN2D_H
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <QString>
#include <memory>
#include <cstdio>
#include <unordered_map>
#include <functional>
#include "renwin.h"

#define MODE_FUNC_START(mode) mode ## Start
#define MODE_FUNC_UPDATE(mode) mode ## Update
#define MODE_FUNC_END(mode) mode ## End

class IRenderer2D;

class CRenWin2D : public IRenWin
{
Q_OBJECT
#define RENWIN_MODE(mode)\
    void MODE_FUNC_START(mode) ();\
    void MODE_FUNC_UPDATE(mode) ();\
    void MODE_FUNC_END(mode) ();
#include "interface/modes2D/renwin2DModes.h"
#undef RENWIN_MODE

public:
    enum class EditMode
    {
#define RENWIN_MODE(mode) mode,
#include "interface/modes2D/renwin2DModes.h"
#undef RENWIN_MODE
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
    void         ModeUpdate();
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

private:
    struct SModeFuncs
    {
        std::function<void()> start;
        std::function<void()> end;
        std::function<void()> update;
    };
    std::unordered_map<int, SModeFuncs> m_modeFunctions;
};

#endif // RENWIN2D_H
