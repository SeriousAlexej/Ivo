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
#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "settings/settings.h"

CSettingsWindow::CSettingsWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CSettingsWindow)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::Tool);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    LoadSettings();
}

CSettingsWindow::~CSettingsWindow()
{
    delete ui;
}

void CSettingsWindow::LoadSettings()
{
    const CSettings& s = CSettings::GetInstance();

    ui->spinBoxW->setValue(s.GetPaperWidth());
    ui->spinBoxH->setValue(s.GetPaperHeight());
    ui->spinBoxHMargs->setValue(s.GetMarginsHorizontal());
    ui->spinBoxVMargs->setValue(s.GetMarginsVertical());
}

void CSettingsWindow::SaveSettings() const
{
    unsigned papW = (unsigned)ui->spinBoxW->value();
    unsigned papH = (unsigned)ui->spinBoxH->value();
    unsigned margH = (unsigned)ui->spinBoxHMargs->value();
    unsigned margV = (unsigned)ui->spinBoxVMargs->value();

    CSettings& s = CSettings::GetInstance();
    s.SetPaperWidth(papW);
    s.SetPaperHeight(papH);
    s.SetMarginsHorizontal(margH);
    s.SetMarginsVertical(margV);
}

void CSettingsWindow::on_buttonBox_accepted()
{
    SaveSettings();
    close();
}

void CSettingsWindow::on_buttonBox_rejected()
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
    UpdateMargins();
}

void CSettingsWindow::on_spinBoxH_valueChanged(int i)
{
    Q_UNUSED(i);
    UpdateMargins();
}

void CSettingsWindow::UpdateMargins()
{
    if(ui->spinBoxW->value() < ui->spinBoxHMargs->value()*2)
        ui->spinBoxHMargs->setValue(ui->spinBoxW->value()/2);

    if(ui->spinBoxH->value() < ui->spinBoxVMargs->value()*2)
        ui->spinBoxVMargs->setValue(ui->spinBoxH->value()/2);
}

void CSettingsWindow::on_spinBoxHMargs_valueChanged(int i)
{
    Q_UNUSED(i);
    UpdateMargins();
}

void CSettingsWindow::on_spinBoxVMargs_valueChanged(int i)
{
    Q_UNUSED(i);
    UpdateMargins();
}
