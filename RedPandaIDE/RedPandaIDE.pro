QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += nokey

QMAKE_CXXFLAGS_RELEASE += -Werror=return-type
QMAKE_CXXFLAGS_DEBUG += -Werror=return-type

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    HighlighterManager.cpp \
    caretlist.cpp \
    codeformatter.cpp \
    colorscheme.cpp \
    compiler/compiler.cpp \
    compiler/compilermanager.cpp \
    compiler/executablerunner.cpp \
    compiler/filecompiler.cpp \
    compiler/platform.cpp \
    compiler/stdincompiler.cpp \
    cpprefacter.cpp \
    parser/cppparser.cpp \
    parser/cpppreprocessor.cpp \
    parser/cpptokenizer.cpp \
    parser/parserutils.cpp \
    parser/statementmodel.cpp \
    qsynedit/Search.cpp \
    qsynedit/SearchBase.cpp \
    qsynedit/SearchRegex.cpp \
    settingsdialog/debuggeneralwidget.cpp \
    settingsdialog/editorautosavewidget.cpp \
    settingsdialog/editorcodecompletionwidget.cpp \
    settingsdialog/editormiscwidget.cpp \
    settingsdialog/formattergeneralwidget.cpp \
    widgets/classbrowser.cpp \
    widgets/codecompletionlistview.cpp \
    widgets/codecompletionpopup.cpp \
    widgets/cpudialog.cpp \
    debugger.cpp \
    editor.cpp \
    editorlist.cpp \
    iconsmanager.cpp \
    main.cpp \
    mainwindow.cpp \
    qsynedit/CodeFolding.cpp \
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
    settingsdialog/compilersetdirectorieswidget.cpp \
    settingsdialog/compilersetoptionwidget.cpp \
    settings.cpp \
    settingsdialog/editorclipboardwidget.cpp \
    settingsdialog/editorcolorschemewidget.cpp \
    settingsdialog/editorfontwidget.cpp \
    settingsdialog/editorgeneralwidget.cpp \
    settingsdialog/editorsymbolcompletionwidget.cpp \
    settingsdialog/editorsyntaxcheckwidget.cpp \
    settingsdialog/environmentappearencewidget.cpp \
    settingsdialog/executorgeneralwidget.cpp \
    settingsdialog/settingsdialog.cpp \
    settingsdialog/settingswidget.cpp \
    systemconsts.cpp \
    utils.cpp \
    widgets/coloredit.cpp \
    widgets/consolewidget.cpp \
    widgets/filepropertiesdialog.cpp \
    widgets/headercompletionpopup.cpp \
    widgets/issuestable.cpp \
    widgets/qconsole.cpp \
    widgets/qpatchedcombobox.cpp \
    widgets/searchdialog.cpp \
    widgets/searchresultview.cpp

HEADERS += \
    HighlighterManager.h \
    caretlist.h \
    codeformatter.h \
    colorscheme.h \
    compiler/compiler.h \
    compiler/compilermanager.h \
    compiler/executablerunner.h \
    compiler/filecompiler.h \
    compiler/stdincompiler.h \
    cpprefacter.h \
    parser/cppparser.h \
    parser/cpppreprocessor.h \
    parser/cpptokenizer.h \
    parser/parserutils.h \
    parser/statementmodel.h \
    platform.h \
    qsynedit/Search.h \
    qsynedit/SearchBase.h \
    qsynedit/SearchRegex.h \
    settingsdialog/debuggeneralwidget.h \
    settingsdialog/editorautosavewidget.h \
    settingsdialog/editorcodecompletionwidget.h \
    settingsdialog/editormiscwidget.h \
    settingsdialog/formattergeneralwidget.h \
    widgets/classbrowser.h \
    widgets/codecompletionlistview.h \
    widgets/codecompletionpopup.h \
    widgets/cpudialog.h \
    debugger.h \
    editor.h \
    editorlist.h \
    iconsmanager.h \
    mainwindow.h \
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
    settingsdialog/compilersetdirectorieswidget.h \
    settingsdialog/compilersetoptionwidget.h \
    settings.h \
    settingsdialog/editorclipboardwidget.h \
    settingsdialog/editorcolorschemewidget.h \
    settingsdialog/editorfontwidget.h \
    settingsdialog/editorgeneralwidget.h \
    settingsdialog/editorsymbolcompletionwidget.h \
    settingsdialog/editorsyntaxcheckwidget.h \
    settingsdialog/environmentappearencewidget.h \
    settingsdialog/executorgeneralwidget.h \
    settingsdialog/settingsdialog.h \
    settingsdialog/settingswidget.h \
    systemconsts.h \
    utils.h \
    common.h \
    widgets/coloredit.h \
    widgets/consolewidget.h \
    widgets/filepropertiesdialog.h \
    widgets/headercompletionpopup.h \
    widgets/issuestable.h \
    widgets/qconsole.h \
    widgets/qpatchedcombobox.h \
    widgets/searchdialog.h \
    widgets/searchresultview.h

FORMS += \
    settingsdialog/debuggeneralwidget.ui \
    settingsdialog/editorautosavewidget.ui \
    settingsdialog/editorcodecompletionwidget.ui \
    settingsdialog/editormiscwidget.ui \
    settingsdialog/formattergeneralwidget.ui \
    widgets/cpudialog.ui \
    mainwindow.ui \
    settingsdialog/compilersetdirectorieswidget.ui \
    settingsdialog/compilersetoptionwidget.ui \
    settingsdialog/editorclipboardwidget.ui \
    settingsdialog/editorcolorschemewidget.ui \
    settingsdialog/editorfontwidget.ui \
    settingsdialog/editorgeneralwidget.ui \
    settingsdialog/editorsymbolcompletionwidget.ui \
    settingsdialog/editorsyntaxcheckwidget.ui \
    settingsdialog/environmentappearencewidget.ui \
    settingsdialog/executorgeneralwidget.ui \
    settingsdialog/settingsdialog.ui \
    widgets/filepropertiesdialog.ui \
    widgets/searchdialog.ui

TRANSLATIONS += \
    RedPandaIDE_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    codes.qrc \
    colorschemes.qrc \
    themes/dark/dark.qrc \
    themes/light/light.qrc \
    themes/dracula/dracula.qrc \
    icons.qrc \
    translations.qrc

RC_ICONS = images/devcpp.ico

#win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../QScintilla/src/release/ -lqscintilla2_qt5d
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../QScintilla/src/debug/ -lqscintilla2_qt5d
#else:unix: LIBS += -L$$OUT_PWD/../../QScintilla/src/ -lqscintilla2_qt5d

#INCLUDEPATH += $$PWD/../../QScintilla/src
#DEPENDPATH += $$PWD/../../QScintilla/src
