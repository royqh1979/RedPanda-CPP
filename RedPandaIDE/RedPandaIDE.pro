QT       += core gui printsupport network svg xml widgets

CONFIG += c++17
CONFIG += nokey

win32: CONFIG += lrelease_dosdevice
else: CONFIG += lrelease
CONFIG += embed_translations

# uncomment the following line to enable vcs (git) support
# CONFIG += ENABLE_VCS

# uncomment the following line to enable sdcc support
CONFIG += ENABLE_SDCC

# uncomment the following line to enable Lua-based add-on support
# CONFIG += ENABLE_LUA_ADDON

APP_NAME = RedPandaCPP

include(../version.inc)

# TEST_VERSION = beta2
system(git rev-list HEAD --count): TEST_VERSION = $$system(git rev-list HEAD --count)

contains(QMAKE_HOST.arch, x86_64):{
    DEFINES += ARCH_X86_64=1
} else: {
    contains(QMAKE_HOST.arch, i386):{
        DEFINES += ARCH_X86=1
    }
    contains(QMAKE_HOST.arch, i686):{
        DEFINES += ARCH_X86=1
    }
}

macos: {
    QT += gui-private

    ICON = ../macos/RedPandaIDE.icns
}

win32: VERSION = $${APP_VERSION}.0
else: VERSION = $${APP_VERSION}

isEmpty(PREFIX) {
    PREFIX = /usr/local
}
isEmpty(LIBEXECDIR) {
    LIBEXECDIR = $${PREFIX}/libexec
}

win32: {
    DEFINES += _WIN32_WINNT=0x0501
    LIBS += -ladvapi32  # registry APIs
    LIBS += -lpsapi  # GetModuleFileNameEx, GetProcessMemoryInfo
    LIBS += -lshlwapi  # SHDeleteKey
    LIBS += -luser32  # window message APIs
}

DEFINES += PREFIX=\\\"$${PREFIX}\\\"
DEFINES += LIBEXECDIR=\\\"$${LIBEXECDIR}\\\"
DEFINES += APP_NAME=\\\"$${APP_NAME}\\\"

!isEmpty(APP_VERSION_SUFFIX): {
    DEFINES += APP_VERSION_SUFFIX=\\\"$${APP_VERSION_SUFFIX}\\\"
}

isEmpty(TEST_VERSION) {
    DEFINES += REDPANDA_CPP_VERSION=\\\"$${APP_VERSION}\\\"
} else {
    DEFINES += REDPANDA_CPP_VERSION=\\\"$${APP_VERSION}.$${TEST_VERSION}\\\"
}
win32 {
    _WINDOWS_PREFER_OPENCONSOLE = $$(WINDOWS_PREFER_OPENCONSOLE)
    equals(_WINDOWS_PREFER_OPENCONSOLE, "ON") {
        DEFINES += WINDOWS_PREFER_OPENCONSOLE
    }
}


gcc {
    QMAKE_CXXFLAGS_RELEASE += -Werror=return-type
    QMAKE_CXXFLAGS_DEBUG += -Werror=return-type
}

