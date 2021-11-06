QT       += core gui printsupport network svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += nokey

gcc {
    QMAKE_CXXFLAGS_RELEASE += -Werror=return-type
    QMAKE_CXXFLAGS_DEBUG += -Werror=return-type
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ConvertUTF.c \
    HighlighterManager.cpp \
    autolinkmanager.cpp \
    caretlist.cpp \
    codeformatter.cpp \
    codesnippetsmanager.cpp \
    colorscheme.cpp \
    compiler/ojproblemcasesrunner.cpp \
    compiler/projectcompiler.cpp \
    compiler/runner.cpp \
    platform.cpp \
    compiler/compiler.cpp \
    compiler/compilermanager.cpp \
    compiler/executablerunner.cpp \
    compiler/filecompiler.cpp \
    compiler/stdincompiler.cpp \
    cpprefacter.cpp \
    parser/cppparser.cpp \
    parser/cpppreprocessor.cpp \
    parser/cpptokenizer.cpp \
    parser/parserutils.cpp \
    parser/statementmodel.cpp \
    problems/ojproblemset.cpp \
    problems/problemcasevalidator.cpp \
    project.cpp \
    projectoptions.cpp \
    projecttemplate.cpp \
    qsynedit/Search.cpp \
    qsynedit/SearchBase.cpp \
    qsynedit/SearchRegex.cpp \
    qsynedit/Types.cpp \
    settingsdialog/compilerautolinkwidget.cpp \
    settingsdialog/debuggeneralwidget.cpp \
    settingsdialog/editorautosavewidget.cpp \
    settingsdialog/editorcodecompletionwidget.cpp \
    settingsdialog/editormiscwidget.cpp \
    settingsdialog/editorsnippetwidget.cpp \
    settingsdialog/editortooltipswidget.cpp \
    settingsdialog/environmentfileassociationwidget.cpp \
    settingsdialog/environmentfolderswidget.cpp \
    settingsdialog/environmentshortcutwidget.cpp \
    settingsdialog/executorproblemsetwidget.cpp \
    settingsdialog/formattergeneralwidget.cpp \
    settingsdialog/projectcompileparamaterswidget.cpp \
    settingsdialog/projectcompilerwidget.cpp \
    settingsdialog/projectdirectorieswidget.cpp \
    settingsdialog/projectdllhostwidget.cpp \
    settingsdialog/projectfileswidget.cpp \
    settingsdialog/projectgeneralwidget.cpp \
    settingsdialog/projectmakefilewidget.cpp \
    settingsdialog/projectoutputwidget.cpp \
    settingsdialog/projectprecompilewidget.cpp \
    settingsdialog/projectversioninfowidget.cpp \
    settingsdialog/toolsgeneralwidget.cpp \
    shortcutmanager.cpp \
    symbolusagemanager.cpp \
    thememanager.cpp \
    todoparser.cpp \
    toolsmanager.cpp \
    widgets/aboutdialog.cpp \
    widgets/bookmarkmodel.cpp \
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
    widgets/custommakefileinfodialog.cpp \
    widgets/darkfusionstyle.cpp \
    widgets/editorstabwidget.cpp \
    widgets/filepropertiesdialog.cpp \
    widgets/functiontooltipwidget.cpp \
    widgets/headercompletionpopup.cpp \
    widgets/issuestable.cpp \
    widgets/labelwithmenu.cpp \
    widgets/macroinfomodel.cpp \
    widgets/newprojectdialog.cpp \
    widgets/ojproblempropertywidget.cpp \
    widgets/ojproblemsetmodel.cpp \
    widgets/qconsole.cpp \
    widgets/qpatchedcombobox.cpp \
    widgets/searchdialog.cpp \
    widgets/searchresultview.cpp

HEADERS += \
    ConvertUTF.h \
    HighlighterManager.h \
    SimpleIni.h \
    autolinkmanager.h \
    caretlist.h \
    codeformatter.h \
    codesnippetsmanager.h \
    colorscheme.h \
    compiler/compiler.h \
    compiler/compilermanager.h \
    compiler/executablerunner.h \
    compiler/filecompiler.h \
    compiler/ojproblemcasesrunner.h \
    compiler/projectcompiler.h \
    compiler/runner.h \
    compiler/stdincompiler.h \
    cpprefacter.h \
    parser/cppparser.h \
    parser/cpppreprocessor.h \
    parser/cpptokenizer.h \
    parser/parserutils.h \
    parser/statementmodel.h \
    platform.h \
    problems/ojproblemset.h \
    problems/problemcasevalidator.h \
    project.h \
    projectoptions.h \
    projecttemplate.h \
    qsynedit/Search.h \
    qsynedit/SearchBase.h \
    qsynedit/SearchRegex.h \
    settingsdialog/compilerautolinkwidget.h \
    settingsdialog/debuggeneralwidget.h \
    settingsdialog/editorautosavewidget.h \
    settingsdialog/editorcodecompletionwidget.h \
    settingsdialog/editormiscwidget.h \
    settingsdialog/editorsnippetwidget.h \
    settingsdialog/editortooltipswidget.h \
    settingsdialog/environmentfileassociationwidget.h \
    settingsdialog/environmentfolderswidget.h \
    settingsdialog/environmentshortcutwidget.h \
    settingsdialog/executorproblemsetwidget.h \
    settingsdialog/formattergeneralwidget.h \
    settingsdialog/projectcompileparamaterswidget.h \
    settingsdialog/projectcompilerwidget.h \
    settingsdialog/projectdirectorieswidget.h \
    settingsdialog/projectdllhostwidget.h \
    settingsdialog/projectfileswidget.h \
    settingsdialog/projectgeneralwidget.h \
    settingsdialog/projectmakefilewidget.h \
    settingsdialog/projectoutputwidget.h \
    settingsdialog/projectprecompilewidget.h \
    settingsdialog/projectversioninfowidget.h \
    settingsdialog/toolsgeneralwidget.h \
    shortcutmanager.h \
    symbolusagemanager.h \
    thememanager.h \
    todoparser.h \
    toolsmanager.h \
    widgets/aboutdialog.h \
    widgets/bookmarkmodel.h \
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
    widgets/custommakefileinfodialog.h \
    widgets/darkfusionstyle.h \
    widgets/editorstabwidget.h \
    widgets/filepropertiesdialog.h \
    widgets/functiontooltipwidget.h \
    widgets/headercompletionpopup.h \
    widgets/issuestable.h \
    widgets/labelwithmenu.h \
    widgets/macroinfomodel.h \
    widgets/newprojectdialog.h \
    widgets/ojproblempropertywidget.h \
    widgets/ojproblemsetmodel.h \
    widgets/qconsole.h \
    widgets/qpatchedcombobox.h \
    widgets/searchdialog.h \
    widgets/searchresultview.h

FORMS += \
    settingsdialog/compilerautolinkwidget.ui \
    settingsdialog/debuggeneralwidget.ui \
    settingsdialog/editorautosavewidget.ui \
    settingsdialog/editorcodecompletionwidget.ui \
    settingsdialog/editormiscwidget.ui \
    settingsdialog/editorsnippetwidget.ui \
    settingsdialog/editortooltipswidget.ui \
    settingsdialog/environmentfileassociationwidget.ui \
    settingsdialog/environmentfolderswidget.ui \
    settingsdialog/environmentshortcutwidget.ui \
    settingsdialog/executorproblemsetwidget.ui \
    settingsdialog/formattergeneralwidget.ui \
    settingsdialog/projectcompileparamaterswidget.ui \
    settingsdialog/projectcompilerwidget.ui \
    settingsdialog/projectdirectorieswidget.ui \
    settingsdialog/projectdllhostwidget.ui \
    settingsdialog/projectfileswidget.ui \
    settingsdialog/projectgeneralwidget.ui \
    settingsdialog/projectmakefilewidget.ui \
    settingsdialog/projectoutputwidget.ui \
    settingsdialog/projectprecompilewidget.ui \
    settingsdialog/projectversioninfowidget.ui \
    settingsdialog/toolsgeneralwidget.ui \
    widgets/aboutdialog.ui \
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
    widgets/custommakefileinfodialog.ui \
    widgets/filepropertiesdialog.ui \
    widgets/newprojectdialog.ui \
    widgets/ojproblempropertywidget.ui \
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
    defaultconfigs.qrc \
    themes.qrc \
    icons.qrc \
    translations.qrc

RC_ICONS = images/devcpp.ico images/associations/c.ico images/associations/cpp.ico images/associations/h.ico images/associations/hpp.ico images/associations/dev.ico
