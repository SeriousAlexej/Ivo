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
#include <QMessageBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QActionGroup>
#include <QCloseEvent>
#include <QToolButton>
#include <QEvent>
#include <QDesktopServices>
#include <cstdio>
#include <cstddef>
#include <TabToolbar/TabToolbar.h>
#include <TabToolbar/StyleTools.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "renwin3d.h"
#include "renwin2d.h"
#include "mesh/mesh.h"
#include "settingswindow.h"
#include "settings/settings.h"
#include "scalewindow.h"
#include "interface/materialmanager.h"
#include "interface/actionupdater.h"
#include "interface/exportwindow.h"
#include "interface/importwindow.h"
#include "pdo/pdotools.h"
#include "notification/hub.h"

namespace Formats3D
{
extern QString GetSupported3DFormats();
}

static QString FormatColor(const QColor& col)
{
    return QString("rgba(%1, %2, %3, %4)")
                      .arg(col.red())
                      .arg(col.green())
                      .arg(col.blue())
                      .arg(col.alpha());
}

static QString FormatColor(const std::vector<tt::Color>& colors)
{
    const std::size_t sz = colors.size();
    if(sz == 1)
        return FormatColor(colors[0].value);

    QString result = "qlineargradient(x1:0, y1:1, x2:0, y2:0";
    for(const tt::Color& col : colors)
        result += QString(", stop:") + QString::number(col.coefficient) + " " + FormatColor(col.value);
    result += ")";

    return result;
}

CMainWindow::CMainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_modelModified(false),
    m_model(nullptr)
{
    ui->setupUi(this);

    setCentralWidget(nullptr);
    m_rw3 = ui->frameLeft;
    m_rw2 = ui->frameRight;

    QActionGroup* ag = new QActionGroup(this);
    ag->addAction(ui->actionModeMove);
    ag->addAction(ui->actionModeRotate);
    ag->addAction(ui->actionModeSnap);
    ag->addAction(ui->actionModeFlaps);
    ag->addAction(ui->actionModeSelect);
    ui->actionModeMove->setChecked(true);

    connect(ui->actionOpen_obj,     &QAction::triggered,         this,  &CMainWindow::LoadModel);
    connect(ui->actionLoad_Texture, &QAction::triggered,         this,  &CMainWindow::OpenMaterialManager);
    connect(ui->actionAbout,        &QAction::triggered,         this,  &CMainWindow::OpenHelp);
    connect(this,                   &CMainWindow::UpdateTexture, m_rw3, &IRenWin::LoadTexture);
    connect(this,                   &CMainWindow::UpdateTexture, m_rw2, &IRenWin::LoadTexture);
    connect(m_rw3,                  &IRenWin::RequestFullRedraw, this,  &CMainWindow::UpdateView);
    connect(m_rw2,                  &IRenWin::RequestFullRedraw, this,  &CMainWindow::UpdateView);

    Subscribe<CSettings::Changed>(&CMainWindow::UpdateView);

    SetupGUI();
    RegisterUpdaters();
    for(auto& updater : m_actionUpdaters)
        updater->RequestUpdate();

    QFile stateFile("LayoutState");
    if(stateFile.open(QIODevice::ReadOnly))
    {
        int32_t sz;
        if(stateFile.read((char*)&sz, sizeof(sz)) != sizeof(sz))
            return;
        QByteArray geom(sz, 0);
        if(stateFile.read(geom.data(), geom.size()) != geom.size())
            return;
        if(stateFile.read((char*)&sz, sizeof(sz)) != sizeof(sz))
            return;
        QByteArray state(sz, 0);
        if(stateFile.read(state.data(), state.size()) != state.size())
            return;

        restoreGeometry(geom);
        restoreState(state);
    }
}

CMainWindow::~CMainWindow()
{
    CSettings& sett = CSettings::GetInstance();
    sett.ttCollapsed = m_tabToolbar->HideAction()->isChecked();
    sett.ttStyle = m_tabToolbar->GetStyle();

    QFile stateFile("LayoutState");
    if(stateFile.open(QIODevice::WriteOnly))
    {
        QByteArray geom = saveGeometry();
        int32_t sz = geom.size();
        stateFile.write((char*)&sz, sizeof(sz));
        stateFile.write(geom);
        QByteArray state = saveState();
        sz = state.size();
        stateFile.write((char*)&sz, sizeof(sz));
        stateFile.write(state);
    }

    m_actionUpdaters.clear();
    delete ui;
}

void CMainWindow::UpdateStyle()
{
    std::unique_ptr<tt::StyleParams> style = tt::CreateStyle(m_tabToolbar->GetStyle());
    const QString colorStr = FormatColor(style->ToolbarBackgroundColor);
    const QString borderStr = FormatColor(style->BorderColor);
    const QString dockStyle = QString(".QDockWidget::title {border: 1px solid " + borderStr + "; background-color: ") + colorStr + "; padding-top: 1px; margin-top: 2px; }";
    ui->dockWidget2D->setStyleSheet(dockStyle);
    ui->dockWidget3D->setStyleSheet(dockStyle);
}

