#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "notification/notification.h"

namespace Ui {
class MainWindow;
}

class CMesh;
class CRenWin3D;
class CRenWin2D;
class CActionUpdater;

class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    NOTIFICATION(ModelStateChanged);

public:
    explicit CMainWindow(QWidget* parent = 0);
    ~CMainWindow();

    bool            HasModel() const;
    const CMesh*    GetModel() const;

signals:
    void UpdateTexture(const QImage* img, unsigned index);

private slots:
    void UpdateView();
    void LoadModel();
    void OpenMaterialManager();
    void ClearTextures();
    void on_actionModeRotate_triggered();
    void on_actionModeSnap_triggered();
    void on_actionModeMove_triggered();
    void on_actionModeFlaps_triggered();
    void on_actionExport_Sheets_triggered();
    void on_actionSettings_triggered();
    void on_actionZoom_fit_triggered();
    void on_actionZoom_2D_triggered();
    void on_actionZoom_3D_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionSave_As_triggered();
    void on_actionSave_triggered();
    void on_actionLoad_Model_triggered();
    void on_actionScale_triggered();
    void on_actionAutoPack_triggered();
    void on_actionShow_Grid_triggered(bool checked);
    void on_actionToggle_Lighting_triggered(bool checked);
    void on_actionPolypaint_triggered();
    void on_actionModeSelect_triggered();
    void on_actionCloseModel_triggered();

private:
    void closeEvent(QCloseEvent *event) override;
    void RegisterUpdaters();
    void SetModelToWindows();
    void ClearModel();
    QMessageBox::StandardButton AskToSaveChanges();
    void SaveToIVO(const char* filename);
    void LoadFromIVO(const char* filename);
    void LoadFromPDOv2_0(const char* filename);

    Ui::MainWindow*                         ui;
    bool                                    m_modelModified;
    std::unique_ptr<CMesh>                  m_model;
    CRenWin3D*                              m_rw3;
    CRenWin2D*                              m_rw2;
    std::string                             m_openedModel = "";
    std::vector
        <std::unique_ptr<CActionUpdater>>   m_actionUpdaters;
    std::unordered_map
        <unsigned, std::string>             m_textures;
    std::unordered_map
        <unsigned, std::unique_ptr<QImage>> m_textureImages;
};

#endif // MAINWINDOW_H
