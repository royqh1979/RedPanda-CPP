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

SOURCES += qt_utils/utils.cpp \
	qt_utils/charsetinfo.cpp

HEADERS += qt_utils/utils.h \
	qt_utils/charsetinfo.h
