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
    qsynedit/highlighter/asm.cpp \
    qsynedit/highlighter/base.cpp \
    qsynedit/highlighter/composition.cpp \
    qsynedit/highlighter/cpp.cpp \
    qsynedit/highlighter/customhighlighterv1.cpp \
    qsynedit/highlighter/glsl.cpp \
    qsynedit/Search.cpp \
    qsynedit/SearchBase.cpp \
    qsynedit/SearchRegex.cpp \
    qsynedit/Types.cpp \
    qsynedit/highlighter/makefilehighlighter.cpp

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
    qsynedit/highlighter/asm.h \
    qsynedit/highlighter/base.h \
    qsynedit/highlighter/composition.h \
    qsynedit/highlighter/cpp.h \
    qsynedit/highlighter/customhighlighterv1.h \
    qsynedit/highlighter/glsl.h \
    qsynedit/highlighter/makefilehighlighter.h

INCLUDEPATH += ../redpanda_qt_utils
