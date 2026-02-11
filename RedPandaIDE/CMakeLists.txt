
# RedPandaIDE #

add_executable(RedPandaIDE WIN32 MACOSX_BUNDLE)

target_qt_plain_cpp(RedPandaIDE
    src/autolinkmanager
    src/colorscheme
    src/customfileiconprovider
    src/projectoptions
    src/settings
    src/syntaxermanager
    src/systemconsts
    src/utils
    src/visithistorymanager
    # compiler
    src/compiler/compilerinfo
    # debugger
    src/debugger/dapprotocol
    src/debugger/gdbmiresultparser
    # parser
    src/parser/cppparser
    src/parser/cpppreprocessor
    src/parser/cpptokenizer
    src/parser/parserutils
    src/parser/statementmodel
    # problems
    src/problems/competitivecompenionhandler
    src/problems/freeprojectsetformat
    src/problems/ojproblemset
    src/problems/problemcasevalidator
    # settings
    src/settings/basesettings
    src/settings/codecompletionsettings
    src/settings/codeformattersettings
    src/settings/compilersetsettings
    src/settings/compilesettings
    src/settings/debuggersettings
    src/settings/dirsettings
    src/settings/editorsettings
    src/settings/environmentsettings
    src/settings/executorsettings
    src/settings/languagesettings
    src/settings/uisettings
    # utils
    src/utils/escape
    src/utils/file
    src/utils/font
    src/utils/os
    src/utils/parsearg
    src/utils/parser
    src/utils/parsemacros
    src/utils/terminal
    src/utils/ui
    )

target_moc_classes(RedPandaIDE
    src/caretlist
    src/codesnippetsmanager
    src/cpprefacter
    src/editor
    src/editormanager
    src/iconsmanager
    src/main
    src/project
    src/projecttemplate
    src/shortcutmanager
    src/symbolusagemanager
    src/thememanager
    src/todoparser
    src/toolsmanager
    # compiler
    src/compiler/compiler
    src/compiler/compilermanager
    src/compiler/executablerunner
    src/compiler/filecompiler
    src/compiler/nasmfilecompiler
    src/compiler/ojproblemcasesrunner
    src/compiler/projectcompiler
    src/compiler/runner
    src/compiler/stdincompiler
    # debugger
    src/debugger/dapdebugger
    src/debugger/debugger
    src/debugger/debuggermodels
    src/debugger/gdbmidebugger
    # parser
    src/parser/cppparser
    src/parser/statementmodel
    # problems
    src/problems/competitivecompenionhandler
    # reformatter
    src/reformatter/basereformatter
    src/reformatter/astyleformatter
    # settings dialog
    src/settingsdialog/settingswidget
    # widgets
    src/widgets/bookmarkmodel
    src/widgets/classbrowser
    src/widgets/codecompletionlistview
    src/widgets/codecompletionpopup
    src/widgets/coloredit
    src/widgets/compileargumentswidget
    src/widgets/customdisablediconengine
    src/widgets/customfilesystemmodel
    src/widgets/darkfusionstyle
    src/widgets/editorstabwidget
    src/widgets/filenameeditdelegate
    src/widgets/functiontooltipwidget
    src/widgets/headercompletionpopup
    src/widgets/issuestable
    src/widgets/labelwithmenu
    src/widgets/lightfusionstyle
    src/widgets/linenumbertexteditor
    src/widgets/macroinfomodel
    src/widgets/ojproblemsetmodel
    src/widgets/qconsole
    src/widgets/qpatchedcombobox
    src/widgets/searchresultview
    src/widgets/shortcutinputedit
    src/widgets/shrinkabletabwidget)

