TEMPLATE = subdirs

SUBDIRS += \
    redpanda_qt_utils \
    testqsynedit

redpanda_qt_utils.subdir = libs/redpanda_qt_utils
testqsynedit.file = libs/qsynedit/testqsynedit.pro

# Add the dependencies so that the RedPandaIDE project can add the depended programs
# into the main app bundle
testqsynedit.depends = redpanda_qt_utils
