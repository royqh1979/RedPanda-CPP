target("redpanda-git-askpass")
    add_rules("qt.widgetapp")

    add_files("main.cpp")
    add_ui_classes("dialog")

    if is_xdg() then
        on_install(install_libexec)
    end
