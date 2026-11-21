
# RedPandaIDE #

add_executable(RedPandaIDE WIN32)

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
    src/parser/pptoken
    src/parser/ppexpander
    src/parser/cpptokenizer
    src/parser/cpptokencursor
    src/parser/cpptypes
    src/parser/cppdeclarationparser
    src/parser/cppsyntaxparser
    src/parser/cppstatementbuilder
    src/parser/cpptyperesolver
    src/parser/typeoperationregistry
    src/parser/cppexpressionevaluator
    src/parser/cppscopes
    src/parser/parserutils
    src/parser/statementmodel
    # editor
    src/editor/cppexpressionextractor
    # search
    src/search/filesearchengine
    src/search/searchresultreplacer
    # refactor
    src/refactor/cpprefactor
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
    src/utils/pe
    src/utils/terminal
    src/utils/ui
    )

target_moc_classes(RedPandaIDE
    src/caretlist
    src/codesnippetsmanager
    src/editor/editor
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
    # refactor
    src/refactor/cpprefactor
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

add_custom_target(test-resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/test/resources"
        "${CMAKE_CURRENT_BINARY_DIR}/resources")

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
    src/parser/pptoken
    src/parser/ppexpander
    src/parser/cpptokenizer
    src/parser/cpptokencursor
    src/parser/cpptypes
    src/parser/cppdeclarationparser
    src/parser/cppsyntaxparser
    src/parser/cppstatementbuilder
    src/parser/cpptyperesolver
    src/parser/typeoperationregistry
    src/parser/cppexpressionevaluator
    src/parser/cppscopes
    src/parser/parserutils
    src/parser/statementmodel

    src/editor/cppexpressionextractor

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
    src/editor/editor

    src/widgets/bookmarkmodel
    src/widgets/codecompletionlistview
    src/widgets/codecompletionpopup
    src/widgets/customdisablediconengine
    src/widgets/functiontooltipwidget
    src/widgets/headercompletionpopup

    src/symbolusagemanager
    src/codesnippetsmanager
    #test
    test/test_cppexpressionextractor
    test/test_editor_base
    test/test_editor_display
    test/test_editor_general
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

target_compile_definitions(test-editor PRIVATE
    ${GLOBAL_COMPILE_DEFINITIONS}
    APP_NAME=\"test-editor\"
    LIBEXECDIR=\"${CMAKE_INSTALL_LIBEXECDIR}\"
    REDPANDA_CPP_VERSION=\"${REDPANDA_VERSION}\"
    ARCH_X86_64=1
    ARCH_X86=1

    # pacman makepkg adds `-ffile-prefix-map=$srcdir=/usr/src/debug/$pkgname`
    # `QFINDTESTDATA` won't find test data without explicitly setting the paths
    QT_TESTCASE_SOURCEDIR="${CMAKE_CURRENT_SOURCE_DIR}/test")

add_test(
    NAME test-editor
    COMMAND test-editor)

add_dependencies(all-test-targets test-editor)

#####################
# test-widgets      #
#####################

add_executable(test-widgets test/test-widgets-main.cpp)

target_qt_plain_cpp(test-widgets
    src/settings
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

    src/parser/cppparser
    src/parser/cpppreprocessor
    src/parser/pptoken
    src/parser/ppexpander
    src/parser/cpptokenizer
    src/parser/cpptokencursor
    src/parser/cpptypes
    src/parser/cppdeclarationparser
    src/parser/cppsyntaxparser
    src/parser/cppstatementbuilder
    src/parser/cpptyperesolver
    src/parser/typeoperationregistry
    src/parser/cppexpressionevaluator
    src/parser/cppscopes
    src/parser/parserutils
    src/parser/statementmodel

    src/editor/cppexpressionextractor

    src/utils
    src/utils/escape
    src/utils/file
    src/utils/font
    src/utils/os
    src/utils/parsearg
    src/utils/parsemacros
    src/utils/pe
    src/utils/ui

    src/compiler/runner
    src/compiler/executablerunner
    src/refactor/cpprefactor
    src/projectoptions
    src/colorscheme
    src/syntaxermanager
    src/iconsmanager
    src/systemconsts
    src/compiler/compilerinfo
    src/reformatter/basereformatter
    test/test_compilersettings
    )

target_moc_classes(test-widgets
    src/editor/editor

    src/settingsdialog/settingswidget
    src/widgets/bookmarkmodel
    src/widgets/classbrowser
    src/widgets/codecompletionlistview
    src/widgets/codecompletionpopup
    src/widgets/coloredit
    src/widgets/customdisablediconengine
    src/widgets/qpatchedcombobox
    src/widgets/functiontooltipwidget
    src/widgets/headercompletionpopup

    src/symbolusagemanager
    src/codesnippetsmanager
    test/test_widgets
    test/test_compilersettings
    )
target_ui_classes(test-widgets
    src/settingsdialog/editorcolorschemewidget
    )
target_include_directories(test-widgets PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(test-widgets PRIVATE
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
    qt_import_plugins(test-widgets
        INCLUDE Qt::QOffscreenIntegrationPlugin)
endif()

target_compile_definitions(test-widgets PRIVATE
    ${GLOBAL_COMPILE_DEFINITIONS}
    APP_NAME=\"test-widgets\"
    LIBEXECDIR=\"${CMAKE_INSTALL_LIBEXECDIR}\"
    REDPANDA_CPP_VERSION=\"${REDPANDA_VERSION}\"
    ARCH_X86_64=1
    ARCH_X86=1)

add_test(
    NAME test-widgets
    COMMAND test-widgets)

add_dependencies(all-test-targets test-widgets)

#####################
# test-file-search  #
#####################

add_executable(test-file-search test/test-file-search-main.cpp)

target_qt_plain_cpp(test-file-search
    src/search/filesearchengine
    src/search/searchresultreplacer
    src/widgets/searchresultview
    test/test_filesearchengine
    test/test_searchresultmodel
    test/test_searchresultreplacer)

target_moc_classes(test-file-search
    src/widgets/searchresultview
    test/test_filesearchengine
    test/test_searchresultmodel
    test/test_searchresultreplacer)

target_include_directories(test-file-search PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(test-file-search PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Test
    qsynedit
    redpanda_qt_utils)

if (QT_FEATURE_static)
    qt_import_plugins(test-file-search
        INCLUDE Qt::QOffscreenIntegrationPlugin)
endif()

add_test(
    NAME test-file-search
    COMMAND test-file-search)
set_tests_properties(test-file-search PROPERTIES
    ENVIRONMENT "${test_environment}")
add_dependencies(all-test-targets test-file-search)

#####################
# test-refactor     #
#####################

add_executable(test-refactor test/test-refactor-main.cpp)

target_qt_plain_cpp(test-refactor
    src/settings
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
    src/parser/cppparser
    src/parser/cpppreprocessor
    src/parser/pptoken
    src/parser/ppexpander
    src/parser/cpptokenizer
    src/parser/cpptokencursor
    src/parser/cpptypes
    src/parser/cppdeclarationparser
    src/parser/cppsyntaxparser
    src/parser/cppstatementbuilder
    src/parser/cpptyperesolver
    src/parser/typeoperationregistry
    src/parser/cppexpressionevaluator
    src/parser/cppscopes
    src/parser/parserutils
    src/parser/statementmodel
    src/editor/cppexpressionextractor
    src/refactor/cpprefactor
    src/utils
    src/utils/escape
    src/utils/file
    src/utils/font
    src/utils/os
    src/utils/parsearg
    src/utils/parsemacros
    src/utils/pe
    src/utils/ui
    src/compiler/runner
    src/compiler/executablerunner
    src/projectoptions
    src/colorscheme
    src/customfileiconprovider
    src/syntaxermanager
    src/iconsmanager
    src/systemconsts
    src/compiler/compilerinfo
    src/reformatter/basereformatter
    test/test_cpprefactor
    test/test_refactor_stubs)

target_moc_classes(test-refactor
    src/editor/editor
    src/project
    src/projecttemplate
    src/settingsdialog/settingswidget
    src/widgets/bookmarkmodel
    src/widgets/classbrowser
    src/widgets/codecompletionlistview
    src/widgets/codecompletionpopup
    src/widgets/coloredit
    src/widgets/customdisablediconengine
    src/widgets/qpatchedcombobox
    src/widgets/functiontooltipwidget
    src/widgets/headercompletionpopup
    src/widgets/searchresultview
    src/symbolusagemanager
    src/codesnippetsmanager
    test/test_cpprefactor)

target_include_directories(test-refactor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(test-refactor PRIVATE
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
    qt_import_plugins(test-refactor
        INCLUDE Qt::QOffscreenIntegrationPlugin)
endif()

target_compile_definitions(test-refactor PRIVATE
    ${GLOBAL_COMPILE_DEFINITIONS}
    APP_NAME=\"test-refactor\"
    LIBEXECDIR=\"${CMAKE_INSTALL_LIBEXECDIR}\"
    REDPANDA_CPP_VERSION=\"${REDPANDA_VERSION}\"
    ARCH_X86_64=1
    ARCH_X86=1)

add_test(
    NAME test-refactor
    COMMAND test-refactor)
set_tests_properties(test-refactor PROPERTIES
    ENVIRONMENT "${test_environment}")
add_dependencies(all-test-targets test-refactor)

#####################
# test-cppparser    #
#####################

add_executable(test-cppparser test/test-cppparser-main.cpp)

target_qt_plain_cpp(test-cppparser
    src/parser/cppparser
    src/parser/cpppreprocessor
    src/parser/pptoken
    src/parser/ppexpander
    src/parser/cpptokenizer
    src/parser/cpptokencursor
    src/parser/cpptypes
    src/parser/cppdeclarationparser
    src/parser/cppsyntaxparser
    src/parser/cppstatementbuilder
    src/parser/cpptyperesolver
    src/parser/typeoperationregistry
    src/parser/cppexpressionevaluator
    src/parser/cppscopes
    src/parser/parserutils
    src/parser/statementmodel
    test/test_cppparsercomponenttestcase
    test/test_cppparsertestcase
    )

target_moc_classes(test-cppparser
    test/test_cppparser
    test/test_cppparserinvalidation
    test/test_cppparsertyntax
    test/test_cppparserstandardlibrary
    test/test_cpppreprocessor
    test/test_cpppreprocessor_include
    test/test_cpptokenizer
    test/test_cppdeclarationparser
    test/test_cppstatementbuilder
    test/test_cpptyperesolver
    test/test_cppexpressionevaluator
    test/test_typeoperationregistry
    test/test_pptoken
    test/test_ppexpander
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

target_compile_definitions(test-cppparser PRIVATE
    ${GLOBAL_COMPILE_DEFINITIONS}
    APP_NAME=\"test-cppparser\"
    LIBEXECDIR=\"${CMAKE_INSTALL_LIBEXECDIR}\"
    REDPANDA_CPP_VERSION=\"${REDPANDA_VERSION}\"
    REDPANDA_PARSER_TEST
    ARCH_X86_64=1
    ARCH_X86=1)

option(REDPANDA_PARSER_DECLARATION_PROFILE
    "Enable declaration parser self-time profiling" OFF)
if(REDPANDA_PARSER_DECLARATION_PROFILE)
    add_compile_definitions(REDPANDA_PARSER_DECLARATION_PROFILE)
endif()

option(REDPANDA_PARSER_STDLIB_TEST
    "Enable standard library parser acceptance tests" OFF)
set(REDPANDA_PARSER_STDLIB_MINGW_ROOT "" CACHE PATH
    "MinGW root used by the standard library parser acceptance tests")
if(REDPANDA_PARSER_STDLIB_TEST)
    list(APPEND test_environment "LC_ALL=C")
    if(NOT EXISTS "${REDPANDA_PARSER_STDLIB_MINGW_ROOT}/bin/g++.exe")
        message(FATAL_ERROR
            "REDPANDA_PARSER_STDLIB_MINGW_ROOT must point to a MinGW installation")
    endif()
    target_compile_definitions(test-cppparser PRIVATE
        REDPANDA_PARSER_STDLIB_TEST
        REDPANDA_PARSER_STDLIB_MINGW_ROOT="${REDPANDA_PARSER_STDLIB_MINGW_ROOT}"
        REDPANDA_PARSER_STDLIB_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/resources/stdlib")
    target_compile_definitions(test-widgets PRIVATE
        REDPANDA_PARSER_STDLIB_TEST
        REDPANDA_PARSER_STDLIB_MINGW_ROOT="${REDPANDA_PARSER_STDLIB_MINGW_ROOT}"
        REDPANDA_PARSER_STDLIB_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/resources/stdlib")
    target_compile_definitions(test-editor PRIVATE
        REDPANDA_PARSER_STDLIB_TEST
        REDPANDA_PARSER_STDLIB_MINGW_ROOT="${REDPANDA_PARSER_STDLIB_MINGW_ROOT}"
        REDPANDA_PARSER_STDLIB_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/resources/stdlib")
endif()

add_test(
    NAME test-cppparser
    COMMAND test-cppparser)

set_tests_properties(
    test-editor
    test-widgets
    test-cppparser
    PROPERTIES
        ENVIRONMENT "${test_environment}")

add_dependencies(all-test-targets test-cppparser)
add_dependencies(test-editor test-resources)
add_dependencies(test-widgets test-resources)
add_dependencies(test-cppparser test-resources)

#####################
# Platform-specific #
#####################

if(WIN32)
    target_ui_classes(RedPandaIDE
        src/settingsdialog/environmentfileassociationwidget
        src/settingsdialog/projectversioninfowidget)

    target_link_libraries(RedPandaIDE PRIVATE
        advapi32
        dbghelp
        psapi
        shlwapi
        user32)
    target_link_libraries(test-widgets PRIVATE dbghelp)
    target_link_libraries(test-refactor PRIVATE dbghelp)
endif()

if(MACOS)
    target_link_libraries(RedPandaIDE PRIVATE Qt::GuiPrivate)
endif()

if(${FILESYSTEM_LAYOUT} STREQUAL "bundle")
    set_target_properties(RedPandaIDE PROPERTIES
        MACOSX_BUNDLE ON)
    set(MACOSX_BUNDLE_ICON_FILE RedPandaIDE.icns)
    set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/platform/macos/RedPandaIDE.icns
        PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources)
    target_sources(RedPandaIDE PRIVATE
        ${CMAKE_SOURCE_DIR}/platform/macos/RedPandaIDE.icns)
endif()

if(LINUX AND QT_FEATURE_static)
    qt_import_plugins(RedPandaIDE
        INCLUDE
            Qt::QComposePlatformInputContextPlugin
            Qt::QFcitx5PlatformInputContextPlugin
            Qt::QIbusPlatformInputContextPlugin)
endif()

if(${FILESYSTEM_LAYOUT} STREQUAL "hierarchy")
    install(
        TARGETS RedPandaIDE
        DESTINATION ${CMAKE_INSTALL_BINDIR})
elseif(${FILESYSTEM_LAYOUT} STREQUAL "flat")
    install(
        TARGETS RedPandaIDE
        DESTINATION .)
elseif(${FILESYSTEM_LAYOUT} STREQUAL "bundle")
    install(
        TARGETS RedPandaIDE
        BUNDLE DESTINATION .)
else()
    message(FATAL_ERROR "Unknown FILESYSTEM_LAYOUT: ${FILESYSTEM_LAYOUT}")
endif()

#########
# Paths #
#########

set (MAIN_PROGRAM_PATH_DEFS
    FILESYSTEM_LAYOUT=FILESYSTEM_LAYOUT_${FILESYSTEM_LAYOUT}
    FILESYSTEM_LAYOUT_hierarchy=1
    FILESYSTEM_LAYOUT_flat=2
    FILESYSTEM_LAYOUT_bundle=3
    MAKE_INTERFACE=MAKE_INTERFACE_${MAKE_INTERFACE}
    MAKE_INTERFACE_unix=1
    MAKE_INTERFACE_mingw32=2
    PORTABLE_CONFIG=PORTABLE_CONFIG_${PORTABLE_CONFIG}
    PORTABLE_CONFIG_oui=1
    PORTABLE_CONFIG_non=2
    PORTABLE_CONFIG_registry=3)

target_compile_definitions(RedPandaIDE PRIVATE ${MAIN_PROGRAM_PATH_DEFS})
target_compile_definitions(test-editor PRIVATE ${MAIN_PROGRAM_PATH_DEFS})
target_compile_definitions(test-widgets PRIVATE ${MAIN_PROGRAM_PATH_DEFS})
target_compile_definitions(test-refactor PRIVATE ${MAIN_PROGRAM_PATH_DEFS})

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
        message(STATUS "Found Qt base translation files: ${qtbase_translation_files}")
        target_embed_qtbase_translations(RedPandaIDE
            QM_FILES ${qtbase_translation_files})
    else()
        message(WARNING "Qt base translation files not found")
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

    # version info shown in the exe's file properties (pure numeric version,
    # pre-release tags are not included); regenerated when version.json or
    # the git HEAD changes
    if(NOT APP_VERSION_BUILD)
        set(APP_VERSION_BUILD 0)
    endif()
    configure_file(resources/windows_version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/windows_version.rc @ONLY)
    target_sources(RedPandaIDE PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/windows_version.rc)
elseif(MACOS)
    set(app_icon ../platform/macos/RedPandaIDE.icns)
    set_source_files_properties(${app_icon}
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(RedPandaIDE PRIVATE
        ${app_icon})
endif()
