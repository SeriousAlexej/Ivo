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

extern QString g_GetSupported3DFormats();

bool g_rw3IsValid = true;
std::mutex g_rw3Mutex;

CMainWindow::CMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_model(nullptr)
{
    ui->setupUi(this);
    m_rw3 = new CRenWin3D(ui->frameLeft);
    m_rw2 = new CRenWin2D(ui->frameRight);
    {
        QHBoxLayout* l = new QHBoxLayout(ui->frameLeft);
        l->addWidget(m_rw3);
        ui->frameLeft->setLayout(l);
    }
    {
        QHBoxLayout* l = new QHBoxLayout(ui->frameRight);
        l->addWidget(m_rw2);
        ui->frameRight->setLayout(l);
    }

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

    QActionGroup *ag = new QActionGroup(this);
    ag->addAction(ui->actionModeMove);
    ag->addAction(ui->actionModeRotate);
    ag->addAction(ui->actionModeSnap);
    ag->addAction(ui->actionModeFlaps);
    ag->addAction(ui->actionModeAddSheet);
    ag->addAction(ui->actionModeMoveSheet);
    ag->addAction(ui->actionModeRemSheet);
    ui->actionModeMove->setChecked(true);

    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionOpen_obj, SIGNAL(triggered()), this, SLOT(LoadModel()));
    connect(ui->actionLoad_Texture, SIGNAL(triggered()), this, SLOT(OpenMaterialManager()));
    connect(this, SIGNAL(UpdateTexture(const QImage*, unsigned)), m_rw3, SLOT(LoadTexture(const QImage*, unsigned)));
    connect(this, SIGNAL(UpdateTexture(const QImage*, unsigned)), m_rw2, SLOT(LoadTexture(const QImage*, unsigned)));
}

CMainWindow::~CMainWindow()
{
    delete ui;
    if(m_model)
        delete m_model;
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
                                                         g_GetSupported3DFormats()).toStdString();
    if(modelPath.empty())
        return;

    try
    {
        CMesh *newModel = new CMesh();

        newModel->LoadMesh(modelPath);

        AskToSaveChanges();
        m_openedModel = "";
        m_rw2->SetModel(newModel);
        m_rw3->SetModel(newModel);
        if(m_model)
            delete m_model;
        m_model = newModel;
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
    m_rw2->SetMode(CRenWin2D::EM_ROTATE);
}

void CMainWindow::on_actionModeSnap_triggered()
{
    m_rw2->SetMode(CRenWin2D::EM_SNAP);
}

void CMainWindow::on_actionModeMove_triggered()
{
    m_rw2->SetMode(CRenWin2D::EM_MOVE);
}

void CMainWindow::on_actionModeFlaps_triggered()
{
    m_rw2->SetMode(CRenWin2D::EM_CHANGE_FLAPS);
}

void CMainWindow::on_actionModeAddSheet_triggered()
{
    m_rw2->SetMode(CRenWin2D::EM_ADD_SHEET);
}

void CMainWindow::on_actionModeMoveSheet_triggered()
{
    m_rw2->SetMode(CRenWin2D::EM_MOVE_SHEET);
}

void CMainWindow::on_actionModeRemSheet_triggered()
{
    m_rw2->SetMode(CRenWin2D::EM_REM_SHEET);
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
    CSettingsWindow sw(this);
    sw.LoadSettings();
    sw.exec();
    m_rw2->UpdateSheetsSize();
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

void CMainWindow::ClearModel()
{
    m_openedModel = "";
    if(m_model)
        delete m_model;
    m_model = nullptr;
    m_rw2->SetModel(nullptr);
    m_rw3->SetModel(nullptr);
    m_rw2->ClearSheets();
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
            switch(GetVersionPDO(ivoModelPath.c_str()))
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
