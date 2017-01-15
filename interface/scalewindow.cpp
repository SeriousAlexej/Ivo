#include "scalewindow.h"
#include "ui_scalewindow.h"

CScaleWindow::CScaleWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ScaleWindow),
    m_currentScale(1.0f),
    m_likeAMutex(false),
    m_outputScale(nullptr)
{
    ui->setupUi(this);
}

CScaleWindow::~CScaleWindow()
{
    delete ui;
}

void CScaleWindow::SetOutputScalePtr(float *scaPtr)
{
    m_outputScale = scaPtr;
}

void CScaleWindow::SetInitialScale(glm::vec3 initSca)
{
    m_initialScale = initSca;
    ResetUI();
}

void CScaleWindow::ResetUI()
{
    if(m_likeAMutex)
        return;
    m_likeAMutex = true;

    m_currentScale = 1.0f;
    ui->spinPrecent->setValue(100);
    ui->spinWidth->setValue(m_initialScale.x);
    ui->spinHeight->setValue(m_initialScale.y);
    ui->spinLength->setValue(m_initialScale.z);
    m_likeAMutex = false;
}

void CScaleWindow::on_buttonBox_accepted()
{
    if(m_outputScale)
    {
        *m_outputScale = m_currentScale;
    }
    close();
}

void CScaleWindow::on_buttonBox_rejected()
{
    close();
}

void CScaleWindow::on_spinPrecent_valueChanged(int i)
{
    Q_UNUSED(i);
    if(m_likeAMutex)
        return;
    m_likeAMutex = true;

    m_currentScale = 0.01f * ui->spinPrecent->value();
    ui->spinWidth->setValue(m_initialScale.x * m_currentScale);
    ui->spinHeight->setValue(m_initialScale.y * m_currentScale);
    ui->spinLength->setValue(m_initialScale.z * m_currentScale);
    m_likeAMutex = false;
}

void CScaleWindow::on_spinWidth_valueChanged(double i)
{
    Q_UNUSED(i);
    if(m_likeAMutex)
        return;
    m_likeAMutex = true;

    m_currentScale = ui->spinWidth->value() / m_initialScale.x;
    ui->spinPrecent->setValue((int)(100.0f * m_currentScale));
    ui->spinHeight->setValue(m_initialScale.y * m_currentScale);
    ui->spinLength->setValue(m_initialScale.z * m_currentScale);
    m_likeAMutex = false;
}

void CScaleWindow::on_spinHeight_valueChanged(double i)
{
    Q_UNUSED(i);
    if(m_likeAMutex)
        return;
    m_likeAMutex = true;

    m_currentScale = ui->spinHeight->value() / m_initialScale.y;
    ui->spinPrecent->setValue((int)(100.0f * m_currentScale));
    ui->spinWidth->setValue(m_initialScale.x * m_currentScale);
    ui->spinLength->setValue(m_initialScale.z * m_currentScale);
    m_likeAMutex = false;
}

void CScaleWindow::on_spinLength_valueChanged(double i)
{
    Q_UNUSED(i);
    if(m_likeAMutex)
        return;
    m_likeAMutex = true;

    m_currentScale = ui->spinLength->value() / m_initialScale.z;
    ui->spinPrecent->setValue((int)(100.0f * m_currentScale));
    ui->spinHeight->setValue(m_initialScale.y * m_currentScale);
    ui->spinWidth->setValue(m_initialScale.x * m_currentScale);
    m_likeAMutex = false;
}

void CScaleWindow::SetEditingAsAbsolute(bool absolute)
{
    if(m_likeAMutex)
        return;
    m_likeAMutex = true;

    ui->spinPrecent->setEnabled(!absolute);
    ui->spinHeight->setEnabled(absolute);
    ui->spinWidth->setEnabled(absolute);
    ui->spinLength->setEnabled(absolute);
    m_likeAMutex = false;
}

void CScaleWindow::on_rbtnAbsolute_toggled(bool checked)
{
    ui->rbtnPercentage->setChecked(!checked);
    SetEditingAsAbsolute(checked);
}

void CScaleWindow::on_rbtnPercentage_toggled(bool checked)
{
    ui->rbtnAbsolute->setChecked(!checked);
    SetEditingAsAbsolute(!checked);
}
