TEMPLATE = app
CONFIG += windows
CONFIG -= app_bundle
CONFIG -= qt
DEFINES -= UNICODE

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
