/*
    Ivo - a free software for unfolding 3D models and papercrafting
    Copyright (C) 2015-2018 Oleksii Sierov (seriousalexej@gmail.com)
	
    Ivo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ivo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Ivo.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "notification/notification.h"
#include "notification/subscriber.h"

namespace Ui {
class MainWindow;
}

namespace tt {
class TabToolbar;
}

class CMesh;
class CRenWin3D;
class CRenWin2D;
class CActionUpdater;

class CMainWindow : public QMainWindow, public Subscriber
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
    void on_actionShow_Edges_triggered(bool checked);
    void on_actionShow_Flaps_triggered(bool checked);
    void on_actionShow_Folds_triggered(bool checked);
    void on_actionShow_Texture_triggered(bool checked);

protected:
    virtual void changeEvent(QEvent* event) override;

private:
    void closeEvent(QCloseEvent *event) override;
    void RegisterUpdaters();
    void SetModelToWindows();
    void ClearModel();
    void OpenHelp() const;
    QMessageBox::StandardButton AskToSaveChanges();
    void SaveToIVO(const QString& filename);
    void LoadFromIVO(const QString& filename);
    void LoadFromPDOv2_0(const QString& filename);
    void UpdateStyle();
    void SetupGUI();

    Ui::MainWindow*                         ui;
    bool                                    m_modelModified;
    std::unique_ptr<CMesh>                  m_model;
    CRenWin3D*                              m_rw3;
    CRenWin2D*                              m_rw2;
    tt::TabToolbar*                         m_tabToolbar;
    QString                                 m_openedModel = "";
    std::vector
        <std::unique_ptr<CActionUpdater>>   m_actionUpdaters;
    std::unordered_map
        <unsigned, std::string>             m_textures;
    std::unordered_map
        <unsigned, std::unique_ptr<QImage>> m_textureImages;
};

#endif // MAINWINDOW_H
