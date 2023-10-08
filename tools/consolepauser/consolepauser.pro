QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

isEmpty(APP_NAME) {
    APP_NAME = RedPandaCPP
}

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
}

# macOS does not provide rt library, only use it for Linux
linux: {
LIBS+= \
    -lrt
}

win32: {
    DEFINES += _WIN32_WINNT=0x0501
    LIBS += -lpsapi  # GetProcessMemoryInfo
}

CONFIG += lrelease
CONFIG += embed_translations

isEmpty(PREFIX) {
    PREFIX = /usr/local
}
isEmpty(LIBEXECDIR) {
    LIBEXECDIR = $${PREFIX}/libexec
}

win32: {
    !isEmpty(PREFIX) {
        target.path = $${PREFIX}
    }
}

# Default rules for deployment.
qnx: target.path = $${LIBEXECDIR}/$${APP_NAME}
else: unix:!android: target.path = $${LIBEXECDIR}/$${APP_NAME}
!isEmpty(target.path): INSTALLS += target
