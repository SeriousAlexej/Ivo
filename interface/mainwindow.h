#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>

class CMesh;

namespace Ui {
class MainWindow;
}

class CRenWin3D;
class CRenWin2D;

class CMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CMainWindow(QWidget *parent = 0);
    ~CMainWindow();

signals:
    void ApplyTexture(QString path);

private slots:
    void Serialize(const char* filename);
    void Deserialize(const char* filename);
    void LoadModel();
    void LoadTexture();
    void on_actionModeRotate_toggled(bool arg1);
    void on_actionModeSnap_toggled(bool arg1);
    void on_actionModeMove_toggled(bool arg1);
    void on_actionModeFlaps_toggled(bool arg1);
    void on_actionModeAddSheet_toggled(bool arg1);
    void on_actionModeMoveSheet_toggled(bool arg1);
    void on_actionModeRemSheet_toggled(bool arg1);
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

private:
    Ui::MainWindow*  ui;
    CMesh*           m_model;
    CRenWin3D*       m_rw3;
    CRenWin2D*       m_rw2;
    std::string      m_openedModel = "";
};

#endif // MAINWINDOW_H
