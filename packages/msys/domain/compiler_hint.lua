local arch = C_System.osArch()
local appArch = C_System.appArch()
local libexecDir = C_System.appLibexecDir()
local lang = C_Desktop.language()
local supportedAppArches = C_System.supportedAppArchList()

local gnuArchMap = {
    i386 = "i686",
    x86_64 = "x86_64",
    arm = "armv7",
    arm64 = "aarch64",
}

local profileNameMap = {
    release = {
        en_US = "release",
        pt_BR = "lançamento",
        zh_CN = "发布",
        zh_TW = "發佈",
    },
    debug = {
        en_US = "debug",
        pt_BR = "depuração",
        zh_CN = "调试",
        zh_TW = "偵錯",
    },
    debugWithAsan = {
        en_US = "debug with ASan",
        pt_BR = "depuração com ASan",
        zh_CN = "ASan 调试",
        zh_TW = "ASan 偵錯",
    },
}

local nameGenerator = {
    mingwGcc = function (lang, arch, profile, isUtf8)
        local template = {
            en_US = "MinGW GCC %1 in %2, %3",
            pt_BR = "GCC MinGW %1 em %2, %3",
            zh_CN = "%2 MinGW GCC %1，%3",
            zh_TW = "%2 MinGW GCC %1，%3",
        }
        local systemCodePage = {
            en_US = "system code page",
            pt_Br = "página de código do sistema",
            zh_CN = "系统代码页",
            zh_TW = "系統代碼頁",
        }
        return C_Util.format(
            template[lang] or template.en_US,
            gnuArchMap[arch],
            isUtf8 and "UTF-8" or systemCodePage[lang] or systemCodePage.en_US,
            profileNameMap[profile][lang] or profileNameMap[profile].en_US
        )
    end,
    clang = function (lang, arch, profile, isMingw)
        local template = {
            en_US = "%1 Clang %2, %3",
            pt_BR = "Clang %2 %1, %3",
            zh_CN = "%1 Clang %2，%3",
            zh_CN = "%1 Clang %2，%3",
        }
        local msvcCompatible = {
            en_US = "MSVC-compatible",
            pt_BR = "compatível com MSVC",
            zh_CN = "兼容 MSVC 的",
            zh_TW = "相容 MSVC 的",
        }
        return C_Util.format(
            template[lang] or template.en_US,
            isMingw and "LLVM-MinGW" or msvcCompatible[lang] or msvcCompatible.en_US,
            gnuArchMap[arch],
            profileNameMap[profile][lang] or profileNameMap[profile].en_US
        )
    end,
}

function generateConfig(nameGen, programs, config)
    local commonOptions = {
        cCompiler = programs.cCompiler,
        cxxCompiler = programs.cxxCompiler,
        debugger = programs.debugger,
        debugServer = programs.debugServer,
        make = programs.make,
        resourceCompiler = programs.resourceCompiler,
        binDirs = programs.binDirs,
        compilerType = config.isClang and "Clang" or "GCC_UTF8",
        preprocessingSuffix = ".i",
        compilationProperSuffix = ".s",
        assemblingSuffix = ".o",
        executableSuffix = ".exe",
        compilationStage = 3,
        ccCmdOptUsePipe = "on",
        ccCmdOptWarningAll = "on",
        ccCmdOptWarningExtra = "on",
        ccCmdOptCheckIsoConformance = "on",
    }
    if programs.libDirs then
        commonOptions.libDirs = programs.libDirs
    end
    if config.isAnsi then
        commonOptions.execCharset = "SYSTEM"
    end
    if config.customCompileParams then
        commonOptions.customCompileParams = config.customCompileParams
    end
    if config.customLinkParams then
        commonOptions.customLinkParams = config.customLinkParams
    end
    local release = {
        name = nameGen(config.arch, "release"),
        staticLink = true,
        linkCmdOptStripExe = "on",
        ccCmdOptOptimize = "2",
    }
    local debug = {
        name = nameGen(config.arch, "debug"),
        ccCmdOptDebugInfo = "on",
    }
    local debugWithAsan = {
        name = nameGen(config.arch, "debugWithAsan"),
        ccCmdOptDebugInfo = "on",
        ccCmdOptAddressSanitizer = "address",
    }
    for k, v in pairs(commonOptions) do
        release[k] = v
        debug[k] = v
        debugWithAsan[k] = v
    end
    return release, debug, debugWithAsan
end

function contains(t, v)
    for _, vv in ipairs(t) do
        if vv == v then
            return true
        end
    end
    return false
end

