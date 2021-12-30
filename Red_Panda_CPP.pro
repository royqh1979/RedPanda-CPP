TEMPLATE = subdirs

SUBDIRS += \
    RedPandaIDE \
    consolepauser

APP_NAME = RedPandaIDE

resources.path = /opt/$${APP_NAME}
resources.files += linux/*

INSTALLS += resources
