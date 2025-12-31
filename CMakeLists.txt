cmake_minimum_required(VERSION 3.19)  # for string(JSON)
project(Red_Panda_CPP
    LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(CTest)
# a phony target that depends on all test targets
# usage:
#   make all-test-targets
#   make test
add_custom_target(all-test-targets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(FORCE_QT5 "Force Qt 5 when building with system Qt" OFF)
option(LUA_ADDON "Enable Lua add-ons" OFF)
option(SDCC "Enable SDCC compiler support" ON)
option(VCS "Enable Git integration" OFF)
option(WINDOWS_PREFER_OPENCONSOLE "Prefer OpenConsole.exe as terminal emulator" OFF)

set(LIBEXECDIR libexec CACHE STRING "Directory for auxiliary executables, RELATIVE TO prefix.")
set(APP_NAME RedPandaCPP CACHE STRING "Subdirectory name to $prefix/share and $prefix/libexec.")

set(QT_COMPONENTS
    Core
    Gui
    LinguistTools
    Network
    PrintSupport
    Svg
    Test
    Widgets
    Xml)

if (FORCE_QT5)
    set(QT_VERSION_MAJOR 5)
    find_package(Qt5 5.15 REQUIRED COMPONENTS ${QT_COMPONENTS})
else()
    find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})
    if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
        find_package(Qt6 6.8 REQUIRED COMPONENTS ${QT_COMPONENTS})
    else()
        find_package(Qt5 5.15 REQUIRED COMPONENTS ${QT_COMPONENTS})
    endif()
endif()

if (${QT_VERSION_MAJOR} LESS 6)
    # a phony target that depends on all lupdate targets (Qt 5)
    # the actual lupdate targets will be added in target_embed_translations function
    add_custom_target(update_translations)
endif()

# read version from "version.json"
file(READ version.json VERSION_JSON)
string(JSON APP_VERSION_MAJOR GET ${VERSION_JSON} major)
string(JSON APP_VERSION_MINOR GET ${VERSION_JSON} minor)
string(JSON APP_VERSION_PATCH GET ${VERSION_JSON} patch)
string(JSON APP_VERSION_PRE_RELEASE GET ${VERSION_JSON} preRelease)