local compilerList = {}
local noSearch = {}
local preferCompiler = 0

function checkAndAddMingw(arch)
    local binDir
    local libDir
    local excludeBinDir
    if arch == "i386" then
        binDir = libexecDir .. "/mingw32/bin" -- must match case because Windows filesystem can be case sensitive
        libDir = libexecDir .. "/mingw32/i686-w64-mingw32/lib"
        excludeBinDir = libexecDir .. "/MinGW32/bin" -- workaround for path check
    elseif arch == "x86_64" then
        binDir = libexecDir .. "/mingw64/bin"
        libDir = libexecDir .. "/mingw64/x86_64-w64-mingw32/lib"
        excludeBinDir = libexecDir .. "/MinGW64/bin"
    else
        return
    end

    if not C_FileSystem.isExecutable(binDir .. "/gcc.exe") then
        return
    end

    local programs = {
        cCompiler = binDir .. "/gcc.exe",
        cxxCompiler = binDir .. "/g++.exe",
        make = binDir .. "/mingw32-make.exe",
        debugger = binDir .. "/gdb.exe",
        debugServer = binDir .. "/gdbserver.exe",
        resourceCompiler = binDir .. "/windres.exe",
        binDirs = {binDir},
    }
    local extraObjects = {
        utf8init = libDir .. "/utf8init.o",
        utf8manifest = libDir .. "/utf8manifest.o",
    }

    local release, debug, debugWithAsan = generateConfig(
        function (arch, profile) return nameGenerator.mingwGcc(lang, arch, profile, true) end,
        programs,
        {
            arch = arch,
            customLinkParams = {extraObjects.utf8init, extraObjects.utf8manifest},
        }
    )
    table.insert(compilerList, release)
    table.insert(compilerList, debug)
    if preferCompiler == 0 then
        preferCompiler = 2
    end

    release, debug, debugWithAsan = generateConfig(
        function (arch, profile) return nameGenerator.mingwGcc(lang, arch, profile, false) end,
        programs,
        {
            arch = arch,
            isAnsi = true,
        }
    )
    table.insert(compilerList, release)
    table.insert(compilerList, debug)

    table.insert(noSearch, excludeBinDir)
end

