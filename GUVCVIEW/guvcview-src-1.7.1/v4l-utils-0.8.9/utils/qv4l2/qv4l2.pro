######################################################################
# Automatically generated by qmake (1.07a) Sat Jun 17 12:35:16 2006
######################################################################

TEMPLATE = app
INCLUDEPATH += . ../libv4l2util ../../lib/include ../../include
CONFIG += debug

# Input
HEADERS += qv4l2.h general-tab.h v4l2-api.h capture-win.h vbi-tab.h raw2sliced.h
SOURCES += qv4l2.cpp general-tab.cpp ctrl-tab.cpp v4l2-api.cpp capture-win.cpp vbi-tab.cpp raw2sliced.cpp
LIBS += -L../../lib/libv4l2 -lv4l2 -L../../lib/libv4lconvert -lv4lconvert -lrt -L../libv4l2util -lv4l2util

RESOURCES += qv4l2.qrc