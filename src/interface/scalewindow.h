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
#ifndef SCALEWINDOW_H
#define SCALEWINDOW_H

#include <QDialog>
#include <glm/vec3.hpp>

namespace Ui {
class ScaleWindow;
}

class CScaleWindow : public QDialog
{
    Q_OBJECT

public:
    explicit CScaleWindow(float& outScale, const glm::vec3& initSca, QWidget* parent = nullptr);
    ~CScaleWindow();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_spinPrecent_valueChanged(int i);
    void on_rbtnAbsolute_toggled(bool checked);
    void on_rbtnPercentage_toggled(bool checked);
    void on_spinWidth_valueChanged(double i);
    void on_spinHeight_valueChanged(double i);
    void on_spinLength_valueChanged(double i);

private:
    void ResetUI();
    void SetEditingAsAbsolute(bool absolute);

    Ui::ScaleWindow* ui;
    glm::vec3        m_initialScale;
    float            m_currentScale;
    float&           m_outputScale;
};

#endif // SCALEWINDOW_H
