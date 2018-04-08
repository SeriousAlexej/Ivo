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
#ifndef EXPORTWINDOW_H
#define EXPORTWINDOW_H

#include <QDialog>

namespace Ui {
class CExportWindow;
}

class CExportWindow : public QDialog
{
    Q_OBJECT

public:
    explicit CExportWindow(QWidget* parent = nullptr);
    ~CExportWindow();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_doubleSpinBoxMultiplier_valueChanged(double mult);

private:
    Ui::CExportWindow *ui;
};

#endif // EXPORTWINDOW_H
