#include <QApplication>
#include "interface/mainwindow.h"
#include "settings/settings.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CSettings& s = CSettings::GetInstance();
    QObject::connect(&s, &CSettings::SetAppStyle,
                     &a, &QApplication::setStyleSheet);
    CMainWindow w;
    w.show();

    return a.exec();
}
