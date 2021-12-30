TEMPLATE = subdirs

SUBDIRS += \
    RedPandaIDE \
    consolepauser

APP_NAME = RedPandaIDE

linux: {
    resources.path = /opt/$${APP_NAME}
    resources.files += linux/* 
    resources.files += NEWS.md
    resources.files += LICENSE
    resources.files += README.md

    INSTALLS += resources
}
