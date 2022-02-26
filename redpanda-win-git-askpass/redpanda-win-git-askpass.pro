TEMPLATE = app
CONFIG += windows
CONFIG -= app_bundle
CONFIG -= qt
DEFINES -= UNICODE

isEmpty(APP_NAME) {
    APP_NAME = RedPandaCPP
}

SOURCES += \
        main.c

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
