target("consolepauser")
    set_kind("binary")

    add_options("mingw-static")

    if is_os("windows") then
        add_files("main.windows.cpp")
    else
        add_files("main.unix.cpp")
    end

    if is_os("windows") then
        add_syslinks("psapi")
    elseif is_os("linux") then
        add_syslinks("rt")
    end

    if is_xdg() then
        on_install(install_libexec)
    end
