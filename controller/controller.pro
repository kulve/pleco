QMAKE_CXXFLAGS += -funsigned-char
QMAKE_CXXFLAGS += -Werror

CONFIG += qt debug warn_on link_prl link_pkgconfig
QT += network widgets
PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 libdrm
for(PKG, $$list($$unique(PKGCONFIG))) {
     !system(pkg-config --exists $$PKG):error($$PKG development files are missing)
}

packagesExist(openhmd) {
     QT += opengl openglwidgets filesystemwatcher
     DEFINES += ENABLE_OPENHMD
     PKGCONFIG += openhmd
     SOURCES += HMD.cpp
     SOURCES += Controller-vr.cpp
     SOURCES += VRWindow.cpp
     SOURCES += VRRenderer.cpp
     HEADERS += HMD.h
     HEADERS += Controller-vr.h
     HEADERS += VRWindow.h
     HEADERS += VRRenderer.h
     RESOURCES += shaders.qrc
} else {
     SOURCES += Controller-qt.cpp
     HEADERS += Controller-qt.h
}

INCLUDEPATH += ../common
LIBS += -L../common -lcommon

SOURCES += main.cpp
SOURCES += Controller.cpp
SOURCES += VideoReceiver.cpp
SOURCES += AudioReceiver.cpp
SOURCES += Joystick.cpp

HEADERS += Controller.h
HEADERS += VideoReceiver.h
HEADERS += AudioReceiver.h
HEADERS += Joystick.h

TARGET = controller

INSTALLS += target
target.path = $$PREFIX/bin
