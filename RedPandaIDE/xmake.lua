set_languages("cxx17")

target("RedPandaIDE")
    add_rules("qt.widgetapp", "qt.ts")

    add_deps("redpanda_qt_utils", "qsynedit")
    add_frameworks("QtNetwork", "QtPrintSupport", "QtSvg", "QtXml")
    add_includedirs(".")

    -- defines

    add_options("app-name", "prefix", "libexecdir")
    add_options("lua-addon", "sdcc", "vcs")

    if APP_VERSION_SUFFIX ~= "" then
        add_defines('APP_VERSION_SUFFIX="' .. APP_VERSION_SUFFIX .. '"')
    end

    if TEST_VERSION ~= "" then
        add_defines('REDPANDA_CPP_VERSION="' .. APP_VERSION .. '.' .. TEST_VERSION .. '"')
    else
        add_defines('REDPANDA_CPP_VERSION="' .. APP_VERSION .. '"')
    end

    if is_arch("x86_64") then
        add_defines("ARCH_X86_64=1")
    elseif is_arch("i[3456]86") then
        add_defines("ARCH_X86=1")
    end

    -- files

    add_files(
        "autolinkmanager.cpp",
        "colorscheme.cpp",
        "customfileiconprovider.cpp",
        "main.cpp",
        "projectoptions.cpp",
        "settings.cpp",
        "syntaxermanager.cpp",
        "systemconsts.cpp",
        "utils.cpp",
        "visithistorymanager.cpp",
        -- compiler
        "compiler/compilerinfo.cpp",
        -- debugger
        "debugger/dapprotocol.cpp",
        "debugger/gdbmiresultparser.cpp",
        -- parser
        "parser/cpppreprocessor.cpp",
        "parser/cpptokenizer.cpp",
        "parser/parserutils.cpp",
        -- problems
        "problems/freeprojectsetformat.cpp",
        "problems/ojproblemset.cpp",
        "problems/problemcasevalidator.cpp",
        "utils/escape.cpp",
        "utils/font.cpp",
        "utils/parsearg.cpp")

    add_moc_classes(
        "caretlist",
        "codesnippetsmanager",
        "cpprefacter",
        "editor",
        "editorlist",
        "iconsmanager",
        "project",
        "projecttemplate",
        "shortcutmanager",
        "symbolusagemanager",
        "thememanager",
        "todoparser",
        "toolsmanager",
        -- compiler
        "compiler/compiler",
        "compiler/compilermanager",
        "compiler/executablerunner",
        "compiler/filecompiler",
        "compiler/ojproblemcasesrunner",
        "compiler/projectcompiler",
        "compiler/runner",
        "compiler/stdincompiler",
        -- debugger
        "debugger/dapdebugger",
        "debugger/debugger",
        "debugger/gdbmidebugger",
        -- parser
        "parser/cppparser",
        "parser/statementmodel",
        -- problems
        "problems/competitivecompenionhandler",
        -- settings dialog
        "settingsdialog/settingswidget",
        -- widgets
        "widgets/bookmarkmodel",
        "widgets/classbrowser",
        "widgets/classbrowser",
        "widgets/codecompletionlistview",
        "widgets/codecompletionpopup",
        "widgets/coloredit",
        "widgets/compileargumentswidget",
        "widgets/customdisablediconengine",
        "widgets/customfilesystemmodel",
        "widgets/darkfusionstyle",
        "widgets/editorstabwidget",
        "widgets/filenameeditdelegate",
        "widgets/functiontooltipwidget",
        "widgets/headercompletionpopup",
        "widgets/issuestable",
        "widgets/labelwithmenu",
        "widgets/lightfusionstyle",
        "widgets/linenumbertexteditor",
        "widgets/macroinfomodel",
        "widgets/ojproblemsetmodel",
        "widgets/qconsole",
        "widgets/qpatchedcombobox",
        "widgets/searchresultview",
        "widgets/shortcutinputedit",
        "widgets/shrinkabletabwidget")

    add_ui_classes(
        "mainwindow",
        -- settings dialog
        "settingsdialog/compilerautolinkwidget",
        "settingsdialog/compilersetdirectorieswidget",
        "settingsdialog/compilersetoptionwidget",
        "settingsdialog/debuggeneralwidget",
        "settingsdialog/editorautosavewidget",
        "settingsdialog/editorclipboardwidget",
        "settingsdialog/editorcodecompletionwidget",
        "settingsdialog/editorcolorschemewidget",
        "settingsdialog/editorcustomctypekeywords",
        "settingsdialog/editorfontwidget",
        "settingsdialog/editorgeneralwidget",
        "settingsdialog/editormiscwidget",
        "settingsdialog/editorsnippetwidget",
        "settingsdialog/editorsymbolcompletionwidget",
        "settingsdialog/editorsyntaxcheckwidget",
        "settingsdialog/editortooltipswidget",
        "settingsdialog/environmentappearancewidget",
        "settingsdialog/environmentfolderswidget",
        "settingsdialog/environmentperformancewidget",
        "settingsdialog/environmentprogramswidget",
        "settingsdialog/environmentshortcutwidget",
        "settingsdialog/executorgeneralwidget",
        "settingsdialog/executorproblemsetwidget",
        "settingsdialog/formattergeneralwidget",
        "settingsdialog/formatterpathwidget",
        "settingsdialog/languageasmgenerationwidget",
        "settingsdialog/projectcompileparamaterswidget",
        "settingsdialog/projectcompilerwidget",
        "settingsdialog/projectdirectorieswidget",
        "settingsdialog/projectdllhostwidget",
        "settingsdialog/projectfileswidget",
        "settingsdialog/projectgeneralwidget",
        "settingsdialog/projectmakefilewidget",
        "settingsdialog/projectoutputwidget",
        "settingsdialog/projectprecompilewidget",
        "settingsdialog/settingsdialog",
        "settingsdialog/toolsgeneralwidget",
        -- widgets
        "widgets/aboutdialog",
        "widgets/choosethemedialog",
        "widgets/cpudialog",
        "widgets/custommakefileinfodialog",
        "widgets/editorfontdialog",
        "widgets/filepropertiesdialog",
        "widgets/infomessagebox",
        "widgets/newclassdialog",
        "widgets/newheaderdialog",
        "widgets/newprojectdialog",
        "widgets/newprojectunitdialog",
        "widgets/newtemplatedialog",
        "widgets/ojproblempropertywidget",
        "widgets/projectalreadyopendialog",
        "widgets/searchdialog",
        "widgets/searchinfiledialog",
        "widgets/signalmessagedialog")

    add_files("*.qrc", "translations/*.ts")

    add_files(
        "resources/iconsets/**.svg", "resources/iconsets/**.json",
        "resources/themes/*.lua", "resources/themes/*.json", "resources/themes/*.png",
        "resources/colorschemes/*.scheme",
        "resources/fonts/asciicontrol.ttf",
        {rule = "RedPandaIDE.auto_qrc"})

    if is_os("windows") then
        add_ui_classes(
            "settingsdialog/environmentfileassociationwidget",
            "settingsdialog/projectversioninfowidget")
    end

    if has_config("lua-addon") then
        add_deps("lua")
        add_files(
            "addon/api.cpp",
            "addon/executor.cpp",
            "addon/runtime.cpp")
        add_links("lua")
    end

    if has_config("sdcc") then
        add_moc_classes(
            "compiler/sdccfilecompiler",
            "compiler/sdccprojectcompiler")
    end

    if has_config("vcs") then
        add_moc_classes(
            "vcs/gitmanager",
            "vcs/gitrepository",
            "vcs/gitutils")
        add_ui_classes(
            "settingsdialog/toolsgitwidget",
            "vcs/gitbranchdialog",
            "vcs/gitfetchdialog",
            "vcs/gitlogdialog",
            "vcs/gitmergedialog",
            "vcs/gitpulldialog",
            "vcs/gitpushdialog",
            "vcs/gitremotedialog",
            "vcs/gitresetdialog",
            "vcs/gituserconfigdialog")
    end

    -- libs

    add_links("redpanda_qt_utils", "qsynedit")
    if is_os("windows") then
        add_links("psapi", "shlwapi")
    end

    -- install

    if is_xdg() then
        on_install(install_bin)
    end

target("test-escape")
    set_kind("binary")
    add_rules("qt.console")

    set_default(false)
    add_tests("test-escape")

    add_files("utils/escape.cpp", "test/escape.cpp")
    add_includedirs(".")
