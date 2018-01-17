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
