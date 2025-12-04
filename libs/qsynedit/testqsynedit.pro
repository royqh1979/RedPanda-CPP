QT += core gui printsupport network svg xml widgets testlib

CONFIG += c++17
CONFIG += qt console warn_on depend_includepath testcase
CONFIG += no_testcase_installs
CONFIG -= app_bundle

TEMPLATE = app

gcc {
    QMAKE_CXXFLAGS_RELEASE += -Werror=return-type
    QMAKE_CXXFLAGS_DEBUG += -Werror=return-type
}

msvc {
    DEFINES += NOMINMAX
}

SOURCES += \
    test/main.cpp \
    test/test_qdocument.cpp \
    test/test_charpos.cpp

HEADERS += \
    test/test_qdocument.h \
    test/test_charpos.h

SOURCES += qsynedit/codefolding.cpp \
    qsynedit/constants.cpp \
    qsynedit/document.cpp \
    qsynedit/formatter/cppformatter.cpp \
    qsynedit/formatter/formatter.cpp \
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
    qsynedit/syntaxer/gas.cpp \
    qsynedit/syntaxer/nasm.cpp \
    qsynedit/syntaxer/cpp.cpp \
    qsynedit/syntaxer/glsl.cpp \
    qsynedit/syntaxer/lua.cpp \
    qsynedit/types.cpp \
    qsynedit/syntaxer/makefile.cpp \
    qsynedit/syntaxer/textfile.cpp \
    qsynedit/syntaxer/syntaxer.cpp

HEADERS += \
    qsynedit/codefolding.h \
    qsynedit/constants.h \
    qsynedit/document.h \
    qsynedit/formatter/cppformatter.h \
    qsynedit/formatter/formatter.h \
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
    qsynedit/syntaxer/gas.h \
    qsynedit/syntaxer/nasm.h \
    qsynedit/syntaxer/cpp.h \
    qsynedit/syntaxer/glsl.h \
    qsynedit/syntaxer/lua.h \
    qsynedit/syntaxer/makefile.h \
    qsynedit/syntaxer/textfile.h \
    qsynedit/syntaxer/syntaxer.h

CONFIG(debug_and_release_target) {
    CONFIG(debug, debug|release) {
        OBJ_OUT_PWD = debug
    }
    CONFIG(release, debug|release) {
        OBJ_OUT_PWD = release
    }
}
INCLUDEPATH += ../redpanda_qt_utils

LIBS += -L$$OUT_PWD/../redpanda_qt_utils/$${OBJ_OUT_PWD} -lredpanda_qt_utils



