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
    void on_pushButtonOK_clicked();
    void on_pushButtonCancel_clicked();
    void on_pushButtonOrientation_clicked();
    void on_pushButtonApplyPreset_clicked();
    void on_spinBoxW_valueChanged(double i);
    void on_spinBoxH_valueChanged(double i);
    void on_doubleSpinBoxMultiplier_valueChanged(double i);

private:
    void SaveSettings() const;
    void UpdateResolution();

    Ui::CSettingsWindow *ui;
};

#endif // SETTINGSWINDOW_H