function checkAndAddClang()
    if not C_FileSystem.isExecutable(libexecDir .. "/llvm-mingw/bin/clang.exe") then
        return
    end

    -- appArch is always debuggable
    local appTriplet = gnuArchMap[appArch] .. "-w64-mingw32"
    local binDir = libexecDir .. "/llvm-mingw/bin"
    local libDir = libexecDir .. "/llvm-mingw/" .. appTriplet .. "/lib"
    local programs = {
        cCompiler = binDir .. "/" .. appTriplet .. "-clang.exe",
        cxxCompiler = binDir .. "/" .. appTriplet .. "-clang++.exe",
        make = binDir .. "/mingw32-make.exe",
        debugger = binDir .. "/lldb-mi.exe",
        debugServer = binDir .. "/lldb-server.exe",
        resourceCompiler = binDir .. "/" .. appTriplet .. "-windres.exe",
        binDirs = {binDir},
    }
    local extraObjects = {
        utf8init = libDir .. "/utf8init.o",
        utf8manifest = libDir .. "/utf8manifest.o",
    }
    local release, debug, debugWithAsan = generateConfig(
        function (arch, profile) return nameGenerator.clang(lang, arch, profile, true) end,
        programs,
        {
            arch = appArch,
            customLinkParams = {extraObjects.utf8init, extraObjects.utf8manifest},
            isClang = true,
        }
    )
    table.insert(compilerList, release)
    table.insert(compilerList, debug)
    if appArch ~= "arm64" then
        table.insert(compilerList, debugWithAsan)
        if preferCompiler == 0 then
            preferCompiler = 3
        end
    else
        if preferCompiler == 0 then
            preferCompiler = 2
        end
    end

    for _, foreignArch in ipairs(supportedAppArches) do
        if foreignArch ~= appArch then
            local foreignTriplet = gnuArchMap[foreignArch] .. "-w64-mingw32"
            local libDir = libexecDir .. "/llvm-mingw/" .. foreignTriplet .. "/lib"
            local programs = {
                cCompiler = binDir .. "/" .. foreignTriplet .. "-clang.exe",
                cxxCompiler = binDir .. "/" .. foreignTriplet .. "-clang++.exe",
                make = binDir .. "/mingw32-make.exe",
                debugger = binDir .. "/lldb-mi.exe",
                debugServer = binDir .. "/lldb-server.exe",
                resourceCompiler = binDir .. "/" .. foreignTriplet .. "-windres.exe",
                binDirs = {binDir},
            }
            local extraObjects = {
                utf8init = libDir .. "/utf8init.o",
                utf8manifest = libDir .. "/utf8manifest.o",
            }
            local release, debug, debugWithAsan = generateConfig(
                function (arch, profile) return nameGenerator.clang(lang, arch, profile, true) end,
                programs,
                {
                    arch = foreignArch,
                    customLinkParams = {extraObjects.utf8init, extraObjects.utf8manifest},
                    isClang = true,
                }
            )
            table.insert(compilerList, release)
        end
    end

    table.insert(noSearch, binDir)

    local llvmOrgPath = C_System.readRegistry([[Software\LLVM\LLVM]], "") or C_System.readRegistry([[Software\Wow6432Node\LLVM\LLVM]], "")
    if not llvmOrgPath then
        return
    end

    local llvmOrgBinDir = llvmOrgPath .. "/bin"
    local msvcTriplet = gnuArchMap[appArch] .. "-pc-windows-msvc"
    local libDir = libexecDir .. "/llvm-mingw/" .. msvcTriplet .. "/lib"
    local programs = {
        cCompiler = llvmOrgBinDir .. "/clang.exe",
        cxxCompiler = llvmOrgBinDir .. "/clang++.exe",
        make = binDir .. "/mingw32-make.exe",
        debugger = binDir .. "/lldb-mi.exe",
        debugServer = binDir .. "/lldb-server.exe",
        resourceCompiler = binDir .. "/" .. appTriplet .. "-windres.exe",
        binDirs = {llvmOrgBinDir},
        libDirs = {libDir},
    }
    local extraObjects = {
        utf8init = libDir .. "/utf8init.o",
        utf8manifest = libDir .. "/utf8manifest.o",
    }
    local release, debug, debugWithAsan = generateConfig(
        function (arch, profile) return nameGenerator.clang(lang, arch, profile, false) end,
        programs,
        {
            arch = appArch,
            customCompileParams = {
                "-target", msvcTriplet,
                "-fms-extensions",
                "-fms-compatibility",
                "-fdelayed-template-parsing",
            },
            customLinkParams = {
                "-target", msvcTriplet,
                extraObjects.utf8init, extraObjects.utf8manifest,
            },
            isClang = true,
        }
    )
    table.insert(compilerList, release)
    table.insert(compilerList, debug)

    for _, foreignArch in ipairs(supportedAppArches) do
        if foreignArch ~= appArch then
            local foreignTriplet = gnuArchMap[foreignArch] .. "-w64-mingw32"
            local msvcTriplet = gnuArchMap[foreignArch] .. "-pc-windows-msvc"
            local libDir = libexecDir .. "/llvm-mingw/" .. msvcTriplet .. "/lib"
            local programs = {
                cCompiler = llvmOrgBinDir .. "/clang.exe",
                cxxCompiler = llvmOrgBinDir .. "/clang++.exe",
                make = binDir .. "/mingw32-make.exe",
                debugger = binDir .. "/lldb-mi.exe",
                debugServer = binDir .. "/lldb-server.exe",
                resourceCompiler = binDir .. "/" .. foreignTriplet .. "-windres.exe",
                binDirs = {llvmOrgBinDir},
                libDirs = {libDir},
            }
            local extraObjects = {
                utf8init = libDir .. "/utf8init.o",
                utf8manifest = libDir .. "/utf8manifest.o",
            }
            local release, debug, debugWithAsan = generateConfig(
                function (arch, profile) return nameGenerator.clang(lang, arch, profile, false) end,
                programs,
                {
                    arch = foreignArch,
                    customCompileParams = {
                        "-target", msvcTriplet,
                        "-fms-extensions",
                        "-fms-compatibility",
                        "-fdelayed-template-parsing",
                    },
                    customLinkParams = {
                        "-target", msvcTriplet,
                        extraObjects.utf8init, extraObjects.utf8manifest,
                    },
                    isClang = true,
                }
            )
            table.insert(compilerList, release)
        end
    end
    table.insert(noSearch, llvmOrgBinDir)
end

if appArch == "x86_64" then
    checkAndAddMingw("x86_64")
    checkAndAddClang()
elseif appArch == "arm64" then
    checkAndAddClang()
else
    checkAndAddMingw("i386")
    checkAndAddClang()
end

local result = {
    compilerList = compilerList,
    noSearch = noSearch,
    preferCompiler = preferCompiler,
}

return result
