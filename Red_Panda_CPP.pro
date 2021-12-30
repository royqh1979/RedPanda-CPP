TEMPLATE = subdirs

SUBDIRS += \
    RedPandaIDE \
    consolepauser

APP_NAME = RedPandaIDE

linux: {
    resources.path = /opt/$${APP_NAME}
    resources.files += linux/*

    INSTALLS += resources
}
