TARGET = shift-ims
TEMPLATE = app
DESTDIR = $$PWD/../../dist
QT = core gui widgets sql printsupport
RC_FILE = app.rc

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    productmanagerwidget.cpp \
    producteditor.cpp \
    product.cpp \
    productlistwidget.cpp

HEADERS += \
    global.h \
    mainwindow.h \
    productmanagerwidget.h \
    producteditor.h \
    product.h \
    productlistwidget.h

FORMS += \
    mainwindow.ui \
    producteditor.ui