# try git commit count for version sorting
execute_process(
    COMMAND git rev-list HEAD --count
    OUTPUT_VARIABLE APP_VERSION_BUILD
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if (APP_VERSION_BUILD)
    set(REDPANDA_VERSION "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}.${APP_VERSION_BUILD}")
else()
    set(REDPANDA_VERSION "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")
endif()

if (APP_VERSION_PRE_RELEASE)
    set(REDPANDA_VERSION "${REDPANDA_VERSION}-${APP_VERSION_PRE_RELEASE}")
endif()

if (UNIX AND NOT APPLE)
    set(XDG ON)
else()
    set(XDG OFF)
endif()

function(target_qt_plain_cpp target)
    foreach (arg IN LISTS ARGN)
        target_sources(${target} PRIVATE ${arg}.cpp)
        # for Qt 5 lupdate
        list(APPEND ${target}_LUPDATE_SOURCES ${arg}.cpp)
        set(${target}_LUPDATE_SOURCES ${${target}_LUPDATE_SOURCES} PARENT_SCOPE)
    endforeach()
endfunction()

function(target_moc_classes target)
    foreach(arg IN LISTS ARGN)
        target_sources(${target} PRIVATE ${arg}.cpp ${arg}.h)
        # for Qt 5 lupdate
        list(APPEND ${target}_LUPDATE_SOURCES ${arg}.cpp)
        set(${target}_LUPDATE_SOURCES ${${target}_LUPDATE_SOURCES} PARENT_SCOPE)
    endforeach()
endfunction()

function(target_ui_classes target)
    foreach(arg IN LISTS ARGN)
        target_sources(${target} PRIVATE ${arg}.cpp ${arg}.h ${arg}.ui)
        # for Qt 5 lupdate
        list(APPEND ${target}_LUPDATE_SOURCES ${arg}.cpp ${arg}.ui)
        set(${target}_LUPDATE_SOURCES ${${target}_LUPDATE_SOURCES} PARENT_SCOPE)
    endforeach()
endfunction()

function(target_embed_translations target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" TS_FILES)

    set(qrc_dir ${CMAKE_CURRENT_BINARY_DIR}/.qrc)
    file(MAKE_DIRECTORY ${qrc_dir})

    set(qrc_file ${qrc_dir}/${target}_qmake_qmake_qm_files.qrc)

    set(qm_files)
    if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
        qt_add_translations(${target}
            SOURCE_TARGETS ${target}
            QM_FILES_OUTPUT_VARIABLE qm_files
            TS_FILES ${arg_TS_FILES})
    else()
        qt_add_translation(qm_files ${arg_TS_FILES})

        # the lupdate target
        add_custom_target(${target}_lupdate)
        add_dependencies(update_translations ${target}_lupdate)

        # source list file
        set(lupdate_dir ${CMAKE_CURRENT_BINARY_DIR}/.lupdate)
        set(lupdate_lst_file ${lupdate_dir}/${target}_lupdate.lst)
        file(WRITE ${lupdate_lst_file})
        foreach (source_file IN LISTS ${target}_LUPDATE_SOURCES)
            file(APPEND ${lupdate_lst_file} "${source_file}\n")
        endforeach()

        # lupdate commands
        foreach (ts_file IN LISTS arg_TS_FILES)
            add_custom_command(
                TARGET ${target}_lupdate POST_BUILD
                COMMAND ${Qt5_LUPDATE_EXECUTABLE}
                ARGS @${lupdate_lst_file} -ts ${ts_file}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endforeach()
    endif()

    file(WRITE ${qrc_file} "<RCC><qresource prefix=\"/i18n\">")

    foreach(qm_file IN LISTS qm_files)
        get_filename_component(qm_filename ${qm_file} NAME)
        file(
            APPEND ${qrc_file}
                "<file alias=\"${qm_filename}\">${qm_file}</file>")
    endforeach()

    file(APPEND ${qrc_file} "</qresource></RCC>")

    target_sources(${target} PRIVATE ${qrc_file})
endfunction()

function(target_embed_resources target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" RESOURCE_NAME FILES)

    set(qrc_dir ${CMAKE_CURRENT_BINARY_DIR}/.qrc)
    file(MAKE_DIRECTORY ${qrc_dir})

    set(qrc_file ${qrc_dir}/${arg_RESOURCE_NAME}.qrc)

    file(WRITE ${qrc_file} "<RCC><qresource prefix=\"/\">")

    foreach(res_file IN LISTS arg_FILES)
        file(RELATIVE_PATH res_filename ${CMAKE_CURRENT_SOURCE_DIR} ${res_file})
        file(
            APPEND ${qrc_file}
                "<file alias=\"${res_filename}\">${res_file}</file>")
    endforeach()

    file(APPEND ${qrc_file} "</qresource></RCC>")

    target_sources(${target} PRIVATE ${qrc_file})
endfunction()

function(target_embed_qtbase_translations target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" QM_FILES)

    set(qrc_dir ${CMAKE_CURRENT_BINARY_DIR}/.qrc)
    file(MAKE_DIRECTORY ${qrc_dir})

    set(qrc_file ${qrc_dir}/qtbase_translation.qrc)

    file(WRITE ${qrc_file} "<RCC><qresource prefix=\"translations\">")

    foreach(qm_file IN LISTS arg_QM_FILES)
        get_filename_component(qm_filename ${qm_file} NAME)
        file(
            APPEND ${qrc_file}
                "<file alias=\"${qm_filename}\">${qm_file}</file>")
    endforeach()

    file(APPEND ${qrc_file} "</qresource></RCC>")

    target_sources(${target} PRIVATE ${qrc_file})
endfunction()

set(GLOBAL_COMPILE_DEFINITIONS)

if (WIN32)
    list(
        APPEND GLOBAL_COMPILE_DEFINITIONS
            _WIN32_WINNT=0x0501
            WIN32_LEAN_AND_MEAN)
    if (MSVC)
        list(APPEND GLOBAL_COMPILE_DEFINITIONS NOMINMAX)
    endif()
endif()

add_subdirectory(RedPandaIDE)
if (LUA_ADDON)
    add_subdirectory(libs/lua)
endif()
add_subdirectory(libs/qsynedit)
add_subdirectory(libs/redpanda_qt_utils)
add_subdirectory(tools/consolepauser)

if (VCS)
    if (WIN32)
        add_subdirectory(tools/redpanda-win-git-askpass)
    else()
        add_subdirectory(tools/redpanda-git-askpass)
    endif()
endif()

#############
# Resources #
#############

# templates
if (XDG)
    install(
        DIRECTORY platform/linux/templates
        DESTINATION share/${APP_NAME}
        FILES_MATCHING PATTERN "*")
elseif(WIN32)
    install(
        DIRECTORY platform/windows/templates/
        DESTINATION templates
        FILES_MATCHING PATTERN "*")
    install(
        DIRECTORY platform/windows/templates-win64/
        DESTINATION templates
        FILES_MATCHING PATTERN "*")
endif()

# docs
if (XDG)
    install(
        FILES README.md NEWS.md LICENSE
        DESTINATION share/doc/${APP_NAME})
else()
    install(
        FILES README.md NEWS.md LICENSE
        DESTINATION .)
endif()

# icon
if (XDG)
    install(
        FILES platform/linux/redpandaide.svg
        DESTINATION share/icons/hicolor/scalable/apps)
endif()

# desktop entry
IF(XDG)
    configure_file(
        platform/linux/RedPandaIDE.desktop.in
        RedPandaIDE.desktop)
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/RedPandaIDE.desktop
        DESTINATION share/applications)
endif()

# mime type
if(XDG)
    install(
        FILES platform/linux/redpandaide.xml
        DESTINATION share/mime/packages)
endif()

# qt.conf
if(WIN32)
    install(
        FILES platform/windows/qt.conf
        DESTINATION .)
endif()

# devcpp.ico
if(WIN32)
    install(
        FILES RedPandaIDE/images/devcpp.ico
        DESTINATION .)
endif()
