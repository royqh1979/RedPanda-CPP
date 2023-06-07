TEMPLATE = subdirs

SUBDIRS += \
    RedPandaIDE \
    astyle \
    consolepauser \
    redpanda_qt_utils \
    qsynedit
    
astyle.subdir = tools/astyle
consolepauser.subdir = tools/consolepauser
redpanda_qt_utils.subdir = libs/redpanda_qt_utils
qsynedit.subdir = libs/qsynedit

APP_NAME = RedPandaCPP

APP_VERSION = 2.23

# Add the dependencies so that the RedPandaIDE project can add the depended programs
# into the main app bundle
RedPandaIDE.depends = astyle consolepauser qsynedit
qsynedit.depends = redpanda_qt_utils

win32: {
SUBDIRS += \
	redpanda-win-git-askpass
redpanda-win-git-askpass.subdir = tools/redpanda-win-git-askpass
RedPandaIDE.depends += redpanda-win-git-askpass
}

unix: {
SUBDIRS += \
    redpanda-git-askpass
redpanda-git-askpass.subdir = tools/redpanda-git-askpass
RedPandaIDE.depends += redpanda-git-askpass
}

linux: {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }
    isEmpty(LIBEXECDIR) {
        LIBEXECDIR = $${PREFIX}/libexec
    }

    QMAKE_SUBSTITUTES += platform/linux/redpandaide.desktop.in

    resources.path = $${PREFIX}/share/$${APP_NAME}
    resources.files += platform/linux/templates
    INSTALLS += resources

    docs.path = $${PREFIX}/share/doc/$${APP_NAME}
    docs.files += README.md
    docs.files += NEWS.md
    docs.files += LICENSE
    INSTALLS += docs

    equals(XDG_ADAPTIVE_ICON, "ON") {
        xdgicons.path = $${PREFIX}/share/icons/hicolor/scalable/apps/
        xdgicons.files += platform/linux/redpandaide.svg
        REDPANDA_ICON_PATH = redpandaide
        INSTALLS += xdgicons
    } else {
        pixmaps.path = $${PREFIX}/share/pixmaps
        pixmaps.files += platform/linux/redpandaide.png
        REDPANDA_ICON_PATH = $${PREFIX}/share/pixmaps/redpandaide.png
        INSTALLS += pixmaps
    }

    desktop.path = $${PREFIX}/share/applications
    desktop.files += platform/linux/redpandaide.desktop
    INSTALLS += desktop
}

win32: {
    !isEmpty(PREFIX) {
        target.path = $${PREFIX}
        QMAKE_SUBSTITUTES += platform/windows/installer-scripts/config.nsh.in
        QMAKE_SUBSTITUTES += platform/windows/installer-scripts/config32.nsh.in
        QMAKE_SUBSTITUTES += platform/windows/installer-scripts/config-clang.nsh.in

        resources.path = $${PREFIX}

        resources.files += platform/windows/templates
        resources.files += platform/windows/installer-scripts/config.nsh
        resources.files += platform/windows/installer-scripts/config32.nsh
        resources.files += platform/windows/installer-scripts/config-clang.nsh
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