void CMainWindow::changeEvent(QEvent* event)
{
    if(event->type() == QEvent::StyleChange)
        QTimer::singleShot(1000, [this]{ UpdateStyle(); });

    QMainWindow::changeEvent(event);
}

bool CMainWindow::HasModel() const
{
    return m_model != nullptr;
}

const CMesh* CMainWindow::GetModel() const
{
    return m_model.get();
}

void CMainWindow::closeEvent(QCloseEvent* event)
{
    const auto decision = AskToSaveChanges();
    if(decision == QMessageBox::Cancel)
    {
        event->ignore();
        return;
    }
    event->accept();
}

void CMainWindow::OpenMaterialManager()
{
    if(!m_model)
        return;
    CMaterialManager matMngr(this);
    matMngr.SetMaterials(m_model->GetMaterials(), m_textures);
    matMngr.exec();

    auto newTextures = matMngr.GetTextures();
    for(auto it=newTextures.begin(); it!=newTextures.end(); it++)
    {
        if(m_textures[it->first] != it->second)
        {
            m_textures[it->first] = it->second;
            if(!it->second.empty())
            {
                m_textureImages[it->first].reset(new QImage(it->second.c_str()));
                (*(m_textureImages[it->first])) = m_textureImages[it->first]->convertToFormat(QImage::Format_RGB32);
            } else {
                m_textureImages[it->first].reset(nullptr);
            }
            emit UpdateTexture(m_textureImages[it->first].get(), it->first);
        }
    }
}

void CMainWindow::ClearTextures()
{
    m_textures.clear();
    m_textureImages.clear();
    m_rw2->ClearTextures();
    m_rw3->ClearTextures();

    if(m_model)
    {
        auto& materials = m_model->GetMaterials();
        for(auto it=materials.begin(); it!=materials.end(); it++)
        {
            m_textures[it->first] = "";
            m_textureImages[it->first] = nullptr;
        }
    }
}

void CMainWindow::LoadModel()
{
    std::string modelPath = QFileDialog::getOpenFileName(this,
                                                         "Open Model",
                                                         "",
                                                         Formats3D::GetSupported3DFormats()).toStdString();
    if(modelPath.empty())
        return;

    try
    {
        const auto decision = AskToSaveChanges();
        if(decision == QMessageBox::Cancel)
            return;

        CImportWindow importSettings(this);
        importSettings.exec();
        if(importSettings.result() != QDialog::Accepted)
            return;

        std::unique_ptr<CMesh> newModel(new CMesh());

        newModel->LoadMesh(modelPath);

        m_openedModel = "";
        m_model = std::move(newModel);
        SetModelToWindows();
        ClearTextures();

        m_rw2->ZoomFit();
        m_rw3->ZoomFit();
        UpdateView();

        NOTIFY(ModelStateChanged);
    } catch(std::exception& e)
    {
        ClearModel();
        QMessageBox::information(this, "Error", e.what());
    }
}

