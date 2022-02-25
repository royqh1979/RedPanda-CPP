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
