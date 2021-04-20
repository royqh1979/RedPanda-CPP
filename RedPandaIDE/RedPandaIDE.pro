QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    compiler/compiler.cpp \
    compiler/compilermanager.cpp \
    compiler/filecompiler.cpp \
    editor.cpp \
    editorlist.cpp \
    main.cpp \
    mainwindow.cpp \
    settingsdialog/compilersetdirectorieswidget.cpp \
    settingsdialog/compilersetoptionwidget.cpp \
    settings.cpp \
    settingsdialog/settingsdialog.cpp \
    settingsdialog/settingswidget.cpp \
    systemconsts.cpp \
    utils.cpp

HEADERS += \
    compiler/compiler.h \
    compiler/compilermanager.h \
    compiler/filecompiler.h \
    editor.h \
    editorlist.h \
    mainwindow.h \
    settingsdialog/compilersetdirectorieswidget.h \
    settingsdialog/compilersetoptionwidget.h \
    settings.h \
    settingsdialog/settingsdialog.h \
    settingsdialog/settingswidget.h \
    systemconsts.h \
    utils.h

FORMS += \
    mainwindow.ui \
    settingsdialog/compilersetdirectorieswidget.ui \
    settingsdialog/compilersetoptionwidget.ui \
    settingsdialog/settingsdialog.ui

TRANSLATIONS += \
    RedPandaIDE_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -L$$PWD -lqscintilla2_qt5

RESOURCES += \
    icons.qrc
