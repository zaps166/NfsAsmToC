#-------------------------------------------------
#
# Project created by QtCreator 2015-02-08T19:30:50
#
#-------------------------------------------------

QT += core
QT -= gui

TARGET  = NfsAsmToC
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

RESOURCES += NfsAsmToC.qrc

QMAKE_CXXFLAGS += -std=c++14

SOURCES += Instruction.cpp Utils.cpp Consts.cpp NfsAsmToC.cpp
HEADERS += Instruction.hpp Utils.hpp Consts.hpp
