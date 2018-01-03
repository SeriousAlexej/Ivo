#include <QMessageBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QActionGroup>
#include <QCloseEvent>
#include <QCoreApplication>
#include <cstdio>
#include <thread>
#include <mutex>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "renwin3d.h"
#include "renwin2d.h"
#include "mesh/mesh.h"
#include "settingswindow.h"
#include "settings/settings.h"
#include "scalewindow.h"
#include "interface/materialmanager.h"
#include "pdo/pdotools.h"

namespace Formats3D
{
extern QString GetSupported3DFormats();
}

static bool g_rw3IsValid = true;
static std::mutex g_rw3Mutex;

CMainWindow::CMainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_model(nullptr)
{
    ui->setupUi(this);
    m_rw3 = ui->frameLeft;
    m_rw2 = ui->frameRight;

    std::thread updateEventThread([this](){
        while(true)
        {
            g_rw3Mutex.lock();
            if(g_rw3IsValid)
            {
                g_rw3Mutex.unlock();
                QEvent* updateEvent = new QEvent((QEvent::Type)(QEvent::User + 1));
                QCoreApplication::postEvent(m_rw3, updateEvent);
                std::this_thread::sleep_for(std::chrono::milliseconds(8));
            } else {
                g_rw3Mutex.unlock();
                break;
            }
        }
    });
    updateEventThread.detach();

    QActionGroup* ag = new QActionGroup(this);
    ag->addAction(ui->actionModeMove);
    ag->addAction(ui->actionModeRotate);
    ag->addAction(ui->actionModeSnap);
    ag->addAction(ui->actionModeFlaps);
    ui->actionModeMove->setChecked(true);

    connect(ui->actionExit,         &QAction::triggered,            this,   &CMainWindow::close);
    connect(ui->actionOpen_obj,     &QAction::triggered,            this,   &CMainWindow::LoadModel);
    connect(ui->actionLoad_Texture, &QAction::triggered,            this,   &CMainWindow::OpenMaterialManager);
    connect(this,                   &CMainWindow::UpdateTexture,    m_rw3,  &IRenWin::LoadTexture);
    connect(this,                   &CMainWindow::UpdateTexture,    m_rw2,  &IRenWin::LoadTexture);
    connect(m_rw3,                  &IRenWin::RequestFullRedraw,    m_rw2,  &CRenWin2D::ClearSelection);
    connect(m_rw3,                  &IRenWin::RequestFullRedraw,    this,   &CMainWindow::UpdateView);
    connect(m_rw2,                  &IRenWin::RequestFullRedraw,    this,   &CMainWindow::UpdateView);
}

CMainWindow::~CMainWindow()
{
    delete ui;
}

void CMainWindow::closeEvent(QCloseEvent *event)
{
    g_rw3Mutex.lock();
    g_rw3IsValid = false;
    g_rw3Mutex.unlock();

    AskToSaveChanges();
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
        std::unique_ptr<CMesh> newModel(new CMesh());

        newModel->LoadMesh(modelPath);

        AskToSaveChanges();
        m_openedModel = "";
        m_model = std::move(newModel);
        SetModelToWindows();
        ClearTextures();

        m_rw2->ZoomFit();
        m_rw3->ZoomFit();
        UpdateView();
    } catch(std::exception& e)
    {
        ClearModel();
        QMessageBox::information(this, "Error", e.what());
    }
}

void CMainWindow::AskToSaveChanges()
{
    if(m_model)
    {
        if(m_model->IsModified() || m_openedModel == "")
        {
            auto result = QMessageBox::question(this, "Save changes", "Do you want to save changes to current model?",
                                                QMessageBox::Yes | QMessageBox::No);
            if(result == QMessageBox::Yes)
            {
                on_actionSave_triggered();
            }
        }
    }
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
    if(m_openedModel.empty())
    {
        m_rw2->ExportSheets("untitled");
    } else {
        m_rw2->ExportSheets(QFileInfo(m_openedModel.c_str()).baseName());
    }
}

