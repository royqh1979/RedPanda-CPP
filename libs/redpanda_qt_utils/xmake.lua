target("redpanda_qt_utils")
    add_rules("qt.static")
    add_rules("qt.ts")
    add_frameworks("QtGui", "QtWidgets")

    add_files("qt_utils/utils.cpp")
    add_moc_classes("qt_utils/charsetinfo")
    add_files("*.ts")
    add_includedirs(".", {interface = true})

    -- do not install
    on_install(function (target) end)
