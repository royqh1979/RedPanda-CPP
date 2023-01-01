QT       += core gui printsupport network svg xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += nokey

isEmpty(APP_NAME) {
    APP_NAME = RedPandaCPP
}

isEmpty(APP_VERSION) {
    APP_VERSION = 2.8
}

macos: {
    # This package needs to be installed via homebrew before we can compile it
    INCLUDEPATH += \
        /opt/homebrew/opt/icu4c/include

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

# windows 7 is the minimum windows version
win32: {
DEFINES += _WIN32_WINNT=0x0601
}

DEFINES += PREFIX=\\\"$${PREFIX}\\\"
DEFINES += LIBEXECDIR=\\\"$${LIBEXECDIR}\\\"
DEFINES += APP_NAME=\\\"$${APP_NAME}\\\"
DEFINES += REDPANDA_CPP_VERSION=\\\"$${APP_VERSION}\\\"

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
        OBJ_OUT_PWD = debug/
    }
    CONFIG(release, debug|release) {
        OBJ_OUT_PWD = release/
    }
}

INCLUDEPATH += ../libs/qsynedit ../libs/redpanda_qt_utils

gcc | clang {
LIBS += $$OUT_PWD/../libs/qsynedit/$${OBJ_OUT_PWD}libqsynedit.a \
        $$OUT_PWD/../libs/redpanda_qt_utils/$${OBJ_OUT_PWD}libredpanda_qt_utils.a
}
msvc {
LIBS += $$OUT_PWD/../libs/qsynedit/$${OBJ_OUT_PWD}qsynedit.lib \
        $$OUT_PWD/../libs/redpanda_qt_utils/$${OBJ_OUT_PWD}redpanda_qt_utils.lib
LIBS += advapi32.lib user32.lib
}

SOURCES += \
    autolinkmanager.cpp \
    caretlist.cpp \
    codeformatter.cpp \
    codesnippetsmanager.cpp \
    colorscheme.cpp \
    compiler/compilerinfo.cpp \
    compiler/ojproblemcasesrunner.cpp \
    compiler/projectcompiler.cpp \
    compiler/runner.cpp \
    customfileiconprovider.cpp \
    gdbmiresultparser.cpp \
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
    settingsdialog/toolsgeneralwidget.cpp \
    settingsdialog/toolsgitwidget.cpp \
    shortcutmanager.cpp \
    symbolusagemanager.cpp \
    syntaxermanager.cpp \
    thememanager.cpp \
    todoparser.cpp \
    toolsmanager.cpp \
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
    visithistorymanager.cpp \
    widgets/aboutdialog.cpp \
    widgets/bookmarkmodel.cpp \
    widgets/choosethemedialog.cpp \
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
    widgets/compileargumentswidget.cpp \
    widgets/consolewidget.cpp \
    widgets/customdisablediconengine.cpp \
    widgets/customfilesystemmodel.cpp \
    widgets/custommakefileinfodialog.cpp \
    widgets/darkfusionstyle.cpp \
    widgets/editorstabwidget.cpp \
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
    widgets/replacedialog.cpp \
    widgets/searchdialog.cpp \
    widgets/searchinfiledialog.cpp \
    widgets/searchresultview.cpp \
    widgets/shortcutinputedit.cpp \
    widgets/shrinkabletabwidget.cpp \
    widgets/signalmessagedialog.cpp

HEADERS += \
    SimpleIni.h \
    autolinkmanager.h \
    caretlist.h \
    codeformatter.h \
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
    cpprefacter.h \
    customfileiconprovider.h \
    gdbmiresultparser.h \
    parser/cppparser.h \
    parser/cpppreprocessor.h \
    parser/cpptokenizer.h \
    parser/parserutils.h \
    parser/statementmodel.h \
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
    settingsdialog/toolsgeneralwidget.h \
    settingsdialog/toolsgitwidget.h \
    shortcutmanager.h \
    symbolusagemanager.h \
    syntaxermanager.h \
    thememanager.h \
    todoparser.h \
    toolsmanager.h \
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
    visithistorymanager.h \
    widgets/aboutdialog.h \
    widgets/bookmarkmodel.h \
    widgets/choosethemedialog.h \
    widgets/classbrowser.h \
    widgets/codecompletionlistview.h \
    widgets/codecompletionpopup.h \
    widgets/cpudialog.h \
    debugger.h \
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
    settingsdialog/environmentappearencewidget.h \
    settingsdialog/executorgeneralwidget.h \
    settingsdialog/settingsdialog.h \
    settingsdialog/settingswidget.h \
    systemconsts.h \
    utils.h \
    common.h \
    widgets/coloredit.h \
    widgets/compileargumentswidget.h \
    widgets/consolewidget.h \
    widgets/customdisablediconengine.h \
    widgets/customfilesystemmodel.h \
    widgets/custommakefileinfodialog.h \
    widgets/darkfusionstyle.h \
    widgets/editorstabwidget.h \
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
    widgets/replacedialog.h \
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
    settingsdialog/toolsgeneralwidget.ui \
    settingsdialog/toolsgitwidget.ui \
    vcs/gitbranchdialog.ui \
    vcs/gitfetchdialog.ui \
    vcs/gitlogdialog.ui \
    vcs/gitmergedialog.ui \
    vcs/gitpulldialog.ui \
    vcs/gitpushdialog.ui \
    vcs/gitremotedialog.ui \
    vcs/gitresetdialog.ui \
    vcs/gituserconfigdialog.ui \
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
    settingsdialog/environmentappearencewidget.ui \
    settingsdialog/executorgeneralwidget.ui \
    settingsdialog/settingsdialog.ui \
    widgets/custommakefileinfodialog.ui \
    widgets/filepropertiesdialog.ui \
    widgets/infomessagebox.ui \
    widgets/newclassdialog.ui \
    widgets/newheaderdialog.ui \
    widgets/newprojectdialog.ui \
    widgets/newprojectunitdialog.ui \
    widgets/newtemplatedialog.ui \
    widgets/ojproblempropertywidget.ui \
    widgets/projectalreadyopendialog.ui \
    widgets/replacedialog.ui \
    widgets/searchdialog.ui \
    widgets/searchinfiledialog.ui \
    widgets/signalmessagedialog.ui

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

