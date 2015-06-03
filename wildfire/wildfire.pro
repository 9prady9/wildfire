#-------------------------------------------------
#
# Project created by QtCreator 2014-07-11T14:43:16
#
#-------------------------------------------------

QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = wildfire
#CONFIG += console
TEMPLATE = app

DEFINES += NOMINMAX
INCLUDEPATH += E:\CodeWorks\gitroot\arrayfire\build\package\include
LIBS += -LE:\CodeWorks\gitroot\arrayfire\build\package\lib -lafopencl


SOURCES += main.cpp\
        mainwindow.cpp \
    ImageCanvas.cpp \
    imageEdit.cpp

HEADERS  += mainwindow.h \
    ImageCanvas.h \
    imageEdit.hpp

FORMS    += mainwindow.ui
