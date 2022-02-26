QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

isEmpty(APP_NAME) {
    APP_NAME = RedPandaCPP
}
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    dialog.cpp

HEADERS += \
    dialog.h

FORMS += \
    dialog.ui

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

# Default rules for deployment.
qnx: target.path = $${PREFIX}/libexec/$${APP_NAME}
else: unix:!android: target.path = $${PREFIX}/libexec/$${APP_NAME}
!isEmpty(target.path): INSTALLS += target