unix: {
    HEADERS += \
    settingsdialog/formatterpathwidget.h \
    settingsdialog/environmentprogramswidget.h

    SOURCES += \
    settingsdialog/formatterpathwidget.cpp \
    settingsdialog/environmentprogramswidget.cpp

    FORMS += \
    settingsdialog/formatterpathwidget.ui \
    settingsdialog/environmentprogramswidget.ui
}

linux: {
    LIBS+= \
    -lrt
}

TRANSLATIONS += \
    translations/RedPandaIDE_zh_CN.ts \
    translations/RedPandaIDE_zh_TW.ts \
    translations/RedPandaIDE_pt_BR.ts

#CONFIG += lrelease embed_translations

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
    codes.qrc \
    defaultconfigs.qrc \
    icons.qrc \
    projecttemplates.qrc \
    translations.qrc

RC_ICONS = images/devcpp.ico images/associations/c.ico images/associations/cpp.ico images/associations/dev.ico images/associations/c.ico images/associations/cpp.ico images/associations/h.ico images/associations/hpp.ico

# fixed lrelease.prf
qtPrepareTool(QMAKE_LRELEASE, lrelease)

isEmpty(LRELEASE_DIR): LRELEASE_DIR = .qm
isEmpty(QM_FILES_RESOURCE_PREFIX): QM_FILES_RESOURCE_PREFIX = i18n

lrelease.name = lrelease
lrelease.input = TRANSLATIONS EXTRA_TRANSLATIONS
lrelease.output = $$LRELEASE_DIR/${QMAKE_FILE_IN_BASE}.qm
lrelease.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} $$QMAKE_LRELEASE_FLAGS -qm ${QMAKE_FILE_OUT}
silent: lrelease.commands = @echo lrelease ${QMAKE_FILE_IN} && $$lrelease.commands
lrelease.CONFIG = no_link target_predeps
QMAKE_EXTRA_COMPILERS += lrelease

all_translations = $$TRANSLATIONS $$EXTRA_TRANSLATIONS
for (translation, all_translations) {
    # mirrors $$LRELEASE_DIR/${QMAKE_FILE_IN_BASE}.qm above
    translation = $$basename(translation)
    QM_FILES += $$OUT_PWD/$$LRELEASE_DIR/$$replace(translation, \\..*$, .qm)
}

qmake_qm_files.files = $$QM_FILES
qmake_qm_files.base = $$OUT_PWD/$$LRELEASE_DIR
qmake_qm_files.prefix = $$QM_FILES_RESOURCE_PREFIX

iconsets_files.files += $$files(resources/iconsets/*.svg, true)
iconsets_files.files += $$files(resources/iconsets/*.json, true)

theme_files.files += $$files(themes/*.json, false)
theme_files.files += $$files(themes/*.png, false)

colorscheme_files.files += $$files(colorschemes/*.scheme, false)
colorscheme_files.prefix = /colorschemes

RESOURCES += qmake_qm_files
RESOURCES += iconsets_files
RESOURCES += theme_files
RESOURCES += colorscheme_files

macos: {
    # Add needed executables into the main app bundle
    bundled_executable.files = \
        $$OUT_PWD/../tools/astyle/astyle \
        $$OUT_PWD/../tools/consolepauser/consolepauser \
        $$OUT_PWD/../tools/redpanda-git-askpass/redpanda-git-askpass.app/Contents/MacOS/redpanda-git-askpass
    bundled_executable.path = Contents/MacOS

    # Also bundled templates

    QMAKE_BUNDLE_DATA += bundled_executable
}
