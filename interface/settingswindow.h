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
    explicit CSettingsWindow(QWidget *parent = 0);
    ~CSettingsWindow();

    void LoadSettings();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_pushButtonOrientation_clicked();
    void on_pushButtonApplyPreset_clicked();
    void on_spinBoxW_valueChanged(int i);
    void on_spinBoxH_valueChanged(int i);
    void on_doubleSpinBoxMultiplier_valueChanged(double i);
    void on_spinBoxHMargs_valueChanged(int i);
    void on_spinBoxVMargs_valueChanged(int i);

private:
    void SaveSettings() const;
    void UpdateResolution();
    void UpdateMargins();

    Ui::CSettingsWindow *ui;
};

#endif // SETTINGSWINDOW_H
