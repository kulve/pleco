CONFIG += qt debug warn_on
QT += network

INCLUDEPATH += ../common
LIBS += -L../common -lcommon

SOURCES += main.cpp

TARGET = controller
