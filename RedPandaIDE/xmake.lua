set_languages("cxx17")

target("RedPandaIDE_main_glibc_hwcaps")
    if has_config("glibc-hwcaps") then
        set_enabled(true)
    else
        set_enabled(false)
    end
    set_kind("binary")
    set_basename("RedPandaIDE")

    add_deps('RedPandaIDE')

    add_files("src/main_glibc_hwcaps.c")

target("RedPandaIDE")
    if has_config("glibc-hwcaps") then
        add_rules("qt.shared")
        set_symbols("hidden")
    else
        add_rules("qt.widgetapp")
    end
    add_rules("qt.ts")

    add_deps("redpanda_qt_utils", "qsynedit")
    add_frameworks(
        "QtGui",
        "QtNetwork",
        "QtPrintSupport",
        "QtSvg",
        "QtWidgets",
        "QtXml")
    add_includedirs(".")

    -- resolve version

    on_load(function (target)
        import("core.base.json")
        local version_json = io.readfile("version.json")
        local version = json.decode(version_json)

        local redpanda_version = format("%d.%d.%d", version.major, version.minor, version.patch)
        try {function()
            local outdata = os.iorunv("git", {"rev-list", "HEAD", "--count"})
            redpanda_version = redpanda_version .. "." .. outdata:trim()
        end}

        if version.preRelease ~= "" then
            redpanda_version = redpanda_version .. "-" .. version.preRelease
            target:add("defines", "APP_VERSION_SUFFIX=\"" .. version.preRelease .. "\"")
        end

        target:add("defines", "REDPANDA_CPP_VERSION=\"" .. redpanda_version .. "\"")
    end)

    -- defines

    add_options("app-name", "prefix", "libexecdir", "glibc-hwcaps")
    add_options("lua-addon", "sdcc", "vcs")

    add_defines("ARCH_X86_64=1")
    add_defines("ARCH_X86=1")

    -- files

    add_files(
        "src/autolinkmanager.cpp",
        "src/colorscheme.cpp",
        "src/customfileiconprovider.cpp",
        "src/projectoptions.cpp",
        "src/settings.cpp",
        "src/syntaxermanager.cpp",
        "src/systemconsts.cpp",
        "src/utils.cpp",
        "src/visithistorymanager.cpp",
        -- compiler
        "src/compiler/compilerinfo.cpp",
        -- debugger
        "src/debugger/dapprotocol.cpp",
        "src/debugger/gdbmiresultparser.cpp",
        -- parser
        "src/parser/cpppreprocessor.cpp",
        "src/parser/cpptokenizer.cpp",
        "src/parser/parserutils.cpp",
        -- problems
        "src/problems/freeprojectsetformat.cpp",
        "src/problems/problemcasevalidator.cpp",
        -- settings
        "src/settings/basesettings.cpp",
        "src/settings/codecompletionsettings.cpp",
        "src/settings/codeformattersettings.cpp",
        "src/settings/compilersetsettings.cpp",
        "src/settings/compilesettings.cpp",
        "src/settings/debuggersettings.cpp",
        "src/settings/dirsettings.cpp",
        "src/settings/editorsettings.cpp",
        "src/settings/environmentsettings.cpp",
        "src/settings/executorsettings.cpp",
        "src/settings/languagesettings.cpp",
        "src/settings/uisettings.cpp",
        -- utils
        "src/utils/escape.cpp",
        "src/utils/file.cpp",
        "src/utils/font.cpp",
        "src/utils/os.cpp",
        "src/utils/parsearg.cpp",
        "src/utils/parser.cpp",
        "src/utils/parsemacros.cpp",
        "src/utils/terminal.cpp",
        "src/utils/ui.cpp")

    add_moc_classes(
        "src/caretlist",
        "src/codesnippetsmanager",
        "src/cpprefacter",
        "src/editor",
        "src/editormanager",
        "src/iconsmanager",
        "src/main",
        "src/project",
        "src/projecttemplate",
        "src/shortcutmanager",
        "src/symbolusagemanager",
        "src/thememanager",
        "src/todoparser",
        "src/toolsmanager",
        -- compiler
        "src/compiler/compiler",
        "src/compiler/compilermanager",
        "src/compiler/executablerunner",
        "src/compiler/filecompiler",
        "src/compiler/nasmfilecompiler",
        "src/compiler/ojproblemcasesrunner",
        "src/compiler/projectcompiler",
        "src/compiler/runner",
        "src/compiler/stdincompiler",
        -- debugger
        "src/debugger/dapdebugger",
        "src/debugger/debugger",
        "src/debugger/debuggermodels",
        "src/debugger/gdbmidebugger",
        -- parser
        "src/parser/cppparser",
        "src/parser/statementmodel",
        -- problems
        "src/problems/competitivecompenionhandler",
        "src/problems/ojproblemset",
        -- reformatter
        "src/reformatter/astyleformatter",
        "src/reformatter/basereformatter",
        -- settings dialog
        "src/settingsdialog/settingswidget",
        -- widgets
        "src/widgets/bookmarkmodel",
        "src/widgets/classbrowser",
        "src/widgets/classbrowser",
        "src/widgets/codecompletionlistview",
        "src/widgets/codecompletionpopup",
        "src/widgets/coloredit",
        "src/widgets/compileargumentswidget",
        "src/widgets/customdisablediconengine",
        "src/widgets/customfilesystemmodel",
        "src/widgets/darkfusionstyle",
        "src/widgets/editorstabwidget",
        "src/widgets/filenameeditdelegate",
        "src/widgets/functiontooltipwidget",
        "src/widgets/headercompletionpopup",
        "src/widgets/issuestable",
        "src/widgets/labelwithmenu",
        "src/widgets/lightfusionstyle",
        "src/widgets/linenumbertexteditor",
        "src/widgets/macroinfomodel",
        "src/widgets/ojproblemsetmodel",
        "src/widgets/qconsole",
        "src/widgets/qpatchedcombobox",
        "src/widgets/searchresultview",
        "src/widgets/shortcutinputedit",
        "src/widgets/shrinkabletabwidget")

    add_ui_classes(
        "src/mainwindow",
        -- settings dialog
        "src/settingsdialog/compilerautolinkwidget",
        "src/settingsdialog/compilergaswidget",
        "src/settingsdialog/compilernasmwidget",
        "src/settingsdialog/compilersetdirectorieswidget",
        "src/settingsdialog/compilersetoptionwidget",
        "src/settingsdialog/debuggeneralwidget",
        "src/settingsdialog/editorautosavewidget",
        "src/settingsdialog/editorclipboardwidget",
        "src/settingsdialog/editorcodecompletionwidget",
        "src/settingsdialog/editorcolorschemewidget",
        "src/settingsdialog/editorcustomctypekeywords",
        "src/settingsdialog/editorfontwidget",
        "src/settingsdialog/editorgeneralwidget",
        "src/settingsdialog/editormiscwidget",
        "src/settingsdialog/editorsnippetwidget",
        "src/settingsdialog/editorsymbolcompletionwidget",
        "src/settingsdialog/editorsyntaxcheckwidget",
        "src/settingsdialog/editortooltipswidget",
        "src/settingsdialog/environmentappearancewidget",
        "src/settingsdialog/environmentfolderswidget",
        "src/settingsdialog/environmentperformancewidget",
        "src/settingsdialog/environmentprogramswidget",
        "src/settingsdialog/environmentshortcutwidget",
        "src/settingsdialog/executorgeneralwidget",
        "src/settingsdialog/executorproblemsetwidget",
        "src/settingsdialog/formattergeneralwidget",
        "src/settingsdialog/formatterpathwidget",
        "src/settingsdialog/languageasmgenerationwidget",
        "src/settingsdialog/projectcompileparamaterswidget",
        "src/settingsdialog/projectcompilerwidget",
        "src/settingsdialog/projectdirectorieswidget",
        "src/settingsdialog/projectdllhostwidget",
        "src/settingsdialog/projectfileswidget",
        "src/settingsdialog/projectgeneralwidget",
        "src/settingsdialog/projectmakefilewidget",
        "src/settingsdialog/projectoutputwidget",
        "src/settingsdialog/projectprecompilewidget",
        "src/settingsdialog/settingsdialog",
        "src/settingsdialog/toolsgeneralwidget",
        -- widgets
        "src/widgets/aboutdialog",
        "src/widgets/choosethemedialog",
        "src/widgets/cpudialog",
        "src/widgets/custommakefileinfodialog",
        "src/widgets/editorfontdialog",
        "src/widgets/filepropertiesdialog",
        "src/widgets/infomessagebox",
        "src/widgets/newclassdialog",
        "src/widgets/newheaderdialog",
        "src/widgets/newprojectdialog",
        "src/widgets/newprojectunitdialog",
        "src/widgets/newtemplatedialog",
        "src/widgets/ojproblempropertywidget",
        "src/widgets/projectalreadyopendialog",
        "src/widgets/searchdialog",
        "src/widgets/searchinfiledialog",
        "src/widgets/signalmessagedialog")

    add_files("*.qrc", "translations/*.ts")

    add_files(
        "resources/iconsets/**.svg", "resources/iconsets/**.json",
        "resources/themes/*.lua", "resources/themes/*.json", "resources/themes/*.png",
        "resources/colorschemes/*.scheme",
        "resources/fonts/asciicontrol.ttf",
        {rule = "RedPandaIDE.auto_qrc"})

    if is_os("windows") then
        add_ui_classes(
            "src/settingsdialog/environmentfileassociationwidget",
            "src/settingsdialog/projectversioninfowidget")
        add_files("resources/windows_icon.rc")
    end

    if has_config("lua-addon") then
        add_deps("lua")
        add_files(
            "src/addon/luaapi.cpp",
            "src/addon/luaexecutor.cpp",
            "src/addon/luaruntime.cpp")
        add_links("lua")
    end

    if has_config("sdcc") then
        add_moc_classes(
            "src/compiler/sdccfilecompiler",
            "src/compiler/sdccprojectcompiler")
    end

    if has_config("vcs") then
        add_moc_classes(
            "src/vcs/gitmanager",
            "src/vcs/gitrepository",
            "src/vcs/gitutils")
        add_ui_classes(
            "src/settingsdialog/toolsgitwidget",
            "src/vcs/gitbranchdialog",
            "src/vcs/gitfetchdialog",
            "src/vcs/gitlogdialog",
            "src/vcs/gitmergedialog",
            "src/vcs/gitpulldialog",
            "src/vcs/gitpushdialog",
            "src/vcs/gitremotedialog",
            "src/vcs/gitresetdialog",
            "src/vcs/gituserconfigdialog")
    end

    -- libs

    add_links("redpanda_qt_utils", "qsynedit")
    if is_os("windows") then
        add_links("psapi", "shlwapi")
    end

target("test-makefile-escape")
    set_kind("binary")
    add_rules("qt.console")

    add_tests("default")

    add_files(
        "src/utils/escape.cpp",
        "test/test-makefile-escape.cpp")
    add_includedirs(".")
