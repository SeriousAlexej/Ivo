#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "settings/settings.h"
#include <cassert>

CSettingsWindow::CSettingsWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CSettingsWindow)
{
    ui->setupUi(this);
}

CSettingsWindow::~CSettingsWindow()
{
    delete ui;
}

void CSettingsWindow::LoadSettings()
{
    const CSettings& s = CSettings::GetInstance();
    const unsigned char renFlags = s.GetRenderFlags();
    ui->checkBoxEdges->setChecked(renFlags & CSettings::R_EDGES);
    ui->checkBoxFlaps->setChecked(renFlags & CSettings::R_FLAPS);
    ui->checkBoxTexture->setChecked(renFlags & CSettings::R_TEXTR);
    ui->checkBoxFolds->setChecked(renFlags & CSettings::R_FOLDS);

    ui->spinBoxW->setValue(s.GetPaperWidth());
    ui->spinBoxH->setValue(s.GetPaperHeight());

    ui->doubleSpinBoxMultiplier->setValue(s.GetResolutionScale());
    ui->horizontalSliderQuality->setValue(s.GetImageQuality());

    ui->doubleSpinBoxLineW->setValue(s.GetLineWidth());
    ui->spinBoxStippleLoop->setValue(s.GetStippleLoop());

    ui->spinBoxDetachAngle->setValue(s.GetDetachAngle());

    ui->spinBoxMaxFlatAngle->setValue(s.GetFoldMaxFlatAngle());

    switch(s.GetImageFormat())
    {
        case CSettings::IF_BMP :
        {
            ui->comboBoxFormat->setCurrentIndex(1);
            break;
        }
        case CSettings::IF_JPG :
        {
            ui->comboBoxFormat->setCurrentIndex(2);
            break;
        }
        case CSettings::IF_PNG :
        {
            ui->comboBoxFormat->setCurrentIndex(0);
            break;
        }
        default: assert(false);
    }
}

void CSettingsWindow::SaveSettings() const
{
    unsigned char renFlags = 0;
    unsigned stippleLoop = (unsigned)ui->spinBoxStippleLoop->value();
    float lineWidth = (float)ui->doubleSpinBoxLineW->value();
    unsigned papW = (unsigned)ui->spinBoxW->value();
    unsigned papH = (unsigned)ui->spinBoxH->value();
    float resScale = (float)ui->doubleSpinBoxMultiplier->value();
    unsigned char imgQuality = (unsigned char)ui->horizontalSliderQuality->value();
    unsigned char detachAngle = (unsigned char)ui->spinBoxDetachAngle->value();
    unsigned char maxFlatAngle = (unsigned char)ui->spinBoxMaxFlatAngle->value();
    CSettings::ImageFormat imgFormat = CSettings::IF_PNG;

    if(ui->checkBoxEdges->isChecked())
    {
        renFlags |= CSettings::R_EDGES;
    }
    if(ui->checkBoxFlaps->isChecked())
    {
        renFlags |= CSettings::R_FLAPS;
    }
    if(ui->checkBoxTexture->isChecked())
    {
        renFlags |= CSettings::R_TEXTR;
    }
    if(ui->checkBoxFolds->isChecked())
    {
        renFlags |= CSettings::R_FOLDS;
    }

    QString format = ui->comboBoxFormat->currentText();
    if(format == "BMP")
    {
        imgFormat = CSettings::IF_BMP;
    }
    else if(format == "JPG")
    {
        imgFormat = CSettings::IF_JPG;
    }
    else if(format == "PNG")
    {
        imgFormat = CSettings::IF_PNG;
    }
    else
    {
        assert(false);
    }

    CSettings& s = CSettings::GetInstance();
    s.SetRenderFlags(renFlags);
    s.SetPaperWidth(papW);
    s.SetPaperHeight(papH);
    s.SetResolutionScale(resScale);
    s.SetImageQuality(imgQuality);
    s.SetImageFormat(imgFormat);
    s.SetStippleLoop(stippleLoop);
    s.SetLineWidth(lineWidth);
    s.SetDetachAngle(detachAngle);
    s.SetFoldMaxFlatAngle(maxFlatAngle);
}

void CSettingsWindow::on_pushButtonOK_clicked()
{
    SaveSettings();
    close();
}

void CSettingsWindow::on_pushButtonCancel_clicked()
{
    close();
}

void CSettingsWindow::on_pushButtonOrientation_clicked()
{
    int w = ui->spinBoxW->value();
    int h = ui->spinBoxH->value();
    ui->spinBoxW->setValue(h);
    ui->spinBoxH->setValue(w);
}

void CSettingsWindow::on_pushButtonApplyPreset_clicked()
{
    QString preset = ui->comboBoxPresets->currentText();
    if(preset == "A1")
    {
        ui->spinBoxW->setValue(841);
        ui->spinBoxH->setValue(594);
    }
    else if(preset == "A2")
    {
        ui->spinBoxW->setValue(594);
        ui->spinBoxH->setValue(420);
    }
    else if(preset == "A3")
    {
        ui->spinBoxW->setValue(420);
        ui->spinBoxH->setValue(297);
    }
    else if(preset == "A4")
    {
        ui->spinBoxW->setValue(297);
        ui->spinBoxH->setValue(210);
    }
    else if(preset == "A5")
    {
        ui->spinBoxW->setValue(210);
        ui->spinBoxH->setValue(148);
    }
    else
    {
        assert(false);
    }
}

void CSettingsWindow::on_spinBoxW_valueChanged(int i)
{
    Q_UNUSED(i);
    UpdateResolution();
}

void CSettingsWindow::on_spinBoxH_valueChanged(int i)
{
    Q_UNUSED(i);
    UpdateResolution();
}

void CSettingsWindow::on_doubleSpinBoxMultiplier_valueChanged(double i)
{
    Q_UNUSED(i);
    UpdateResolution();
}

void CSettingsWindow::UpdateResolution()
{
    double mult = ui->doubleSpinBoxMultiplier->value();
    double w = mult * ui->spinBoxW->value();
    double h = mult * ui->spinBoxH->value();

    ui->lineW->setText(QString::number((int)w));
    ui->lineH->setText(QString::number((int)h));
}
