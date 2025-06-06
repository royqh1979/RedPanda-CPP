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

if(XDG)
    install(
        TARGETS consolepauser
        DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/${APP_NAME})
else()
    install(
        TARGETS consolepauser
        DESTINATION .)
endif()
