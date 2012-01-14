# This project assumes that char is quint8
QMAKE_CXXFLAGS += -funsigned-char

CONFIG += qt debug warn_on ordered
TEMPLATE = subdirs
#SUBDIRS = common controller slave plugins
SUBDIRS = common slave plugins
controller.depends = common
slave.depends = common
plugins.depends = common
