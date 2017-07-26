cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)

project(Ivo)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules")

find_package(Qt5Core       5.5 REQUIRED)
find_package(Qt5Gui        5.5 REQUIRED)
find_package(Qt5Widgets    5.5 REQUIRED)
find_package(Qt5OpenGL     5.5 REQUIRED)
find_package(assimp            REQUIRED)
find_package(glm               REQUIRED)

include_directories(
    ${assimp_INCLUDE_DIRS}
    ${glm_INCLUDE_DIRS}
)

if(WIN32)
    list(APPEND ADDITIONAL_LIBRARIES opengl32)
elseif(UNIX)
    list(APPEND ADDITIONAL_LIBRARIES GL pthread)
endif()

set(SRC_LIST_C
    "main.cpp"
    "mesh/trianglegroup.cpp"
    "mesh/triangle2d.cpp"
    "mesh/mesh.cpp"
    "interface/mainwindow.cpp"
    "interface/settingswindow.cpp"
    "settings/settings.cpp"
    "mesh/command.cpp"
    "interface/scalewindow.cpp"
    "interface/materialmanager.cpp"
    "formats3d.cpp"
    "pdo/pdoloader.cpp"
    "pdo/pdotools.cpp"
    "ivo/ivoloader.cpp"
    "io/saferead.cpp"
    "renderers/renderlegacy3d.cpp"
    "renderers/renderlegacy2d.cpp"
    "renderers/renderbase3d.cpp"
    "renderers/renderbase2d.cpp"
    "interface/renwin.cpp"
    "interface/renwin2d.cpp"
    "interface/renwin3d.cpp"
    "renderers/abstractrenderer.cpp"
    "geometric/aabbox.cpp"
)

set(SRC_LIST_H
    "mesh/mesh.h"
    "interface/mainwindow.h"
    "interface/settingswindow.h"
    "settings/settings.h"
    "mesh/command.h"
    "interface/scalewindow.h"
    "interface/materialmanager.h"
    "pdo/pdotools.h"
    "io/saferead.h"
    "renderers/renderlegacy3d.h"
    "renderers/renderbase3d.h"
    "renderers/renderbase2d.h"
    "renderers/renderlegacy2d.h"
    "interface/renwin.h"
    "interface/renwin2d.h"
    "interface/renwin3d.h"
    "interface/selectioninfo.h"
    "renderers/abstractrenderer.h"
    "geometric/aabbox.h"
)

set(UIS
    "interface/mainwindow.ui"
    "interface/settingswindow.ui"
    "interface/scalewindow.ui"
    "interface/materialmanager.ui"
)

set(RESOURCES
    "res.qrc"
)

add_executable(Ivo
    ${SRC_LIST_C}
    ${SRC_LIST_H}
    ${UIS}
    ${RESOURCES}
)

target_link_libraries(Ivo
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::OpenGL
    ${assimp_LIBRARIES}
    ${ADDITIONAL_LIBRARIES}
)

if(MSVC)
    target_compile_definitions(Ivo PUBLIC "-D_CRT_SECURE_NO_WARNINGS")
endif()
