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

APP_NAME = RedPandaCPP

APP_VERSION = 2.6

linux: {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
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

    pixmaps.path = $${PREFIX}/share/pixmaps
    pixmaps.files += platform/linux/redpandaide.png
    INSTALLS += pixmaps

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
    }
}
