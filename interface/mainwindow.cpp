#include <QMessageBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QActionGroup>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "renderers/renwin3d.h"
#include "renderers/renwin2d.h"
#include "mesh/mesh.h"
#include "settingswindow.h"

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
    connect(ui->actionLoad_Texture, SIGNAL(triggered()), this, SLOT(LoadTexture()));
    connect(this, SIGNAL(ApplyTexture(QString)), m_rw3, SLOT(LoadTexture(QString)));
    connect(this, SIGNAL(ApplyTexture(QString)), m_rw2, SLOT(LoadTexture(QString)));
    connect(ui->actionRemove_Texture, SIGNAL(triggered()), m_rw2, SLOT(ClearTexture()));
    connect(ui->actionRemove_Texture, SIGNAL(triggered()), m_rw3, SLOT(ClearTexture()));
}

CMainWindow::~CMainWindow()
{
    delete ui;
    if(m_model)
        delete m_model;
}

void CMainWindow::LoadTexture()
{
    QString texturePath = QFileDialog::getOpenFileName(this,
                                                       "Load Texture",
                                                       "",
                                                       "Image file (*.png *.bmp *.jpg)");
    if(texturePath.isEmpty())
        return;
    emit ApplyTexture(texturePath);
}

void CMainWindow::LoadModel()
{
    std::string modelPath = QFileDialog::getOpenFileName(this,
                                                         "Open Model",
                                                         "",
                                                         "3D Mesh (*.obj)").toStdString();
    if(modelPath.empty())
        return;

    CMesh *newModel = new CMesh();
    if(!newModel->LoadMesh(modelPath))
    {
        delete newModel;
        QMessageBox::information(this, "Error", "Could not open model!");
    } else {
        m_rw2->SetModel(newModel);
        m_rw3->SetModel(newModel);
        if(m_model)
            delete m_model;
        m_model = newModel;
        update();
    }
}

void CMainWindow::on_actionModeRotate_toggled(bool arg1)
{
    m_rw2->SetMode(CRenWin2D::EM_ROTATE);
}

void CMainWindow::on_actionModeSnap_toggled(bool arg1)
{
    m_rw2->SetMode(CRenWin2D::EM_SNAP);
}

void CMainWindow::on_actionModeMove_toggled(bool arg1)
{
    m_rw2->SetMode(CRenWin2D::EM_MOVE);
}

void CMainWindow::on_actionModeFlaps_toggled(bool arg1)
{
    m_rw2->SetMode(CRenWin2D::EM_CHANGE_FLAPS);
}

void CMainWindow::on_actionModeAddSheet_toggled(bool arg1)
{
    m_rw2->SetMode(CRenWin2D::EM_ADD_SHEET);
}

void CMainWindow::on_actionModeMoveSheet_toggled(bool arg1)
{
    m_rw2->SetMode(CRenWin2D::EM_MOVE_SHEET);
}

void CMainWindow::on_actionModeRemSheet_toggled(bool arg1)
{
    m_rw2->SetMode(CRenWin2D::EM_REM_SHEET);
}

void CMainWindow::on_actionExport_Sheets_triggered()
{
    m_rw2->ExportSheets();
}

void CMainWindow::on_actionSettings_triggered()
{
    CSettingsWindow sw(this);
    sw.LoadSettings();
    sw.exec();
    m_rw2->UpdateSheetsSize();
    update();
}

void CMainWindow::on_actionZoom_fit_triggered()
{
    m_rw2->ZoomFit();
    m_rw3->ZoomFit();
}
