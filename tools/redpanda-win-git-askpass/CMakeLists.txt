add_executable(redpanda-win-git-askpass WIN32 main.cpp)

target_compile_definitions(redpanda-win-git-askpass PRIVATE
    ${GLOBAL_COMPILE_DEFINITIONS}
    UNICODE)

if(MSVC)
    target_link_libraries(redpanda-win-git-askpass PRIVATE
        advapi32.lib
        comdlg32.lib
        gdi32.lib
        shell32.lib
        user32.lib
        ws2_32.lib)
endif()

install(TARGETS redpanda-win-git-askpass
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
