TEMPLATE = app
TARGET = benchqml
QT += qml quick gui
SOURCES += main.cpp
HEADERS += benchharness.h
CONFIG += qml_debug
RESOURCES = resources.qrc
OTHER_FILES += shim.qml

# other qml files which we test with
OTHER_FILES += data/* data/qtquickcontrols-gallery/* data/qtquickcontrols-gallery/qml/* data/qtquickcontrols-gallery51/* data/qtquickcontrols-gallery51/qml/*
