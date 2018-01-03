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
    "geometric/aabbox.cpp"
    "geometric/binPacking.cpp"
    "geometric/minOBBox.cpp"
    "geometric/obbox.cpp"
    "interface/modes2D/flaps.cpp"
    "interface/modes2D/move.cpp"
    "interface/modes2D/rotate.cpp"
    "interface/modes2D/snap.cpp"
    "interface/mainwindow.cpp"
    "interface/materialmanager.cpp"
    "interface/renwin.cpp"
    "interface/renwin2d.cpp"
    "interface/renwin3d.cpp"
    "interface/scalewindow.cpp"
    "interface/settingswindow.cpp"
    "io/saferead.cpp"
    "ivo/ivoloader.cpp"
    "mesh/command.cpp"
    "mesh/mesh.cpp"
    "mesh/meshPacking.cpp"
    "mesh/triangle2d.cpp"
    "mesh/trianglegroup.cpp"
    "pdo/pdoloader.cpp"
    "pdo/pdotools.cpp"
    "renderers/abstractrenderer.cpp"
    "renderers/renderbase2d.cpp"
    "renderers/renderbase3d.cpp"
    "renderers/renderlegacy2d.cpp"
    "renderers/renderlegacy3d.cpp"
    "settings/settings.cpp"
    "formats3d.cpp"
    "main.cpp"
)

set(SRC_LIST_H
    "geometric/aabbox.h"
    "geometric/binPacking.h"
    "geometric/obbox.h"
    "interface/modes2D/renwin2DModes.h"
    "interface/mainwindow.h"
    "interface/materialmanager.h"
    "interface/renwin.h"
    "interface/renwin2d.h"
    "interface/renwin2dEditInfo.h"
    "interface/renwin3d.h"
    "interface/scalewindow.h"
    "interface/selectioninfo.h"
    "interface/settingswindow.h"
    "io/saferead.h"
    "mesh/command.h"
    "mesh/mesh.h"
    "pdo/pdotools.h"
    "renderers/abstractrenderer.h"
    "renderers/renderbase2d.h"
    "renderers/renderbase3d.h"
    "renderers/renderlegacy2d.h"
    "renderers/renderlegacy3d.h"
    "settings/settings.h"
)

set(UIS
    "interface/mainwindow.ui"
    "interface/materialmanager.ui"
    "interface/scalewindow.ui"
    "interface/settingswindow.ui"
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
