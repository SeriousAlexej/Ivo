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
#include "importwindow.h"
#include "ui_importwindow.h"
#include "settings/settings.h"

CImportWindow::CImportWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CImportWindow)
{
    setWindowFlags(windowFlags() | Qt::Tool);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);
    ui->spinBoxDetachAngle->setValue(CSettings::GetInstance().GetDetachAngle());
}

CImportWindow::~CImportWindow()
{
    delete ui;
}

void CImportWindow::on_buttonBox_accepted()
{
    CSettings::GetInstance().SetDetachAngle(ui->spinBoxDetachAngle->value());
    accept();
}

void CImportWindow::on_buttonBox_rejected()
{
    reject();
}
