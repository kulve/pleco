QMAKE_CXXFLAGS += -funsigned-char -fexceptions -fstack-protector -fno-omit-frame-pointer --param=ssp-buffer-size=4 -fmessage-length=0

CONFIG += qt debug warn_on link_prl
QT += network

INCLUDEPATH += ../common
LIBS += -L../common -lcommon

SOURCES += main.cpp
SOURCES += Controller.cpp

HEADERS += Controller.h

TARGET = controller
