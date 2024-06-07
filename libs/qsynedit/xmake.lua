target("qsynedit")
    add_rules("qt.static")
    add_rules("qt.ts")
    add_frameworks("QtGui", "QtWidgets")
    add_deps("redpanda_qt_utils")

    add_files(
        "qsynedit/codefolding.cpp",
        "qsynedit/constants.cpp",
        "qsynedit/keystrokes.cpp",
        "qsynedit/miscprocs.cpp",
        "qsynedit/painter.cpp",
        "qsynedit/types.cpp",
        -- exporter
        "qsynedit/exporter/exporter.cpp",
        "qsynedit/exporter/htmlexporter.cpp",
        "qsynedit/exporter/qtsupportedhtmlexporter.cpp",
        "qsynedit/exporter/rtfexporter.cpp",
        -- formatter
        "qsynedit/formatter/cppformatter.cpp",
        "qsynedit/formatter/formatter.cpp",
        -- syntaxer
        "qsynedit/syntaxer/asm.cpp",
        "qsynedit/syntaxer/cpp.cpp",
        "qsynedit/syntaxer/glsl.cpp",
        "qsynedit/syntaxer/lua.cpp",
        "qsynedit/syntaxer/makefile.cpp",
        "qsynedit/syntaxer/textfile.cpp",
        "qsynedit/syntaxer/syntaxer.cpp")

    add_moc_classes(
        "qsynedit/document",
        "qsynedit/gutter",
        "qsynedit/qsynedit",
        -- searcher
        "qsynedit/searcher/baseseacher",
        "qsynedit/searcher/basicsearcher",
        "qsynedit/searcher/regexsearcher")

    add_ui_classes()

    add_files("*.ts")

    add_includedirs(".", {interface = true})

    -- do not install
    on_install(function (target) end)
