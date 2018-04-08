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
#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDialog>

namespace Ui {
class CSettingsWindow;
}

class CSettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit CSettingsWindow(QWidget* parent = nullptr);
    ~CSettingsWindow();

    void LoadSettings();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_pushButtonOrientation_clicked();
    void on_pushButtonApplyPreset_clicked();
    void on_spinBoxW_valueChanged(int i);
    void on_spinBoxH_valueChanged(int i);
    void on_spinBoxHMargs_valueChanged(int i);
    void on_spinBoxVMargs_valueChanged(int i);

private:
    void SaveSettings() const;
    void UpdateMargins();

    Ui::CSettingsWindow* ui;
};

#endif // SETTINGSWINDOW_H
