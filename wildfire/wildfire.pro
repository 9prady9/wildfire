#-------------------------------------------------
#
# Project created by QtCreator 2014-07-11T14:43:16
#
#-------------------------------------------------

QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = wildfire
TEMPLATE = app

DEFINES += NOMINMAX

# set this path to compile the code.
#AF_PATH = ~/arrayfire

INCLUDEPATH += $${AF_PATH}/include
LIBS += -L$${AF_PATH}/lib64 -lafopencl -lforge

SOURCES += main.cpp\
        mainwindow.cpp \
    ImageCanvas.cpp \
    imageEdit.cpp

HEADERS  += mainwindow.h \
    ImageCanvas.h \
    imageEdit.hpp

FORMS    += mainwindow.ui
