TEMPLATE = lib
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += nokey

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

SOURCES += CodeFolding.cpp \
    Constants.cpp \
    KeyStrokes.cpp \
    MiscClasses.cpp \
    MiscProcs.cpp \
    SynEdit.cpp \
    TextBuffer.cpp \
    TextPainter.cpp \
    exporter/synexporter.cpp \
    exporter/synhtmlexporter.cpp \
    exporter/synrtfexporter.cpp \
    highlighter/asm.cpp \
    highlighter/base.cpp \
    highlighter/composition.cpp \
    highlighter/cpp.cpp \
    highlighter/glsl.cpp \
    Search.cpp \
    SearchBase.cpp \
    SearchRegex.cpp \
    Types.cpp

HEADERS += Search.h \
    SearchBase.h \
    SearchRegex.h \
    CodeFolding.h \
    Constants.h \
    KeyStrokes.h \
    MiscClasses.h \
    MiscProcs.h \
    SynEdit.h \
    TextBuffer.h \
    TextPainter.h \
    Types.h \
    exporter/synexporter.h \
    exporter/synhtmlexporter.h \
    exporter/synrtfexporter.h \
    highlighter/asm.h \
    highlighter/base.h \
    highlighter/composition.h \
    highlighter/cpp.h \
    highlighter/glsl.h \