void CMainWindow::on_actionSettings_triggered()
{
    m_rw3->releaseKeyboard();
    CSettingsWindow sw(this);
    sw.LoadSettings();
    sw.exec();
    UpdateView();
    m_rw3->grabKeyboard();
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
        m_rw2->ClearSelection();
        UpdateView();
    }
}

void CMainWindow::on_actionRedo_triggered()
{
    if(m_model)
    {
        m_model->Redo();
        m_rw2->ClearSelection();
        UpdateView();
    }
}

void CMainWindow::on_actionSave_As_triggered()
{
    if(!m_model)
        return;

    std::string filePath = QFileDialog::getSaveFileName(this,
                                                         "Save As",
                                                         "",
                                                         "Ivo model (*.ivo)").toStdString();
    if(filePath.empty())
        return;

    if(filePath.length() < 4 || filePath.substr(filePath.length()-4) != ".ivo")
    {
        filePath += ".ivo";
    }

    SaveToIVO(filePath.c_str());
}

void CMainWindow::on_actionSave_triggered()
{
    if(m_openedModel.empty())
    {
        on_actionSave_As_triggered();
    } else {
        SaveToIVO(m_openedModel.c_str());
    }
}

void CMainWindow::SetModelToWindows()
{
    m_rw2->SetModel(m_model.get());
    m_rw3->SetModel(m_model.get());
}

void CMainWindow::ClearModel()
{
    m_openedModel = "";
    m_model.reset(nullptr);
    SetModelToWindows();
    ClearTextures();
}

void CMainWindow::on_actionLoad_Model_triggered()
{
    std::string ivoModelPath = QFileDialog::getOpenFileName(this,
                                                         "Open Ivo Model",
                                                         "",
                                                         "All supported formats (*.ivo *.pdo);;Ivo model (*.ivo);;Pepakura model (*.pdo)").toStdString();
    if(ivoModelPath.empty())
        return;

    AskToSaveChanges();
    ClearModel();

    try
    {
        if(ivoModelPath.length() >=4 &&
           ivoModelPath.substr(ivoModelPath.length()-4) == ".ivo")
        {
            LoadFromIVO(ivoModelPath.c_str());
        } else {
            switch(PdoTools::GetVersionPDO(ivoModelPath.c_str()))
            {
            case 20:
                LoadFromPDOv2_0(ivoModelPath.c_str());
                break;
            default:
                QMessageBox::information(this, "Error", "Unsupported PDO format version!");
            }
        }

        m_rw2->ZoomFit();
        m_rw3->ZoomFit();
        UpdateView();

    } catch(std::exception& e)
    {
        ClearModel();
        QMessageBox::information(this, "Error", e.what());
    }
}

void CMainWindow::on_actionScale_triggered()
{
    if(m_model)
    {
        m_rw3->releaseKeyboard();
        float outScale = 1.0f;

        CScaleWindow scaleWnd(this);
        scaleWnd.SetOutputScalePtr(&outScale);
        scaleWnd.SetInitialScale(m_model->GetSizeMillimeters());
        scaleWnd.exec();

        if(outScale != 1.0f)
        {
            m_model->Scale(outScale);
        }
        UpdateView();
        m_rw3->grabKeyboard();
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
        m_model->PackGroups();
        UpdateView();
    }
}

void CMainWindow::on_actionShow_Grid_triggered(bool checked)
{
    m_rw3->ToggleGrid(checked);
    UpdateView();
}

void CMainWindow::on_actionToggle_Lighting_triggered(bool checked)
{
    m_rw3->ToggleLighting(checked);
    UpdateView();
}

void CMainWindow::on_actionPolypaint_triggered()
{
    if(ui->actionPolypaint->isChecked())
    {
        m_rw3->SetEditMode(CRenWin3D::EM_POLYPAINT);
    } else {
        m_rw3->SetEditMode(CRenWin3D::EM_NONE);
    }
}