msvc {
    DEFINES += NOMINMAX
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


CONFIG(debug_and_release_target) {
    CONFIG(debug, debug|release) {
        OBJ_OUT_PWD = debug
    }
    CONFIG(release, debug|release) {
        OBJ_OUT_PWD = release
    }
}

INCLUDEPATH += ../libs/qsynedit ../libs/redpanda_qt_utils ../libs/lua

LIBS += -L$$OUT_PWD/../libs/qsynedit/$${OBJ_OUT_PWD} -lqsynedit \
        -L$$OUT_PWD/../libs/redpanda_qt_utils/$${OBJ_OUT_PWD} -lredpanda_qt_utils \
        -L$$OUT_PWD/../libs/lua/$${OBJ_OUT_PWD} -llua

SOURCES += \
    autolinkmanager.cpp \
    caretlist.cpp \
    codesnippetsmanager.cpp \
    colorscheme.cpp \
    compiler/compilerinfo.cpp \
    compiler/ojproblemcasesrunner.cpp \
    compiler/projectcompiler.cpp \
    compiler/runner.cpp \
    customfileiconprovider.cpp \
    compiler/compiler.cpp \
    compiler/compilermanager.cpp \
    compiler/executablerunner.cpp \
    compiler/filecompiler.cpp \
    compiler/stdincompiler.cpp \
    debugger/debugger.cpp \
    debugger/gdbmidebugger.cpp \
    debugger/gdbmiresultparser.cpp \
#    debugger/dapprotocol.cpp \
#    debugger/dapdebugger.cpp \    
    cpprefacter.cpp \
    parser/cppparser.cpp \
    parser/cpppreprocessor.cpp \
    parser/cpptokenizer.cpp \
    parser/parserutils.cpp \
    parser/statementmodel.cpp \
    problems/competitivecompenionhandler.cpp \
    problems/freeprojectsetformat.cpp \
    problems/ojproblemset.cpp \
    problems/problemcasevalidator.cpp \
    project.cpp \
    projectoptions.cpp \
    projecttemplate.cpp \
    settingsdialog/compilerautolinkwidget.cpp \
    settingsdialog/debuggeneralwidget.cpp \
    settingsdialog/editorautosavewidget.cpp \
    settingsdialog/editorcodecompletionwidget.cpp \
    settingsdialog/editorcustomctypekeywords.cpp \
    settingsdialog/editormiscwidget.cpp \
    settingsdialog/editorsnippetwidget.cpp \
    settingsdialog/editortooltipswidget.cpp \
    settingsdialog/environmentfolderswidget.cpp \
    settingsdialog/environmentperformancewidget.cpp \
    settingsdialog/environmentprogramswidget.cpp \
    settingsdialog/environmentshortcutwidget.cpp \
    settingsdialog/executorproblemsetwidget.cpp \
    settingsdialog/formattergeneralwidget.cpp \
    settingsdialog/formatterpathwidget.cpp \
    settingsdialog/languageasmgenerationwidget.cpp \
#    settingsdialog/languagecformatwidget.cpp \
    settingsdialog/projectcompileparamaterswidget.cpp \
    settingsdialog/projectcompilerwidget.cpp \
    settingsdialog/projectdirectorieswidget.cpp \
    settingsdialog/projectdllhostwidget.cpp \
    settingsdialog/projectfileswidget.cpp \
    settingsdialog/projectgeneralwidget.cpp \
    settingsdialog/projectmakefilewidget.cpp \
    settingsdialog/projectoutputwidget.cpp \
    settingsdialog/projectprecompilewidget.cpp \
    settingsdialog/toolsgeneralwidget.cpp \
    shortcutmanager.cpp \
    symbolusagemanager.cpp \
    syntaxermanager.cpp \
    thememanager.cpp \
    todoparser.cpp \
    toolsmanager.cpp \
    visithistorymanager.cpp \
    widgets/aboutdialog.cpp \
    widgets/bookmarkmodel.cpp \
    widgets/choosethemedialog.cpp \
    widgets/classbrowser.cpp \
    widgets/codecompletionlistview.cpp \
    widgets/codecompletionpopup.cpp \
    widgets/cpudialog.cpp \
    editor.cpp \
    editorlist.cpp \
    iconsmanager.cpp \
    main.cpp \
    mainwindow.cpp \
    settingsdialog/compilersetdirectorieswidget.cpp \
    settingsdialog/compilersetoptionwidget.cpp \
    settings.cpp \
    settingsdialog/editorclipboardwidget.cpp \
    settingsdialog/editorcolorschemewidget.cpp \
    settingsdialog/editorfontwidget.cpp \
    settingsdialog/editorgeneralwidget.cpp \
    settingsdialog/editorsymbolcompletionwidget.cpp \
    settingsdialog/editorsyntaxcheckwidget.cpp \
    settingsdialog/environmentappearancewidget.cpp \
    settingsdialog/executorgeneralwidget.cpp \
    settingsdialog/settingsdialog.cpp \
    settingsdialog/settingswidget.cpp \
    systemconsts.cpp \
    utils.cpp \
    utils/escape.cpp \
    utils/font.cpp \
    utils/parsearg.cpp \
    widgets/coloredit.cpp \
    widgets/compileargumentswidget.cpp \
    widgets/customdisablediconengine.cpp \
    widgets/customfilesystemmodel.cpp \
    widgets/custommakefileinfodialog.cpp \
    widgets/darkfusionstyle.cpp \
    widgets/editorfontdialog.cpp \
    widgets/editorstabwidget.cpp \
    widgets/filenameeditdelegate.cpp \
    widgets/filepropertiesdialog.cpp \
    widgets/functiontooltipwidget.cpp \
    widgets/headercompletionpopup.cpp \
    widgets/infomessagebox.cpp \
    widgets/issuestable.cpp \
    widgets/labelwithmenu.cpp \
    widgets/lightfusionstyle.cpp \
    widgets/linenumbertexteditor.cpp \
    widgets/macroinfomodel.cpp \
    widgets/newclassdialog.cpp \
    widgets/newheaderdialog.cpp \
    widgets/newprojectdialog.cpp \
    widgets/newprojectunitdialog.cpp \
    widgets/newtemplatedialog.cpp \
    widgets/ojproblempropertywidget.cpp \
    widgets/ojproblemsetmodel.cpp \
    widgets/projectalreadyopendialog.cpp \
    widgets/qconsole.cpp \
    widgets/qpatchedcombobox.cpp \
    widgets/searchdialog.cpp \
    widgets/searchinfiledialog.cpp \
    widgets/searchresultview.cpp \
    widgets/shortcutinputedit.cpp \
    widgets/shrinkabletabwidget.cpp \
    widgets/signalmessagedialog.cpp

HEADERS += \
    SimpleIni.h \
    addon/api.h \
    addon/executor.h \
    addon/runtime.h \
    autolinkmanager.h \
    caretlist.h \
    codesnippetsmanager.h \
    colorscheme.h \
    compiler/compiler.h \
    compiler/compilerinfo.h \
    compiler/compilermanager.h \
    compiler/executablerunner.h \
    compiler/filecompiler.h \
    compiler/ojproblemcasesrunner.h \
    compiler/projectcompiler.h \
    compiler/runner.h \
    compiler/stdincompiler.h \
    debugger/debugger.h \
    debugger/gdbmidebugger.h \
    debugger/gdbmiresultparser.h \
#    debugger/dapprotocol.h \    
#    debugger/dapdebugger.h \    
    cpprefacter.h \
    customfileiconprovider.h \
    parser/cppparser.h \
    parser/cpppreprocessor.h \
    parser/cpptokenizer.h \
    parser/parserutils.h \
    parser/statementmodel.h \
    problems/competitivecompenionhandler.h \
    problems/freeprojectsetformat.h \
    problems/ojproblemset.h \
    problems/problemcasevalidator.h \
    project.h \
    projectoptions.h \
    projecttemplate.h \
    settingsdialog/compilerautolinkwidget.h \
    settingsdialog/debuggeneralwidget.h \
    settingsdialog/editorautosavewidget.h \
    settingsdialog/editorcodecompletionwidget.h \
    settingsdialog/editorcustomctypekeywords.h \
    settingsdialog/editormiscwidget.h \
    settingsdialog/editorsnippetwidget.h \
    settingsdialog/editortooltipswidget.h \
    settingsdialog/environmentfolderswidget.h \
    settingsdialog/environmentperformancewidget.h \
    settingsdialog/environmentprogramswidget.h \
    settingsdialog/environmentshortcutwidget.h \
    settingsdialog/executorproblemsetwidget.h \
    settingsdialog/formattergeneralwidget.h \
    settingsdialog/formatterpathwidget.h \
    settingsdialog/languageasmgenerationwidget.h \
#    settingsdialog/languagecformatwidget.h \
    settingsdialog/projectcompileparamaterswidget.h \
    settingsdialog/projectcompilerwidget.h \
    settingsdialog/projectdirectorieswidget.h \
    settingsdialog/projectdllhostwidget.h \
    settingsdialog/projectfileswidget.h \
    settingsdialog/projectgeneralwidget.h \
    settingsdialog/projectmakefilewidget.h \
    settingsdialog/projectoutputwidget.h \
    settingsdialog/projectprecompilewidget.h \
    settingsdialog/toolsgeneralwidget.h \
    shortcutmanager.h \
    symbolusagemanager.h \
    syntaxermanager.h \
    thememanager.h \
    todoparser.h \
    toolsmanager.h \
    visithistorymanager.h \
    widgets/aboutdialog.h \
    widgets/bookmarkmodel.h \
    widgets/choosethemedialog.h \
    widgets/classbrowser.h \
    widgets/codecompletionlistview.h \
    widgets/codecompletionpopup.h \
    widgets/cpudialog.h \
    editor.h \
    editorlist.h \
    iconsmanager.h \
    mainwindow.h \
    settingsdialog/compilersetdirectorieswidget.h \
    settingsdialog/compilersetoptionwidget.h \
    settings.h \
    settingsdialog/editorclipboardwidget.h \
    settingsdialog/editorcolorschemewidget.h \
    settingsdialog/editorfontwidget.h \
    settingsdialog/editorgeneralwidget.h \
    settingsdialog/editorsymbolcompletionwidget.h \
    settingsdialog/editorsyntaxcheckwidget.h \
    settingsdialog/environmentappearancewidget.h \
    settingsdialog/executorgeneralwidget.h \
    settingsdialog/settingsdialog.h \
    settingsdialog/settingswidget.h \
    systemconsts.h \
    utils.h \
    utils/escape.h \
    utils/font.h \
    utils/parsearg.h \
    common.h \
    widgets/coloredit.h \
    widgets/compileargumentswidget.h \
    widgets/customdisablediconengine.h \
    widgets/customfilesystemmodel.h \
    widgets/custommakefileinfodialog.h \
    widgets/darkfusionstyle.h \
    widgets/editorfontdialog.h \
    widgets/editorstabwidget.h \
    widgets/filenameeditdelegate.h \
    widgets/filepropertiesdialog.h \
    widgets/functiontooltipwidget.h \
    widgets/headercompletionpopup.h \
    widgets/infomessagebox.h \
    widgets/issuestable.h \
    widgets/labelwithmenu.h \
    widgets/lightfusionstyle.h \
    widgets/linenumbertexteditor.h \
    widgets/macroinfomodel.h \
    widgets/newclassdialog.h \
    widgets/newheaderdialog.h \
    widgets/newprojectdialog.h \
    widgets/newprojectunitdialog.h \
    widgets/newtemplatedialog.h \
    widgets/ojproblempropertywidget.h \
    widgets/ojproblemsetmodel.h \
    widgets/projectalreadyopendialog.h \
    widgets/qconsole.h \
    widgets/qpatchedcombobox.h \
    widgets/searchdialog.h \
    widgets/searchinfiledialog.h \
    widgets/searchresultview.h \
    widgets/shortcutinputedit.h \
    widgets/shrinkabletabwidget.h \
    widgets/signalmessagedialog.h

FORMS += \
    settingsdialog/compilerautolinkwidget.ui \
    settingsdialog/debuggeneralwidget.ui \
    settingsdialog/editorautosavewidget.ui \
    settingsdialog/editorcodecompletionwidget.ui \
    settingsdialog/editorcustomctypekeywords.ui \
    settingsdialog/editormiscwidget.ui \
    settingsdialog/editorsnippetwidget.ui \
    settingsdialog/editortooltipswidget.ui \
    settingsdialog/environmentfolderswidget.ui \
    settingsdialog/environmentperformancewidget.ui \
    settingsdialog/environmentprogramswidget.ui \
    settingsdialog/environmentshortcutwidget.ui \
    settingsdialog/executorproblemsetwidget.ui \
    settingsdialog/formattergeneralwidget.ui \
    settingsdialog/formatterpathwidget.ui \
    settingsdialog/languageasmgenerationwidget.ui \
#    settingsdialog/languagecformatwidget.ui \
    settingsdialog/projectcompileparamaterswidget.ui \
    settingsdialog/projectcompilerwidget.ui \
    settingsdialog/projectdirectorieswidget.ui \
    settingsdialog/projectdllhostwidget.ui \
    settingsdialog/projectfileswidget.ui \
    settingsdialog/projectgeneralwidget.ui \
    settingsdialog/projectmakefilewidget.ui \
    settingsdialog/projectoutputwidget.ui \
    settingsdialog/projectprecompilewidget.ui \
    settingsdialog/toolsgeneralwidget.ui \
    widgets/aboutdialog.ui \
    widgets/choosethemedialog.ui \
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
    settingsdialog/environmentappearancewidget.ui \
    settingsdialog/executorgeneralwidget.ui \
    settingsdialog/settingsdialog.ui \
    widgets/custommakefileinfodialog.ui \
    widgets/editorfontdialog.ui \
    widgets/filepropertiesdialog.ui \
    widgets/infomessagebox.ui \
    widgets/newclassdialog.ui \
    widgets/newheaderdialog.ui \
    widgets/newprojectdialog.ui \
    widgets/newprojectunitdialog.ui \
    widgets/newtemplatedialog.ui \
    widgets/ojproblempropertywidget.ui \
    widgets/projectalreadyopendialog.ui \
    widgets/searchdialog.ui \
    widgets/searchinfiledialog.ui \
    widgets/signalmessagedialog.ui

ENABLE_SDCC {
    DEFINES += ENABLE_SDCC

    SOURCES += \
        compiler/sdccfilecompiler.cpp \
        compiler/sdccprojectcompiler.cpp

    HEADERS += \
        compiler/sdccfilecompiler.h \
        compiler/sdccprojectcompiler.h

}

ENABLE_LUA_ADDON {
    DEFINES += ENABLE_LUA_ADDON

    SOURCES += \
        addon/api.cpp \
        addon/executor.cpp \
        addon/runtime.cpp

    HEADERS += \
        addon/api.h \
        addon/executor.h \
        addon/runtime.h
}

ENABLE_VCS {

    DEFINES += ENABLE_VCS
    SOURCES += \
        vcs/gitbranchdialog.cpp \
        vcs/gitfetchdialog.cpp \
        vcs/gitlogdialog.cpp \
        vcs/gitmanager.cpp \
        vcs/gitmergedialog.cpp \
        vcs/gitpulldialog.cpp \
        vcs/gitpushdialog.cpp \
        vcs/gitremotedialog.cpp \
        vcs/gitrepository.cpp \
        vcs/gitresetdialog.cpp \
        vcs/gituserconfigdialog.cpp \
        vcs/gitutils.cpp \
        settingsdialog/toolsgitwidget.cpp

    HEADERS += \
        vcs/gitbranchdialog.h \
        vcs/gitfetchdialog.h \
        vcs/gitlogdialog.h \
        vcs/gitmanager.h \
        vcs/gitmergedialog.h \
        vcs/gitpulldialog.h \
        vcs/gitpushdialog.h \
        vcs/gitremotedialog.h \
        vcs/gitrepository.h \
        vcs/gitresetdialog.h \
        vcs/gituserconfigdialog.h \
        vcs/gitutils.h \
        settingsdialog/toolsgitwidget.h


    FORMS += \
        vcs/gitbranchdialog.ui \
        vcs/gitfetchdialog.ui \
        vcs/gitlogdialog.ui \
        vcs/gitmergedialog.ui \
        vcs/gitpulldialog.ui \
        vcs/gitpushdialog.ui \
        vcs/gitremotedialog.ui \
        vcs/gitresetdialog.ui \
        vcs/gituserconfigdialog.ui \
        settingsdialog/toolsgitwidget.ui
}

win32: {
    FORMS +=  \
        settingsdialog/projectversioninfowidget.ui \
    settingsdialog/environmentfileassociationwidget.ui

    HEADERS += \
    settingsdialog/projectversioninfowidget.h \
    settingsdialog/environmentfileassociationwidget.h

    SOURCES += \
    settingsdialog/environmentfileassociationwidget.cpp \
    settingsdialog/projectversioninfowidget.cpp
}

linux: {
    # legacy glibc compatibility -- modern Unices have all components in `libc.so`
    LIBS += -lrt -ldl

    _LINUX_STATIC_IME_PLUGIN = $$(LINUX_STATIC_IME_PLUGIN)
    equals(_LINUX_STATIC_IME_PLUGIN, "ON") {
        SOURCES += \
            resources/linux_static_ime_plugin.cpp
        QTPLUGIN.platforminputcontexts += \
            composeplatforminputcontextplugin \
            fcitx5platforminputcontextplugin \
            ibusplatforminputcontextplugin
        LIBS += -L$$[QT_INSTALL_PLUGINS]/platforminputcontexts
    }
}

TRANSLATIONS += \
    translations/RedPandaIDE_zh_CN.ts \
    translations/RedPandaIDE_zh_TW.ts \
    translations/RedPandaIDE_pt_BR.ts

win32: {
    !isEmpty(PREFIX) {
        target.path = $${PREFIX}
    }
}

# Default rules for deployment.
qnx: target.path = $${PREFIX}/bin
else: unix:!android: target.path = $${PREFIX}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    fonts.qrc \
    codes.qrc \
    defaultconfigs.qrc \
    icons.qrc \
    projecttemplates.qrc

RC_ICONS = images/devcpp.ico images/associations/c.ico images/associations/cpp.ico images/associations/dev.ico images/associations/c.ico images/associations/cpp.ico images/associations/h.ico images/associations/hpp.ico

iconsets_files.files += $$files(resources/iconsets/*.svg, true)
iconsets_files.files += $$files(resources/iconsets/*.json, true)

theme_files.files += $$files(resources/themes/*.lua, false)
theme_files.files += $$files(resources/themes/*.json, false)
theme_files.files += $$files(resources/themes/*.png, false)

colorscheme_files.files += $$files(resources/colorschemes/*.scheme, false)

RESOURCES += iconsets_files
RESOURCES += theme_files
RESOURCES += colorscheme_files

qtConfig(static) {
    qt_translation_files.files += $$[QT_INSTALL_TRANSLATIONS]/qtbase_pt_BR.qm
    qt_translation_files.files += $$[QT_INSTALL_TRANSLATIONS]/qtbase_zh_CN.qm
    qt_translation_files.files += $$[QT_INSTALL_TRANSLATIONS]/qtbase_zh_TW.qm
    qt_translation_files.base = $$[QT_INSTALL_TRANSLATIONS]
    qt_translation_files.prefix = /translations

    RESOURCES += qt_translation_files
}

macos: {
    # Add needed executables into the main app bundle
    bundled_executable.files = $$OUT_PWD/../tools/consolepauser/consolepauser
    bundled_executable.path = Contents/MacOS

    # Also bundled templates

    QMAKE_BUNDLE_DATA += bundled_executable
}
