includes("./version.lua")

TEST_VERSION = "$(shell git rev-list HEAD --count)"

add_rules("mode.debug", "mode.release")
set_warnings("all", "extra", "pedantic")
set_languages("cxx17", "c11")
set_encodings("utf-8")

function is_xdg()
    return is_os("linux", "bsd")
end

if is_os("windows") then
    add_defines("NOMINMAX")
    add_defines("_WIN32_WINNT=0x0501")
end

option("app-name")
    set_default("RedPandaCPP")
    set_showmenu(true)
    add_defines("APP_NAME=\"$(app-name)\"")

option("prefix")
    if is_xdg() then
        set_showmenu(true)
        set_default("/usr/local")
    else
        set_showmenu(false)
        set_default("")
    end
    add_defines('PREFIX="$(prefix)"')

option("libexecdir")
    add_deps("prefix")
    if is_xdg() then
        set_default("$prefix/libexec")  -- dummy
        set_showmenu(true)
        before_check(function (option)
            option:set_value(option:dep("prefix"):value() .. "/libexec")
        end)
    else
        set_default("")
        set_showmenu(false)
    end
    add_defines('LIBEXECDIR="$(libexecdir)"')

-- feature flags

option("lua-addon")
    set_default(true)
    set_showmenu(true)
    add_defines("ENABLE_LUA_ADDON")

option("sdcc")
    set_default(true)
    set_showmenu(true)
    add_defines("ENABLE_SDCC")

option("vcs")
    set_default(false)
    set_showmenu(true)
    add_defines("ENABLE_VCS")

option_end()

rule("qt.ts")
    add_deps("qt.env", "qt.qrc")
    set_extensions(".ts")
    on_config(function (target)
        -- get lrelease
        local qt = assert(target:data("qt"), "Qt not found!")
        local lrelease = path.join(qt.bindir, is_host("windows") and "lrelease.exe" or "lrelease")
        assert(os.isexec(lrelease), "lrelease not found!")
        -- save lrelease
        target:data_set("qt.lrelease", lrelease)
    end)
    on_buildcmd_files(function (target, batchcmds, sourcebatch, opt)
        local lrelease = target:data("qt.lrelease")
        local qrc_content = [[
            <RCC>
                <qresource prefix="/i18n">
        ]]
        for _, sourcefile_ts in ipairs(sourcebatch.sourcefiles) do
            if is_host("windows") then
                sourcefile_ts = sourcefile_ts:gsub("\\", "/")
            end
            -- get qm file
            local sourcefile_qm = path.join(target:autogendir(), "rules", "qt", "ts", path.basename(sourcefile_ts) .. ".qm")
            local sourcefile_dir = path.directory(sourcefile_qm)
            -- build ts to qm file
            batchcmds:show_progress(opt.progress, "${color.build.object}compiling.qt.ts %s", sourcefile_ts)
            batchcmds:mkdir(sourcefile_dir)
            batchcmds:vrunv(lrelease, {sourcefile_ts, "-qm", sourcefile_qm})

            qrc_content = qrc_content .. [[
                   <file alias="]] .. path.filename(sourcefile_qm) .. [[">]] .. path.absolute(sourcefile_qm) .. [[</file>
            ]]
        end
        qrc_content = qrc_content .. [[
                </qresource>
            </RCC>
        ]]

        local rcc = target:data("qt.rcc")
        local name = target:name() .. "_qmake_qmake_qm_files"  -- same as qmake
        local sourcefile_qrc = path.join(target:autogendir(), "rules", "qt", "ts", name .. ".qrc")
        io.writefile(sourcefile_qrc, qrc_content)
        -- get c++ source file for qrc
        local sourcefile_cpp = path.join(target:autogendir(), "rules", "qt", "ts", name .. ".cpp")
        -- add objectfile
        local objectfile = target:objectfile(sourcefile_cpp)
        table.insert(target:objectfiles(), objectfile)
        -- add commands
        batchcmds:show_progress(opt.progress, "${color.build.object}compiling.qt.ts %s", sourcefile_cpp)
        batchcmds:vrunv(rcc, {"-name", name, sourcefile_qrc, "-o", sourcefile_cpp})
        batchcmds:compile(sourcefile_cpp, objectfile)
        -- add deps
        batchcmds:add_depfiles(sourcebatch.sourcefiles)
        batchcmds:set_depmtime(os.mtime(objectfile))
        batchcmds:set_depcache(target:dependfile(objectfile))
    end)

