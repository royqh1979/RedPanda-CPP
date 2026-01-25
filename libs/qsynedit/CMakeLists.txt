#########
# Files #
#########

set(QSYNEDIT_QT_PLAIN_CPP
    qsynedit/codefolding
    qsynedit/constants
    qsynedit/keystrokes
    qsynedit/miscprocs
    qsynedit/painter
    qsynedit/types
    # exporter
    qsynedit/exporter/exporter
    qsynedit/exporter/htmlexporter
    qsynedit/exporter/qtsupportedhtmlexporter
    qsynedit/exporter/rtfexporter
    # formatter
    qsynedit/formatter/cppformatter
    qsynedit/formatter/formatter
    # syntaxer
    qsynedit/syntaxer/asm
    qsynedit/syntaxer/cpp
    qsynedit/syntaxer/gas
    qsynedit/syntaxer/glsl
    qsynedit/syntaxer/lua
    qsynedit/syntaxer/makefile
    qsynedit/syntaxer/nasm
    qsynedit/syntaxer/syntaxer
    qsynedit/syntaxer/textfile)

set(QSYNEDIT_MOC_CLASSES
    qsynedit/document
    qsynedit/gutter
    qsynedit/qsynedit
    # searcher
    qsynedit/searcher/baseseacher
    qsynedit/searcher/basicsearcher
    qsynedit/searcher/regexsearcher)

################
# Main library #
################

add_library(qsynedit STATIC)

target_qt_plain_cpp(qsynedit ${QSYNEDIT_QT_PLAIN_CPP})

target_moc_classes(qsynedit ${QSYNEDIT_MOC_CLASSES})

target_compile_definitions(qsynedit PUBLIC
    ${GLOBAL_COMPILE_DEFINITIONS}
    ARCH_X86_64=1
    ARCH_X86=1)

target_embed_translations(qsynedit
    TS_FILES
        qsynedit_ru_RU.ts
        qsynedit_zh_CN.ts)

target_include_directories(qsynedit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(qsynedit PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
    redpanda_qt_utils)

################
# Test library #
################

#add_library(qsynedit_test STATIC)

#target_qt_plain_cpp(qsynedit_test ${QSYNEDIT_QT_PLAIN_CPP})

#target_moc_classes(qsynedit_test ${QSYNEDIT_MOC_CLASSES})

#target_compile_definitions(qsynedit PUBLIC
#    ${GLOBAL_COMPILE_DEFINITIONS}
#    ARCH_X86_64=1
#    ARCH_X86=1)

#target_include_directories(qsynedit_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#target_link_libraries(qsynedit_test PUBLIC
#    Qt::Core
#    Qt::Gui
#    Qt::Widgets
#    redpanda_qt_utils)

#target_compile_definitions(qsynedit_test PUBLIC
#    QSYNEDIT_TEST
#    ARCH_X86_64=1
#    ARCH_X86=1)

#add_library(qsynedit::test ALIAS qsynedit_test)

################
# Test program #
################

add_executable(test-qsynedit test/main.cpp)

target_qt_plain_cpp(test-qsynedit
    ${QSYNEDIT_QT_PLAIN_CPP})

target_moc_classes(test-qsynedit
    ${QSYNEDIT_MOC_CLASSES}
    test/test_charpos
    test/test_cppsyntaxer
    test/test_document
    test/test_qsynedit_base
    test/test_qsynedit_cpp
    test/test_qsynedit_cpp_search_replace
    test/test_qsynedit_emoji
    test/test_utils)

target_include_directories(test-qsynedit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(test-qsynedit PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Test
    redpanda_qt_utils)

if (QT_FEATURE_static)
    qt_import_plugins(test-qsynedit
        INCLUDE Qt::QOffscreenIntegrationPlugin)
endif()

#add_custom_command(
#    TARGET test-qsynedit POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#        "$<TARGET_PROPERTY:test-qsynedit,SOURCE_DIR>/test/resources"
#        "$<TARGET_PROPERTY:test-qsynedit,BINARY_DIR>/resources")

add_custom_target(
    copy_test_qsynedit_resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/test/resources
    ${CMAKE_CURRENT_BINARY_DIR}/resources
    COMMENT "Copying test-qsynedit resources"
)

add_dependencies(test-qsynedit copy_test_qsynedit_resources)

target_compile_definitions(test-qsynedit PRIVATE
    ${GLOBAL_COMPILE_DEFINITIONS}
    QSYNEDIT_TEST
    ARCH_X86_64=1
    ARCH_X86=1)


if (CMAKE_HOST_LINUX AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(test-qsynedit PRIVATE "-fsanitize=undefined" "-fno-omit-frame-pointer")
    target_link_options(test-qsynedit PRIVATE "-fsanitize=undefined")
endif()

add_test(
    NAME test-qsynedit
    COMMAND test-qsynedit)

add_dependencies(all-test-targets test-qsynedit)