target_ui_classes(RedPandaIDE
    src/mainwindow
    # settings dialog
    src/settingsdialog/compilerautolinkwidget
    src/settingsdialog/compilergaswidget
    src/settingsdialog/compilernasmwidget
    src/settingsdialog/compilersetdirectorieswidget
    src/settingsdialog/compilersetoptionwidget
    src/settingsdialog/debuggeneralwidget
    src/settingsdialog/editorautosavewidget
    src/settingsdialog/editorclipboardwidget
    src/settingsdialog/editorcodecompletionwidget
    src/settingsdialog/editorcolorschemewidget
    src/settingsdialog/editorcustomctypekeywords
    src/settingsdialog/editorfontwidget
    src/settingsdialog/editorgeneralwidget
    src/settingsdialog/editormiscwidget
    src/settingsdialog/editorsnippetwidget
    src/settingsdialog/editorsymbolcompletionwidget
    src/settingsdialog/editorsyntaxcheckwidget
    src/settingsdialog/editortooltipswidget
    src/settingsdialog/environmentappearancewidget
    src/settingsdialog/environmentfolderswidget
    src/settingsdialog/environmentperformancewidget
    src/settingsdialog/environmentprogramswidget
    src/settingsdialog/environmentshortcutwidget
    src/settingsdialog/executorgeneralwidget
    src/settingsdialog/executorproblemsetwidget
    src/settingsdialog/formattergeneralwidget
    src/settingsdialog/formatterpathwidget
    src/settingsdialog/languageasmgenerationwidget
    src/settingsdialog/projectcompileparamaterswidget
    src/settingsdialog/projectcompilerwidget
    src/settingsdialog/projectdirectorieswidget
    src/settingsdialog/projectdllhostwidget
    src/settingsdialog/projectfileswidget
    src/settingsdialog/projectgeneralwidget
    src/settingsdialog/projectmakefilewidget
    src/settingsdialog/projectoutputwidget
    src/settingsdialog/projectprecompilewidget
    src/settingsdialog/settingsdialog
    src/settingsdialog/toolsgeneralwidget
    # widgets
    src/widgets/aboutdialog
    src/widgets/choosethemedialog
    src/widgets/cpudialog
    src/widgets/custommakefileinfodialog
    src/widgets/editorfontdialog
    src/widgets/filepropertiesdialog
    src/widgets/infomessagebox
    src/widgets/newclassdialog
    src/widgets/newheaderdialog
    src/widgets/newprojectdialog
    src/widgets/newprojectunitdialog
    src/widgets/newtemplatedialog
    src/widgets/ojproblempropertywidget
    src/widgets/projectalreadyopendialog
    src/widgets/searchdialog
    src/widgets/searchinfiledialog
    src/widgets/signalmessagedialog)

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

#target_link_options(RedPandaIDE PRIVATE
#    $<$<CONFIG:Release>:-s -static>
#    $<$<CONFIG:MinSizeRel>:-s -static>
#)

#####################
# test-editor       #
#####################

add_executable(test-editor test/test-editor-main.cpp)

target_qt_plain_cpp(test-editor
    #settings
    src/settings/basesettings
    src/settings/dirsettings
    src/settings/editorsettings
    src/settings/codecompletionsettings

    src/parser/cppparser
    src/parser/cpppreprocessor
    src/parser/cpptokenizer
    src/parser/parserutils
    src/parser/statementmodel

    # utils
    src/utils/escape
    src/utils/file
    src/utils/font
    src/utils/os
    src/utils/parsearg
    src/utils/parsemacros
    src/utils/ui

    src/colorscheme
    src/syntaxermanager
    src/iconsmanager
    src/systemconsts
    src/compiler/compilerinfo
    src/reformatter/basereformatter
    )

