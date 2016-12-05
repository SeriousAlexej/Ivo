#-------------------------------------------------
#
# Project created by QtCreator 2015-12-29T20:50:13
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

win32 {
LIBS += -lOpengl32
}
LIBS += -L$$PWD/../assimp-3.3.1/build/code/ -lassimp

INCLUDEPATH += $$PWD/../glm
INCLUDEPATH += $$PWD/../assimp-3.3.1/include
DEPENDPATH += $$PWD/../assimp-3.3.1/include

TARGET = Ivo
TEMPLATE = app


SOURCES += main.cpp\
    mesh/trianglegroup.cpp \
    mesh/triangle2d.cpp \
    mesh/mesh.cpp \
    interface/mainwindow.cpp \
    renderers/renwin2d.cpp \
    renderers/renwin3d.cpp \
    interface/settingswindow.cpp \
    settings/settings.cpp \
    mesh/command.cpp \
    interface/scalewindow.cpp \
    interface/materialmanager.cpp \
    formats3d.cpp \
    renderers/renwin.cpp

HEADERS  += \
    mesh/mesh.h \
    interface/mainwindow.h \
    renderers/renwin2d.h \
    renderers/renwin3d.h \
    interface/settingswindow.h \
    settings/settings.h \
    mesh/command.h \
    interface/scalewindow.h \
    interface/materialmanager.h \
    renderers/renwin.h

FORMS    += interface/mainwindow.ui \
    interface/settingswindow.ui \
    interface/scalewindow.ui \
    interface/materialmanager.ui

RESOURCES += \
    res.qrc
