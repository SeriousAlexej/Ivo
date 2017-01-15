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
    explicit CScaleWindow(QWidget *parent = 0);
    ~CScaleWindow();

    void SetOutputScalePtr(float* scaPtr);
    void SetInitialScale(glm::vec3 initSca);

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
    Ui::ScaleWindow *ui;
    void ResetUI();
    void SetEditingAsAbsolute(bool absolute);

    glm::vec3                  m_initialScale;
    float                      m_currentScale;
    bool                       m_likeAMutex;
    float*                     m_outputScale;
};

#endif // SCALEWINDOW_H