target_moc_classes(test-editor
    src/editor

    src/widgets/bookmarkmodel
    src/widgets/codecompletionlistview
    src/widgets/codecompletionpopup
    src/widgets/customdisablediconengine
    src/widgets/functiontooltipwidget
    src/widgets/headercompletionpopup

    src/symbolusagemanager
    src/codesnippetsmanager
    #test
    test/test_editor_base
    test/test_editor_symbol_completion
)
target_include_directories(test-editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(test-editor PRIVATE
        Qt::Core
        Qt::Gui
        Qt::PrintSupport
        Qt::Svg
        Qt::Test
        Qt::Widgets
        Qt::Xml
        qsynedit
        redpanda_qt_utils)

if (QT_FEATURE_static)
    qt_import_plugins(test-editor
        INCLUDE Qt::QOffscreenIntegrationPlugin)
endif()

add_custom_command(
    TARGET test-editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_PROPERTY:test-editor,SOURCE_DIR>/test/resources"
        "$<TARGET_PROPERTY:test-editor,BINARY_DIR>/resources")

target_compile_definitions(test-editor PRIVATE
    ${GLOBAL_COMPILE_DEFINITIONS}
    APP_NAME=\"test-editor\"
    LIBEXECDIR=\"${CMAKE_INSTALL_LIBEXECDIR}\"
    REDPANDA_CPP_VERSION=\"${REDPANDA_VERSION}\"
    ARCH_X86_64=1
    ARCH_X86=1)

add_test(
    NAME test-editor
    COMMAND test-editor)

add_dependencies(all-test-targets test-editor)

#####################
# test-cppparser    #
#####################

add_executable(test-cppparser test/test-cppparser-main.cpp)

target_qt_plain_cpp(test-cppparser
    src/parser/cppparser
    src/parser/cpppreprocessor
    src/parser/cpptokenizer
    src/parser/parserutils
    src/parser/statementmodel
    )

target_moc_classes(test-cppparser
    #test
    test/test_cppparser
    test/test_cpppreprocessor
)
target_include_directories(test-cppparser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(test-cppparser PRIVATE
        Qt::Core
        Qt::Test
        qsynedit
        redpanda_qt_utils)

if (QT_FEATURE_static)
    qt_import_plugins(test-cppparser
        INCLUDE Qt::QOffscreenIntegrationPlugin)
endif()

add_custom_command(
    TARGET test-cppparser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_PROPERTY:test-editor,SOURCE_DIR>/test/resources"
        "$<TARGET_PROPERTY:test-editor,BINARY_DIR>/resources")

target_compile_definitions(test-cppparser PRIVATE
    ${GLOBAL_COMPILE_DEFINITIONS}
    APP_NAME=\"test-cppparser\"
    LIBEXECDIR=\"${CMAKE_INSTALL_LIBEXECDIR}\"
    REDPANDA_CPP_VERSION=\"${REDPANDA_VERSION}\"
    ARCH_X86_64=1
    ARCH_X86=1)

add_test(
    NAME test-cppparser
    COMMAND test-cppparser)

add_dependencies(all-test-targets test-cppparser)

#####################
# Platform-specific #
#####################

if(WIN32)
    target_ui_classes(RedPandaIDE
        src/settingsdialog/environmentfileassociationwidget
        src/settingsdialog/projectversioninfowidget)

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
        src/addon/luaapi
        src/addon/luaexecutor
        src/addon/luaruntime)

    target_compile_definitions(RedPandaIDE PRIVATE ENABLE_LUA_ADDON)

    target_link_libraries(RedPandaIDE PRIVATE lua)
endif()

if(SDCC)
    target_moc_classes(RedPandaIDE
        src/compiler/sdccfilecompiler
        src/compiler/sdccprojectcompiler)

    target_compile_definitions(RedPandaIDE PRIVATE ENABLE_SDCC)
endif()

if(VCS)
    target_qt_plain_cpp(RedPandaIDE
        src/settings/vcssettings)
    target_moc_classes(RedPandaIDE
        src/vcs/gitmanager
        src/vcs/gitrepository
        src/vcs/gitutils)

    target_ui_classes(RedPandaIDE
        src/settingsdialog/toolsgitwidget
        src/vcs/gitbranchdialog
        src/vcs/gitfetchdialog
        src/vcs/gitlogdialog
        src/vcs/gitmergedialog
        src/vcs/gitpulldialog
        src/vcs/gitpushdialog
        src/vcs/gitremotedialog
        src/vcs/gitresetdialog
        src/vcs/gituserconfigdialog)

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
