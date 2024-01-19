target("astyle")
    set_kind("binary")

    add_options("mingw-static")

    if is_os("windows") then
        add_links("shell32")
    end

    add_files(
        "ASBeautifier.cpp",
        "ASEnhancer.cpp",
        "ASFormatter.cpp",
        "ASLocalizer.cpp",
        "ASResource.cpp",
        "astyle_main.cpp")

    if is_xdg() then
        on_install(install_libexec)
    end
