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
        en_US = "Release",
        pt_BR = "Lançamento",
        zh_CN = "发布",
        zh_TW = "發佈",
    },
    debug = {
        en_US = "Debug",
        pt_BR = "Depuração",
        zh_CN = "调试",
        zh_TW = "偵錯",
    },
    debugWithAsan = {
        en_US = "Debug with ASan",
        pt_BR = "Depuração com ASan",
        zh_CN = "调试（带 ASan）",
        zh_TW = "偵錯（帶 ASan）",
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
        name = nameGen("release"),
        staticLink = true,
        linkCmdOptStripExe = "on",
        ccCmdOptOptimize = "2",
    }
    local debug = {
        name = nameGen("debug"),
        ccCmdOptDebugInfo = "on",
    }
    local debugWithAsan = {
        name = nameGen("debugWithAsan"),
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
local noSearch = {
    libexecDir .. "/MinGW64/bin",
    libexecDir .. "/MinGW32/bin",
    libexecDir .. "/llvm-mingw/bin",
}
local preferCompiler = 0

local _ = [[
    ----------+----------+----------+--------------
              |  OS x86  |  OS x64  |   OS ARM64
    ----------+----------+----------+--------------
     App x86  | GCC32 OK | GCC32 OK |   GCC32 OK
              | LLVM OK  | LLVM OK  |    LLVM OK
    ----------+----------+----------+--------------
              |          | GCC32 OK |   GCC32 OK
     App x64  |   N/A    | GCC64 OK |   GCC64 OK
              |          | LLVM OK  |    LLVM OK
    ----------+----------+----------+--------------
              |          |          |   GCC32 OK
    App ARM64 |   N/A    |   N/A    | GCC64 depends
              |          |          |    LLVM OK
    ----------+----------+-------------------------
]]

function checkAndAddMingw32()
    if not C_FileSystem.isExecutable(libexecDir .. "/mingw32/bin/gcc.exe") then
        return
    end

    local binDir = libexecDir .. "/mingw32/bin"
    local libDir = libexecDir .. "/mingw32/i686-w64-mingw32/lib"
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
        function (profile) return nameGenerator.mingwGcc(lang, "i386", profile, true) end,
        programs,
        {
            customLinkParams = {extraObjects.utf8init, extraObjects.utf8manifest},
        }
    )
    table.insert(compilerList, release)
    table.insert(compilerList, debug)
    table.insert(compilerList, debugWithAsan)
    preferCompiler = 3

    release, debug, debugWithAsan = generateConfig(
        function (profile) return nameGenerator.mingwGcc(lang, "i386", profile, false) end,
        programs,
        {
            isAnsi = true,
        }
    )
    table.insert(compilerList, release)
    table.insert(compilerList, debug)
    table.insert(compilerList, debugWithAsan)
end

function checkAndAddMingw64()
    -- special check required for ARM64 App on ARM64 OS
    if arch == "arm64" and not contains(supportedAppArches, "x86_64") then
        return
    end
    if not C_FileSystem.isExecutable(libexecDir .. "/mingw64/bin/gcc.exe") then
        return
    end

    local binDir = libexecDir .. "/mingw64/bin"
    local libDir = libexecDir .. "/mingw64/x86_64-w64-mingw32/lib"
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
        function (profile) return nameGenerator.mingwGcc(lang, "x86_64", profile, true) end,
        programs,
        {
            customLinkParams = {extraObjects.utf8init, extraObjects.utf8manifest},
        }
    )
    table.insert(compilerList, release)
    table.insert(compilerList, debug)
    table.insert(compilerList, debugWithAsan)
    preferCompiler = 3

    release, debug, debugWithAsan = generateConfig(
        function (profile) return nameGenerator.mingwGcc(lang, "x86_64", profile, false) end,
        programs,
        {
            isAnsi = true,
        }
    )
    table.insert(compilerList, release)
    table.insert(compilerList, debug)
    table.insert(compilerList, debugWithAsan)
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
        function (profile) return nameGenerator.clang(lang, appArch, profile, true) end,
        programs,
        {
            customLinkParams = {extraObjects.utf8init, extraObjects.utf8manifest},
            isClang = true,
        }
    )
    table.insert(compilerList, release)
    table.insert(compilerList, debug)
    table.insert(compilerList, debugWithAsan)
    preferCompiler = 3

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
                function (profile) return nameGenerator.clang(lang, foreignArch, profile, true) end,
                programs,
                {
                    customLinkParams = {extraObjects.utf8init, extraObjects.utf8manifest},
                    isClang = true,
                }
            )
            table.insert(compilerList, release)
        end
    end

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
        function (profile) return nameGenerator.clang(lang, appArch, profile, false) end,
        programs,
        {
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
    table.insert(compilerList, debugWithAsan)

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
                function (profile) return nameGenerator.clang(lang, foreignArch, profile, false) end,
                programs,
                {
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
    checkAndAddMingw64()
    checkAndAddMingw32()
    checkAndAddClang()
elseif appArch == "arm64" then
    checkAndAddClang()
    checkAndAddMingw64()
    checkAndAddMingw32()
else
    checkAndAddMingw32()
    checkAndAddClang()
end

local result = {
    compilerList = compilerList,
    noSearch = noSearch,
    preferCompiler = preferCompiler,
}

return result
