CONFIG += windows
CONFIG -= qt
CONFIG += c++11
CONFIG -= app_bundle

msvc {
    LIBS += advapi32.lib shell32.lib comdlg32.lib user32.lib gdi32.lib ws2_32.lib
}

DEFINES -= UNICODE

isEmpty(APP_NAME) {
    APP_NAME = RedPandaCPP
}

SOURCES += \
        main.cpp

RC_FILE += \
    redpanda-git-askpass_private.rc
    resource.rc

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

win32: {
    !isEmpty(PREFIX) {
        target.path = $${PREFIX}
    }
}


# Default rules for deployment.
qnx: target.path = $${PREFIX}/libexec/$${APP_NAME}
else: unix:!android: target.path = $${PREFIX}/libexec/$${APP_NAME}
!isEmpty(target.path): INSTALLS += target
