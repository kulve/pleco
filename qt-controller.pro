TEMPLATE = subdirs
SUBDIRS = common controller slave 
controller.depends = common
slave.depends = common
