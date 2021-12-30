QT -= gui

APP_NAME = RedPandaIDE

CONFIG += c++11 console
CONFIG -= app_bundle

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

# Default rules for deployment.
qnx: target.path = /tmp/$${APP_NAME}/bin
else: unix:!android: target.path = /opt/$${APP_NAME}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ASLocalizer.h \
    astyle.h \
    astyle_main.h
