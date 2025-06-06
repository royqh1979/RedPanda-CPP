add_library(redpanda_qt_utils STATIC)

target_qt_plain_cpp(redpanda_qt_utils qt_utils/utils)

target_moc_classes(redpanda_qt_utils qt_utils/charsetinfo)

target_embed_translations(redpanda_qt_utils
    TS_FILES
        qt_utils_ru_RU.ts
        qt_utils_zh_CN.ts)

target_include_directories(redpanda_qt_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(redpanda_qt_utils PUBLIC ${GLOBAL_COMPILE_DEFINITIONS})

target_link_libraries(redpanda_qt_utils PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets)
