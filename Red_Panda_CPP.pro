TEMPLATE = subdirs

SUBDIRS += \
    RedPandaIDE \
    consolepauser \
    redpanda_qt_utils \
    qsynedit \
    lua

consolepauser.subdir = tools/consolepauser
redpanda_qt_utils.subdir = libs/redpanda_qt_utils
qsynedit.subdir = libs/qsynedit
lua.subdir = libs/lua

# Add the dependencies so that the RedPandaIDE project can add the depended programs
# into the main app bundle
RedPandaIDE.depends = consolepauser qsynedit lua
qsynedit.depends = redpanda_qt_utils

APP_NAME = RedPandaCPP
include(version.inc)

!isEmpty(APP_VERSION_SUFFIX): {
    APP_VERSION = "$${APP_VERSION}$${APP_VERSION_SUFFIX}"
}

# win32: {
# SUBDIRS += \
#     redpanda-win-git-askpass
# redpanda-win-git-askpass.subdir = tools/redpanda-win-git-askpass
# RedPandaIDE.depends += redpanda-win-git-askpass
# }

# unix: {
# SUBDIRS += \
#     redpanda-git-askpass
#     redpanda-git-askpass.subdir = tools/redpanda-git-askpass
#     RedPandaIDE.depends += redpanda-git-askpass
# }

unix:!macos: {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }
    isEmpty(LIBEXECDIR) {
        LIBEXECDIR = $${PREFIX}/libexec
    }

    QMAKE_SUBSTITUTES += platform/linux/RedPandaIDE.desktop.in

    resources.path = $${PREFIX}/share/$${APP_NAME}
    resources.files += platform/linux/templates
    INSTALLS += resources

    docs.path = $${PREFIX}/share/doc/$${APP_NAME}
    docs.files += README.md
    docs.files += NEWS.md
    docs.files += LICENSE
    INSTALLS += docs

    xdgicons.path = $${PREFIX}/share/icons/hicolor/scalable/apps/
    xdgicons.files += platform/linux/redpandaide.svg
    INSTALLS += xdgicons

    desktop.path = $${PREFIX}/share/applications
    desktop.files += platform/linux/RedPandaIDE.desktop
    INSTALLS += desktop

    mime.path = $${PREFIX}/share/mime/packages
    mime.files = platform/linux/redpandaide.xml
    INSTALLS += mime
}

win32: {
    !isEmpty(PREFIX) {
        target.path = $${PREFIX}

        resources.path = $${PREFIX}

        resources.files += platform/windows/templates
        resources.files += platform/windows/qt.conf
        resources.files += README.md
        resources.files += NEWS.md
        resources.files += LICENSE
        resources.files += RedPandaIDE/images/devcpp.ico

        INSTALLS += resources

        equals(X86_64, "ON") {
            extra_templates.path = $${PREFIX}/templates
            extra_templates.files += platform/windows/templates-win64/*
            INSTALLS += extra_templates
        }
    }
}
