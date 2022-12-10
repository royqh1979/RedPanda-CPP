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

SOURCES += qsynedit/CodeFolding.cpp \
    qsynedit/Constants.cpp \
    qsynedit/KeyStrokes.cpp \
    qsynedit/MiscClasses.cpp \
    qsynedit/MiscProcs.cpp \
    qsynedit/SynEdit.cpp \
    qsynedit/TextBuffer.cpp \
    qsynedit/TextPainter.cpp \
    qsynedit/exporter/synexporter.cpp \
    qsynedit/exporter/synhtmlexporter.cpp \
    qsynedit/exporter/synrtfexporter.cpp \
    qsynedit/syntaxer/asm.cpp \
    qsynedit/syntaxer/cpp.cpp \
    qsynedit/syntaxer/customhighlighterv1.cpp \
    qsynedit/syntaxer/glsl.cpp \
    qsynedit/Search.cpp \
    qsynedit/SearchBase.cpp \
    qsynedit/SearchRegex.cpp \
    qsynedit/Types.cpp \
    qsynedit/syntaxer/makefile.cpp \
    qsynedit/syntaxer/syntaxer.cpp

HEADERS += qsynedit/Search.h \
    qsynedit/SearchBase.h \
    qsynedit/SearchRegex.h \
    qsynedit/CodeFolding.h \
    qsynedit/Constants.h \
    qsynedit/KeyStrokes.h \
    qsynedit/MiscClasses.h \
    qsynedit/MiscProcs.h \
    qsynedit/SynEdit.h \
    qsynedit/TextBuffer.h \
    qsynedit/TextPainter.h \
    qsynedit/Types.h \
    qsynedit/exporter/synexporter.h \
    qsynedit/exporter/synhtmlexporter.h \
    qsynedit/exporter/synrtfexporter.h \
    qsynedit/syntaxer/asm.h \
    qsynedit/syntaxer/cpp.h \
    qsynedit/syntaxer/customhighlighterv1.h \
    qsynedit/syntaxer/glsl.h \
    qsynedit/syntaxer/makefile.h \
    qsynedit/syntaxer/syntaxer.h

INCLUDEPATH += ../redpanda_qt_utils
