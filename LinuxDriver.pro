#-------------------------------------------------
#
# Project created by QtCreator 2023-06-12T22:50:22
#
#-------------------------------------------------
QT       += core gui network concurrent dbus websockets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += C++11


TARGET = NewlineLinuxDriver
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    websocketserver.cpp \
    tcpserver.cpp \
    multifingergesture.cpp \
    qtsingleapplication.cpp \
    qtlocalpeer.cpp \
    qtlockedfile.cpp \
    qtlockedfile_win.cpp \
    qtlockedfile_unix.cpp

HEADERS  += mainwindow.h \
    websocketserver.h \
    tcpserver.h \
    multifingergesture.h \
    qtsingleapplication.h \
    qtlocalpeer.h \
    qtlockedfile.h

FORMS    += mainwindow.ui

unix:!macx {
        LIBS += -lXtst
        LIBS += -lX11
        LIBS += -ludev
        LIBS += -lxcb
}

RESOURCES += \
    resource.qrc
