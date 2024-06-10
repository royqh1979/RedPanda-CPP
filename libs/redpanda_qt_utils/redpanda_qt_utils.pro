TEMPLATE = lib
QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += nokey
CONFIG += staticlib

win32: CONFIG += lrelease_dosdevice
else: CONFIG += lrelease
CONFIG += embed_translations
QMAKE_RESOURCE_FLAGS += -name $(QMAKE_TARGET)_${QMAKE_FILE_BASE}

win32: {
    DEFINES += _WIN32_WINNT=0x0501
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

TRANSLATIONS += \
    qt_utils_zh_CN.ts