rule("RedPandaIDE.auto_qrc")
    add_deps("qt.env", "qt.qrc")
    on_buildcmd_files(function (target, batchcmds, sourcebatch, opt)
        local name = 'RedPandaIDE_auto_qrc'
        -- prepare qrc file
        local qrc_content = [[
            <RCC>
                <qresource prefix="/">
        ]]
        for _, sourcefile in ipairs(sourcebatch.sourcefiles) do
            if is_host("windows") then
                sourcefile = sourcefile:gsub("\\", "/")
            end
            qrc_content = qrc_content .. [[
                   <file alias="]] .. string.gsub(sourcefile, "^RedPandaIDE/", "") .. [[">]] .. path.absolute(sourcefile) .. [[</file>
            ]]
        end
        qrc_content = qrc_content .. [[
                </qresource>
            </RCC>
        ]]
        local sourcefile_qrc = path.join(target:autogendir(), "rules", "qt", "auto_qrc", name .. ".qrc")
        io.writefile(sourcefile_qrc, qrc_content)
        -- get rcc
        local rcc = target:data("qt.rcc")
        -- get c++ source file for qrc
        local sourcefile_cpp = path.join(target:autogendir(), "rules", "qt", "auto_qrc", name .. ".cpp")
        -- add objectfile
        local objectfile = target:objectfile(sourcefile_cpp)
        table.insert(target:objectfiles(), objectfile)
        -- add commands
        batchcmds:show_progress(opt.progress, "${color.build.object}compiling.RedPandaIDE.auto_qrc")
        batchcmds:vrunv(rcc, {"-name", name, sourcefile_qrc, "-o", sourcefile_cpp})
        batchcmds:compile(sourcefile_cpp, objectfile)
        -- add deps
        batchcmds:add_depfiles(sourcebatch.sourcefiles)
        batchcmds:set_depmtime(os.mtime(objectfile))
        batchcmds:set_depcache(target:dependfile(objectfile))
    end)

rule_end()

function add_moc_classes(...)
    local classes = {...}
    for _, class in ipairs(classes) do
        add_files(
            class .. ".cpp",
            class .. ".h")
    end
end

function add_ui_classes(...)
    local classes = {...}
    for _, class in ipairs(classes) do
        add_files(
            class .. ".cpp",
            class .. ".h",
            class .. ".ui")
    end
end

-- imitate `make DESTDIR=... install` on XDG platforms

function install_libexec(target)
    local installdir = target:installdir() .. "/$(libexecdir)/$(app-name)"
    print("installing", target:name(), "to", installdir, "..")
    os.cp(target:targetfile(), installdir .. "/" .. target:filename())
end

function install_bin(target)
    local installdir = target:installdir() .. "/$(prefix)/bin"
    print("installing", target:name(), "to", installdir, "..")
    os.cp(target:targetfile(), installdir .. "/" .. target:filename())
end

includes("RedPandaIDE")
if has_config("lua-addon") then
    includes("libs/lua")
end
includes("libs/qsynedit")
includes("libs/redpanda_qt_utils")
includes("tools/consolepauser")
if has_config("vcs") then
    if is_os("windows") then
        includes("tools/redpanda-win-git-askpass")
    else
        includes("tools/redpanda-git-askpass")
    end
end

target("resources")
    set_kind("phony")

    -- templates

    if is_xdg() then
        add_installfiles("platform/linux/templates/(**.*)", {prefixdir = "$(prefix)/share/$(app-name)/templates"})
    elseif is_os("windows") then
        add_installfiles("platform/windows/templates/(**.*)", {prefixdir = "bin/templates"})
        if is_arch("x86_64") then
            add_installfiles("platform/windows/templates-win64/(**.*)", {prefixdir = "bin/templates"})
        end
    end

    -- docs

    if is_xdg() then
        add_installfiles("README.md", "NEWS.md", "LICENSE", {prefixdir = "$(prefix)/share/doc/$(app-name)"})
    else
        add_installfiles("README.md", "NEWS.md", "LICENSE", {prefixdir = "bin"})
    end

    -- icon

    if is_xdg() then
        add_installfiles("platform/linux/redpandaide.svg", {prefixdir = "$(prefix)/share/icons/hicolor/scalable/apps"})
    end

    -- desktop entry

    if is_xdg() then
        add_configfiles("platform/linux/RedPandaIDE.desktop.in", {
            pattern = "$${(.-)}",
            variables = {
                PREFIX = get_config("prefix"),
            },
        })
        add_installfiles("$(buildir)/RedPandaIDE.desktop", {prefixdir = "$(prefix)/share/applications"})
    end

    -- mime type

    if is_xdg() then
        add_installfiles("platform/linux/redpandaide.xml", {prefixdir = "$(prefix)/share/mime/packages"})
    end

    -- qt.conf

    if is_os("windows") then
        add_installfiles("platform/windows/qt.conf", {prefixdir = "bin"})
    end
