QT -= gui

isEmpty(APP_NAME) {
    APP_NAME = RedPandaCPP
}

CONFIG += c++11 console
CONFIG -= app_bundle

msvc {
CONFIG += windows
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ASBeautifier.cpp \
    ASEnhancer.cpp \
    ASFormatter.cpp \
    ASLocalizer.cpp \
    ASResource.cpp \
    astyle_main.cpp

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

win32: {
    !isEmpty(PREFIX) {
        target.path = $${PREFIX}
    }
}

win32-msvc {
QMAKE_CFLAGS += /source-charset:utf-8
QMAKE_CXXFLAGS += /source-charset:utf-8
}

# Default rules for deployment.
qnx: target.path = $${PREFIX}/libexec/$${APP_NAME}
else: unix:!android: target.path = $${PREFIX}/libexec/$${APP_NAME}

!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ASLocalizer.h \
    astyle.h \
    astyle_main.h
