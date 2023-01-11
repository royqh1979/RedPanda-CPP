TEMPLATE = lib
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += nokey
CONFIG += staticlib

win32: {
DEFINES += _WIN32_WINNT=0x0601
}

gcc {
    QMAKE_CXXFLAGS_RELEASE += -Werror=return-type
    QMAKE_CXXFLAGS_DEBUG += -Werror=return-type
}

msvc {
    DEFINES += NOMINMAX
}

SOURCES += qsynedit/codefolding.cpp \
    qsynedit/constants.cpp \
    qsynedit/document.cpp \
    qsynedit/keystrokes.cpp \
    qsynedit/miscprocs.cpp \
    qsynedit/exporter/exporter.cpp \
    qsynedit/exporter/htmlexporter.cpp \
    qsynedit/exporter/qtsupportedhtmlexporter.cpp \
    qsynedit/exporter/rtfexporter.cpp \
    qsynedit/gutter.cpp \
    qsynedit/painter.cpp \
    qsynedit/qsynedit.cpp \
    qsynedit/searcher/baseseacher.cpp \
    qsynedit/searcher/basicsearcher.cpp \
    qsynedit/searcher/regexsearcher.cpp \
    qsynedit/syntaxer/asm.cpp \
    qsynedit/syntaxer/cpp.cpp \
    qsynedit/syntaxer/customhighlighterv1.cpp \
    qsynedit/syntaxer/glsl.cpp \
    qsynedit/types.cpp \
    qsynedit/syntaxer/makefile.cpp \
    qsynedit/syntaxer/syntaxer.cpp

HEADERS += \
    qsynedit/codefolding.h \
    qsynedit/constants.h \
    qsynedit/document.h \
    qsynedit/keystrokes.h \
    qsynedit/miscprocs.h \
    qsynedit/types.h \
    qsynedit/exporter/exporter.h \
    qsynedit/exporter/htmlexporter.h \
    qsynedit/exporter/qtsupportedhtmlexporter.h \
    qsynedit/exporter/rtfexporter.h \
    qsynedit/gutter.h \
    qsynedit/painter.h \
    qsynedit/qsynedit.h \
    qsynedit/searcher/baseseacher.h \
    qsynedit/searcher/basicsearcher.h \
    qsynedit/searcher/regexsearcher.h \
    qsynedit/syntaxer/asm.h \
    qsynedit/syntaxer/cpp.h \
    qsynedit/syntaxer/customhighlighterv1.h \
    qsynedit/syntaxer/glsl.h \
    qsynedit/syntaxer/makefile.h \
    qsynedit/syntaxer/syntaxer.h

INCLUDEPATH += ../redpanda_qt_utils
