target("redpanda-git-askpass")
    add_rules("qt.widgetapp")

    add_files("main.cpp")
    add_ui_classes("dialog")

    set_install_libexec()
