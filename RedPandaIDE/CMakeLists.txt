add_executable(RedPandaIDE WIN32 MACOSX_BUNDLE)

target_qt_plain_cpp(RedPandaIDE
    autolinkmanager
    colorscheme
    customfileiconprovider
    projectoptions
    settings
    syntaxermanager
    systemconsts
    utils
    visithistorymanager
    # compiler
    compiler/compilerinfo
    # debugger
    debugger/dapprotocol
    debugger/gdbmiresultparser
    # parser
    parser/cppparser
    parser/cpppreprocessor
    parser/cpptokenizer
    parser/parserutils
    parser/statementmodel
    # problems
    problems/competitivecompenionhandler
    problems/freeprojectsetformat
    problems/ojproblemset
    problems/problemcasevalidator
    # utils
    utils/escape
    utils/font
    utils/parsearg)

target_moc_classes(RedPandaIDE
    caretlist
    codesnippetsmanager
    cpprefacter
    editor
    editormanager
    iconsmanager
    main
    project
    projecttemplate
    shortcutmanager
    symbolusagemanager
    thememanager
    todoparser
    toolsmanager
    # compiler
    compiler/compiler
    compiler/compilermanager
    compiler/executablerunner
    compiler/filecompiler
    compiler/nasmfilecompiler
    compiler/ojproblemcasesrunner
    compiler/projectcompiler
    compiler/runner
    compiler/stdincompiler
    # debugger
    debugger/dapdebugger
    debugger/debugger
    debugger/gdbmidebugger
    # parser
    parser/cppparser
    parser/statementmodel
    # problems
    problems/competitivecompenionhandler
    # settings dialog
    settingsdialog/settingswidget
    # widgets
    widgets/bookmarkmodel
    widgets/classbrowser
    widgets/codecompletionlistview
    widgets/codecompletionpopup
    widgets/coloredit
    widgets/compileargumentswidget
    widgets/customdisablediconengine
    widgets/customfilesystemmodel
    widgets/darkfusionstyle
    widgets/editorstabwidget
    widgets/filenameeditdelegate
    widgets/functiontooltipwidget
    widgets/headercompletionpopup
    widgets/issuestable
    widgets/labelwithmenu
    widgets/lightfusionstyle
    widgets/linenumbertexteditor
    widgets/macroinfomodel
    widgets/ojproblemsetmodel
    widgets/qconsole
    widgets/qpatchedcombobox
    widgets/searchresultview
    widgets/shortcutinputedit
    widgets/shrinkabletabwidget)

target_ui_classes(RedPandaIDE
    mainwindow
    # settings dialog
    settingsdialog/compilerautolinkwidget
    settingsdialog/compilergaswidget
    settingsdialog/compilernasmwidget
    settingsdialog/compilersetdirectorieswidget
    settingsdialog/compilersetoptionwidget
    settingsdialog/debuggeneralwidget
    settingsdialog/editorautosavewidget
    settingsdialog/editorclipboardwidget
    settingsdialog/editorcodecompletionwidget
    settingsdialog/editorcolorschemewidget
    settingsdialog/editorcustomctypekeywords
    settingsdialog/editorfontwidget
    settingsdialog/editorgeneralwidget
    settingsdialog/editormiscwidget
    settingsdialog/editorsnippetwidget
    settingsdialog/editorsymbolcompletionwidget
    settingsdialog/editorsyntaxcheckwidget
    settingsdialog/editortooltipswidget
    settingsdialog/environmentappearancewidget
    settingsdialog/environmentfolderswidget
    settingsdialog/environmentperformancewidget
    settingsdialog/environmentprogramswidget
    settingsdialog/environmentshortcutwidget
    settingsdialog/executorgeneralwidget
    settingsdialog/executorproblemsetwidget
    settingsdialog/formattergeneralwidget
    settingsdialog/formatterpathwidget
    settingsdialog/languageasmgenerationwidget
    settingsdialog/projectcompileparamaterswidget
    settingsdialog/projectcompilerwidget
    settingsdialog/projectdirectorieswidget
    settingsdialog/projectdllhostwidget
    settingsdialog/projectfileswidget
    settingsdialog/projectgeneralwidget
    settingsdialog/projectmakefilewidget
    settingsdialog/projectoutputwidget
    settingsdialog/projectprecompilewidget
    settingsdialog/settingsdialog
    settingsdialog/toolsgeneralwidget
    # widgets
    widgets/aboutdialog
    widgets/choosethemedialog
    widgets/cpudialog
    widgets/custommakefileinfodialog
    widgets/editorfontdialog
    widgets/filepropertiesdialog
    widgets/infomessagebox
    widgets/newclassdialog
    widgets/newheaderdialog
    widgets/newprojectdialog
    widgets/newprojectunitdialog
    widgets/newtemplatedialog
    widgets/ojproblempropertywidget
    widgets/projectalreadyopendialog
    widgets/searchdialog
    widgets/searchinfiledialog
    widgets/signalmessagedialog)

target_compile_definitions(RedPandaIDE PRIVATE
    ${GLOBAL_COMPILE_DEFINITIONS}
    APP_NAME=\"${APP_NAME}\"
    LIBEXECDIR=\"${CMAKE_INSTALL_LIBEXECDIR}\"
    REDPANDA_CPP_VERSION=\"${REDPANDA_VERSION}\"
    ARCH_X86_64=1
    ARCH_X86=1)

