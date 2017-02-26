TARGET = shift-ims
TEMPLATE = app
DESTDIR = $$PWD/../../dist
QT = core gui widgets sql printsupport
RC_FILE = app.rc

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    global.h \
    mainwindow.h

FORMS += \
    mainwindow.ui
