QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    editor.cpp \
    editorlist.cpp \
    main.cpp \
    mainwindow.cpp \
    utils.cpp

HEADERS += \
    editor.h \
    editorlist.h \
    mainwindow.h \
    utils.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    RedPandaIDE_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -L$$PWD -lqscintilla2_qt5