if(APP_VERSION_PRE_RELEASE)
    target_compile_definitions(RedPandaIDE PRIVATE
        APP_VERSION_SUFFIX=\"${APP_VERSION_PRE_RELEASE}\")
endif()

target_include_directories(RedPandaIDE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(RedPandaIDE PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::PrintSupport
    Qt::Svg
    Qt::Widgets
    Qt::Xml
    qsynedit
    redpanda_qt_utils)

target_link_options(RedPandaIDE PRIVATE
    $<$<CONFIG:Release>:-s>
    $<$<CONFIG:MinSizeRel>:-s>
)

#####################
# Platform-specific #
#####################

if(WIN32)
    target_ui_classes(RedPandaIDE
        settingsdialog/environmentfileassociationwidget
        settingsdialog/projectversioninfowidget)

    target_link_libraries(RedPandaIDE PRIVATE
        advapi32
        psapi
        shlwapi
        user32)
endif()

if(MACOS)
    target_link_libraries(RedPandaIDE PRIVATE Qt::GuiPrivate)
endif()

if(LINUX AND QT_FEATURE_static)
    qt_import_plugins(RedPandaIDE
        INCLUDE
            Qt::QComposePlatformInputContextPlugin
            Qt::QFcitx5PlatformInputContextPlugin
            Qt::QIbusPlatformInputContextPlugin)
endif()

if(XDG)
    install(
        TARGETS RedPandaIDE
        DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
    install(
        TARGETS RedPandaIDE
        BUNDLE DESTINATION .
        RUNTIME DESTINATION .)
endif()

###################
# Feature options #
###################

if(LUA_ADDON)
    target_qt_plain_cpp(RedPandaIDE
        addon/luaapi
        addon/luaexecutor
        addon/luaruntime)

    target_compile_definitions(RedPandaIDE PRIVATE ENABLE_LUA_ADDON)

    target_link_libraries(RedPandaIDE PRIVATE lua)
endif()

if(SDCC)
    target_moc_classes(RedPandaIDE
        compiler/sdccfilecompiler
        compiler/sdccprojectcompiler)

    target_compile_definitions(RedPandaIDE PRIVATE ENABLE_SDCC)
endif()

if(VCS)
    target_moc_classes(RedPandaIDE
        vcs/gitmanager
        vcs/gitrepository
        vcs/gitutils)

    target_ui_classes(RedPandaIDE
        settingsdialog/toolsgitwidget
        vcs/gitbranchdialog
        vcs/gitfetchdialog
        vcs/gitlogdialog
        vcs/gitmergedialog
        vcs/gitpulldialog
        vcs/gitpushdialog
        vcs/gitremotedialog
        vcs/gitresetdialog
        vcs/gituserconfigdialog)

    target_compile_definitions(RedPandaIDE PRIVATE ENABLE_VCS)
endif()

if(WINDOWS_PREFER_OPENCONSOLE)
    target_compile_definitions(RedPandaIDE PRIVATE WINDOWS_PREFER_OPENCONSOLE)
endif()

#############
# Resources #
#############

target_sources(RedPandaIDE PRIVATE
    fonts.qrc
    codes.qrc
    defaultconfigs.qrc
    icons.qrc
    projecttemplates.qrc)

# translation

target_embed_translations(RedPandaIDE
    TS_FILES
        translations/RedPandaIDE_pt_BR.ts
        translations/RedPandaIDE_ru_RU.ts
        translations/RedPandaIDE_zh_CN.ts
        translations/RedPandaIDE_zh_TW.ts)

if(QT_FEATURE_static)
    set(qtbase_translation_filenames
        qtbase_pt_BR.qm
        qtbase_ru_RU.qm
        qtbase_zh_CN.qm
        qtbase_zh_TW.qm)

    set(qtbase_translation_files)

    foreach(filename IN LISTS qtbase_translation_filenames)
        if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
            set(absolute_filename ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}/${filename})
        else()
            # there is no `QT5_INSTALL_TRANSLATIONS`. hard-code default path
            set(absolute_filename ${QT5_INSTALL_PREFIX}/translations/${filename})
        endif()
        if(EXISTS ${absolute_filename})
            list(APPEND qtbase_translation_files ${absolute_filename})
        endif()
    endforeach()

    if(qtbase_translation_files)
        target_embed_qtbase_translations(RedPandaIDE
            QM_FILES ${qtbase_translation_files})
    endif()
endif()

# appereance

file(GLOB_RECURSE resource_iconset_files
    "resources/iconsets/*.json"
    "resources/iconsets/*.svg")
target_embed_resources(RedPandaIDE
    RESOURCE_NAME iconsets
    FILES ${resource_iconset_files})

file(GLOB resource_theme_files
    "resources/themes/*.json"
    "resources/themes/*.lua"
    "resources/themes/*.png")
target_embed_resources(RedPandaIDE
    RESOURCE_NAME theme
    FILES ${resource_theme_files})

file(GLOB resource_colorscheme_files
    "resources/colorschemes/*.scheme")
target_embed_resources(RedPandaIDE
    RESOURCE_NAME colorschemes
    FILES ${resource_colorscheme_files})

# app icon

if(WIN32)
    target_sources(RedPandaIDE PRIVATE
        resources/windows_icon.rc)
elseif(MACOS)
    set(app_icon ../platform/macos/RedPandaIDE.icns)
    set_source_files_properties(${app_icon}
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(RedPandaIDE PRIVATE
        ${app_icon})
endif()
