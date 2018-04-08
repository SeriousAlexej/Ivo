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
#include <cassert>
#include "exportwindow.h"
#include "ui_exportwindow.h"
#include "settings/settings.h"

CExportWindow::CExportWindow(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::CExportWindow)
{
    setWindowFlags(windowFlags() | Qt::Tool);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);

    const CSettings& s = CSettings::GetInstance();

    ui->doubleSpinBoxMultiplier->setValue(s.GetResolutionScale());
    ui->horizontalSliderQuality->setValue(s.GetImageQuality());
    on_doubleSpinBoxMultiplier_valueChanged(ui->doubleSpinBoxMultiplier->value());

    switch(s.GetImageFormat())
    {
        case CSettings::IF_BMP :
            ui->comboBoxFormat->setCurrentIndex(1);
            break;

        case CSettings::IF_JPG :
            ui->comboBoxFormat->setCurrentIndex(2);
            break;

        case CSettings::IF_PNG :
            ui->comboBoxFormat->setCurrentIndex(0);
            break;

        default: assert(false);
    }
}

CExportWindow::~CExportWindow()
{
    delete ui;
}

void CExportWindow::on_buttonBox_accepted()
{
    float resScale = (float)ui->doubleSpinBoxMultiplier->value();
    unsigned char imgQuality = (unsigned char)ui->horizontalSliderQuality->value();

    CSettings::ImageFormat imgFormat = CSettings::IF_PNG;

    QString format = ui->comboBoxFormat->currentText();
    if(format == "BMP")
        imgFormat = CSettings::IF_BMP;
    else if(format == "JPG")
        imgFormat = CSettings::IF_JPG;
    else if(format == "PNG")
        imgFormat = CSettings::IF_PNG;
    else
        assert(false);

    CSettings& s = CSettings::GetInstance();
    s.SetResolutionScale(resScale);
    s.SetImageQuality(imgQuality);
    s.SetImageFormat(imgFormat);

    accept();
}

void CExportWindow::on_buttonBox_rejected()
{
    reject();
}

void CExportWindow::on_doubleSpinBoxMultiplier_valueChanged(double mult)
{
    const CSettings& s = CSettings::GetInstance();
    int w = mult * s.GetPaperWidth();
    int h = mult * s.GetPaperHeight();

    ui->labelResolution->setText(QString("%1x%2").arg(w).arg(h));
}
