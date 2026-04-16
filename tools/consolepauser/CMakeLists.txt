add_executable(consolepauser)

target_compile_definitions(consolepauser PRIVATE ${GLOBAL_COMPILE_DEFINITIONS})

if(WIN32)
    target_sources(consolepauser PRIVATE main.windows.cpp)

    target_link_libraries(consolepauser PRIVATE psapi)

    if(MINGW)
        target_link_options(consolepauser PRIVATE -municode)
    endif()
endif()

if(UNIX)
    target_sources(consolepauser PRIVATE main.unix.cpp)
endif()

if(${FILESYSTEM_LAYOUT} STREQUAL "hierarchy")
    install(
        TARGETS consolepauser
        DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/${APP_NAME})
elseif(${FILESYSTEM_LAYOUT} STREQUAL "flat")
    install(
        TARGETS consolepauser
        DESTINATION .)
elseif(${FILESYSTEM_LAYOUT} STREQUAL "bundle")
    install(
        TARGETS consolepauser
        DESTINATION RedPandaIDE.app/Contents/MacOS)
else()
    message(FATAL_ERROR "Unknown FILESYSTEM_LAYOUT: ${FILESYSTEM_LAYOUT}")
endif()
