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
        EM_CHANGE_FLAPS,
        EM_MOVE_SHEET,
        EM_ADD_SHEET,
        EM_REM_SHEET
    };

    explicit CRenWin2D(QWidget *parent = nullptr);
    virtual ~CRenWin2D();

    void SetModel(CMesh *mdl) override final;
    void ReserveTextureID(unsigned id) override final;
    void SetMode(EditMode m);
    void ExportSheets(const QString baseName);
    void UpdateSheetsSize();
    void SerializeSheets(FILE* f) const;
    void DeserializeSheets(FILE* f);
    void ZoomFit();
    void ClearSheets() { m_sheets.clear(); }
    void AddSheet(const glm::vec2& pos, const glm::vec2& widHei);

public slots:
    void LoadTexture(const QImage *img, unsigned index) override;
    void ClearTextures() override;

protected:
    virtual void initializeGL() override final;
    virtual void paintGL() override final;
    virtual void resizeGL(int w, int h) override final;
    virtual bool event(QEvent *e) override final;

private:
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

    enum
    { CAM_TRANSLATE,
      CAM_STILL,
      CAM_ZOOM,
      CAM_MODE }                    m_cameraMode = CAM_STILL;
    glm::vec3                       m_cameraPosition; //3rd component - zoom coeff.
    float                           m_w;
    float                           m_h;
    QPointF                         m_mousePressPoint;
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
    std::unique_ptr<IRenderer2D>    m_renderer;
};

#endif // RENWIN2D_H