QMessageBox::StandardButton CMainWindow::AskToSaveChanges()
{
    QMessageBox::StandardButton result = QMessageBox::No;

    if(m_model)
    {
        if(m_modelModified || m_openedModel == "")
        {
            result = QMessageBox::question(this, "Save changes", "Do you want to save changes to current model?",
                                                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if(result == QMessageBox::Yes)
                on_actionSave_triggered();
        }
    }

    return result;
}

void CMainWindow::on_actionModeRotate_triggered()
{
    m_rw2->SetMode(CRenWin2D::EditMode::Rotate);
}

void CMainWindow::on_actionModeSnap_triggered()
{
    m_rw2->SetMode(CRenWin2D::EditMode::Snap);
}

void CMainWindow::on_actionModeMove_triggered()
{
    m_rw2->SetMode(CRenWin2D::EditMode::Move);
}

void CMainWindow::on_actionModeFlaps_triggered()
{
    m_rw2->SetMode(CRenWin2D::EditMode::Flaps);
}

void CMainWindow::on_actionExport_Sheets_triggered()
{
    CExportWindow exportSettings(this);
    exportSettings.exec();
    if(exportSettings.result() != QDialog::Accepted)
        return;

    if(m_openedModel.isEmpty())
        m_rw2->ExportSheets("untitled");
    else
        m_rw2->ExportSheets(QFileInfo(m_openedModel).baseName());
}

void CMainWindow::on_actionSettings_triggered()
{
    CSettingsWindow sw(this);
    sw.LoadSettings();
    sw.exec();
    UpdateView();
}

void CMainWindow::on_actionZoom_fit_triggered()
{
    m_rw2->ZoomFit();
    m_rw3->ZoomFit();
}

void CMainWindow::on_actionZoom_2D_triggered()
{
    m_rw2->ZoomFit();
}

void CMainWindow::on_actionZoom_3D_triggered()
{
    m_rw3->ZoomFit();
}

void CMainWindow::on_actionUndo_triggered()
{
    if(m_model)
    {
        m_model->Undo();
        UpdateView();
    }
}

void CMainWindow::on_actionRedo_triggered()
{
    if(m_model)
    {
        m_model->Redo();
        UpdateView();
    }
}

void CMainWindow::on_actionSave_As_triggered()
{
    if(!m_model)
        return;

    QString filePath = QFileDialog::getSaveFileName(this, "Save As", "", "Ivo model (*.ivo)");
    if(filePath.isEmpty())
        return;

    if(filePath.length() < 4 || filePath.right(4) != ".ivo")
        filePath += ".ivo";

    SaveToIVO(filePath);
}

void CMainWindow::on_actionSave_triggered()
{
    if(m_openedModel.isEmpty())
        on_actionSave_As_triggered();
    else
        SaveToIVO(m_openedModel);
}

void CMainWindow::SetModelToWindows()
{
    m_rw2->SetModel(m_model.get());
    m_rw3->SetModel(m_model.get());
}

void CMainWindow::OpenHelp() const
{
    QDesktopServices::openUrl(QUrl("https://github.com/SeriousAlexej/Ivo"));
}

void CMainWindow::ClearModel()
{
    m_modelModified = false;
    m_openedModel = "";
    m_model.reset(nullptr);
    SetModelToWindows();
    ClearTextures();

    NOTIFY(ModelStateChanged);
}

void CMainWindow::on_actionLoad_Model_triggered()
{
    QString ivoModelPath = QFileDialog::getOpenFileName(this, "Open Ivo Model", "", "All supported formats (*.ivo *.pdo);;Ivo model (*.ivo);;Pepakura model (*.pdo)");
    if(ivoModelPath.isEmpty())
        return;

    const auto decision = AskToSaveChanges();
    if(decision == QMessageBox::Cancel)
        return;

    ClearModel();

    try
    {
        if(ivoModelPath.length() >=4 &&
           ivoModelPath.right(4) == ".ivo")
        {
            LoadFromIVO(ivoModelPath);
        } else {
            switch(PdoTools::GetVersionPDO(ivoModelPath))
            {
            case 20:
                LoadFromPDOv2_0(ivoModelPath);
                break;
            default:
                QMessageBox::information(this, "Error", "Unsupported PDO format version!");
            }
        }

        m_rw2->ZoomFit();
        m_rw3->ZoomFit();
        UpdateView();

        NOTIFY(ModelStateChanged);
    } catch(std::exception& e)
    {
        ClearModel();
        QMessageBox::warning(this, "Error", e.what());
    }
}

void CMainWindow::on_actionScale_triggered()
{
    if(m_model)
    {
        float outScale = 1.0f;

        CScaleWindow scaleWnd(outScale, m_model->GetSizeMillimeters(), this);
        scaleWnd.exec();

        if(outScale != 1.0f)
            m_model->Scale(outScale);

        UpdateView();
    }
}

void CMainWindow::UpdateView()
{
    m_rw2->update();
    m_rw3->update();
    update();
}

void CMainWindow::on_actionAutoPack_triggered()
{
    if(m_model)
    {
        if(!m_model->PackGroups())
            QMessageBox::warning(this, "Warning", QString("Not all parts could be automatically arranged!\n") +
                                                  QString("Some parts have been placed at the top left corner of the first sheet of paper.\n") +
                                                  QString("Consider breaking them into smaller pieces, increasing paper size or shrinking margins."));
        UpdateView();
    }
}

void CMainWindow::on_actionPolypaint_triggered()
{
    if(ui->actionPolypaint->isChecked())
        m_rw3->SetEditMode(CRenWin3D::EM_POLYPAINT);
    else
        m_rw3->SetEditMode(CRenWin3D::EM_NONE);
}

void CMainWindow::on_actionModeSelect_triggered()
{
    m_rw2->SetMode(CRenWin2D::EditMode::Select);
}

void CMainWindow::on_actionCloseModel_triggered()
{
    const auto decision = AskToSaveChanges();
    if(decision != QMessageBox::Cancel)
        ClearModel();
}

void CMainWindow::on_actionShow_Edges_triggered(bool checked)
{
    CSettings::GetInstance().SetRenderFlagState(CSettings::R_EDGES, checked);
}

void CMainWindow::on_actionShow_Flaps_triggered(bool checked)
{
    CSettings::GetInstance().SetRenderFlagState(CSettings::R_FLAPS, checked);
}

void CMainWindow::on_actionShow_Folds_triggered(bool checked)
{
    CSettings::GetInstance().SetRenderFlagState(CSettings::R_FOLDS, checked);
}

void CMainWindow::on_actionShow_Texture_triggered(bool checked)
{
    CSettings::GetInstance().SetRenderFlagState(CSettings::R_TEXTR, checked);
}

void CMainWindow::on_actionShow_Grid_triggered(bool checked)
{
    CSettings::GetInstance().SetRenderFlagState(CSettings::R_GRID, checked);
}

void CMainWindow::on_actionToggle_Lighting_triggered(bool checked)
{
    CSettings::GetInstance().SetRenderFlagState(CSettings::R_LIGHT, checked);
}
