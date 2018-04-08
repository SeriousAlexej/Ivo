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
#include "scalewindow.h"
#include "ui_scalewindow.h"
#include <QSignalBlocker>
#include <vector>

namespace
{

class ScaleWindowSignalBlocker
{
public:
    ScaleWindowSignalBlocker(Ui::ScaleWindow* ui)
    {
        m_blockers.emplace_back(ui->spinWidth);
        m_blockers.emplace_back(ui->spinHeight);
        m_blockers.emplace_back(ui->spinLength);
        m_blockers.emplace_back(ui->spinPrecent);
    }

private:
    std::vector<QSignalBlocker> m_blockers;
};

} //namespace

CScaleWindow::CScaleWindow(float& outScale, const glm::vec3& initSca, QWidget* parent) :
    QDialog(parent),
    ui(new Ui::ScaleWindow),
    m_initialScale(initSca),
    m_currentScale(1.0f),
    m_outputScale(outScale)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ResetUI();
}

CScaleWindow::~CScaleWindow()
{
    delete ui;
}

void CScaleWindow::ResetUI()
{
    const ScaleWindowSignalBlocker blocker(ui);

    m_currentScale = 1.0f;
    ui->spinPrecent->setValue(100);
    ui->spinWidth->setValue(m_initialScale.x);
    ui->spinHeight->setValue(m_initialScale.y);
    ui->spinLength->setValue(m_initialScale.z);
}

void CScaleWindow::on_buttonBox_accepted()
{
    m_outputScale = m_currentScale;
    close();
}

void CScaleWindow::on_buttonBox_rejected()
{
    close();
}

void CScaleWindow::on_spinPrecent_valueChanged(int i)
{
    Q_UNUSED(i);
    const ScaleWindowSignalBlocker blocker(ui);

    m_currentScale = 0.01f * ui->spinPrecent->value();
    ui->spinWidth->setValue(m_initialScale.x * m_currentScale);
    ui->spinHeight->setValue(m_initialScale.y * m_currentScale);
    ui->spinLength->setValue(m_initialScale.z * m_currentScale);
}

void CScaleWindow::on_spinWidth_valueChanged(double i)
{
    Q_UNUSED(i);
    const ScaleWindowSignalBlocker blocker(ui);

    m_currentScale = ui->spinWidth->value() / m_initialScale.x;
    ui->spinPrecent->setValue((int)(100.0f * m_currentScale));
    ui->spinHeight->setValue(m_initialScale.y * m_currentScale);
    ui->spinLength->setValue(m_initialScale.z * m_currentScale);
}

void CScaleWindow::on_spinHeight_valueChanged(double i)
{
    Q_UNUSED(i);
    const ScaleWindowSignalBlocker blocker(ui);

    m_currentScale = ui->spinHeight->value() / m_initialScale.y;
    ui->spinPrecent->setValue((int)(100.0f * m_currentScale));
    ui->spinWidth->setValue(m_initialScale.x * m_currentScale);
    ui->spinLength->setValue(m_initialScale.z * m_currentScale);
}

void CScaleWindow::on_spinLength_valueChanged(double i)
{
    Q_UNUSED(i);
    const ScaleWindowSignalBlocker blocker(ui);

    m_currentScale = ui->spinLength->value() / m_initialScale.z;
    ui->spinPrecent->setValue((int)(100.0f * m_currentScale));
    ui->spinHeight->setValue(m_initialScale.y * m_currentScale);
    ui->spinWidth->setValue(m_initialScale.x * m_currentScale);
}

void CScaleWindow::SetEditingAsAbsolute(bool absolute)
{
    const ScaleWindowSignalBlocker blocker(ui);

    ui->spinPrecent->setEnabled(!absolute);
    ui->spinHeight->setEnabled(absolute);
    ui->spinWidth->setEnabled(absolute);
    ui->spinLength->setEnabled(absolute);
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
