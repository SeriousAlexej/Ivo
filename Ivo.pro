#-------------------------------------------------
#
# Project created by QtCreator 2015-12-29T20:50:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

INCLUDEPATH += $$PWD/../glm

TARGET = Ivo
TEMPLATE = app


SOURCES += main.cpp\
    modelLoader/objParser.cpp \
    modelLoader/objScanner.cpp \
    mesh/trianglegroup.cpp \
    mesh/triangle2d.cpp \
    mesh/mesh.cpp \
    interface/mainwindow.cpp \
    renderers/renwin2d.cpp \
    renderers/renwin3d.cpp \
    interface/settingswindow.cpp \
    settings/settings.cpp \
    mesh/command.cpp \
    interface/scalewindow.cpp

HEADERS  += \
    modelLoader/objParser.base.h \
    modelLoader/objParser.h \
    modelLoader/objParser.impl.h \
    modelLoader/objScanner.base.h \
    modelLoader/objScanner.h \
    modelLoader/objScanner.impl.h \
    modelLoader/structure.h \
    mesh/mesh.h \
    interface/mainwindow.h \
    renderers/renwin2d.h \
    renderers/renwin3d.h \
    interface/settingswindow.h \
    settings/settings.h \
    mesh/command.h \
    interface/scalewindow.h

FORMS    += interface/mainwindow.ui \
    interface/settingswindow.ui \
    interface/scalewindow.ui

RESOURCES += \
    res.qrc
