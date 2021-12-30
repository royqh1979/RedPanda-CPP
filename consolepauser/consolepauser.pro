QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

APP_DIR = RedPandaIDE

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32: {
SOURCES += \
    main.windows.cpp
}

unix: {
SOURCES += \
    main.unix.cpp
    LIBS+= \
        -lrt
}

CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${APP_DIR}/bin
else: unix:!android: target.path = /opt/$${APP_DIR}/bin
!isEmpty(target.path): INSTALLS += target
