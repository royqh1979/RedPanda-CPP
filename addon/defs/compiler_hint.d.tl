local env = require("defs.global_env")

global C_Debug = env.C_Debug
global C_Desktop = env.C_Desktop
global C_FileSystem = env.C_FileSystem
global C_System = env.C_System
global C_Util = env.C_Util

global record CompilerHint
    -- found compiler sets
    compilerList: {CompilerSet}

    -- do not search in these directories anymore
    noSearch: {string}

    -- prefer compiler set index (in Lua, 1-based) in compilerList
    -- 0 for no preference
    preferCompiler: integer

    record CompilerSet
        name: string

        -- internal

        -- e.g. "x86_64-linux-gnu", "x86_64-w64-mingw32"
        dumpMachine: string
        -- e.g. "13.2.1", "17.0.6"
        version: string
        -- e.g. "TDM-GCC", "MinGW"
        type: string
        -- e.g. "x86_64", "aarch64"
        target: string
        compilerType: CompilerType

        -- general

        staticLink: boolean
        -- automatically sets useCustomCompileParams
        customCompileParams: {string}
        -- automatically sets useCustomLinkParams
        customLinkParams: {string}
        -- automatically sets autoAddCharsetParams
        execCharset: string

        -- setting: code generation

        ccCmdOptOptimize: string
        ccCmdOptStd: string
        cCmdOptStd: string
        ccCmdOptInstruction: string
        ccCmdOptPointerSize: string
        ccCmdOptDebugInfo: string
        ccCmdOptProfileInfo: string
        ccCmdOptSyntaxOnly: string

        -- setting: warnings

        ccCmdOptInhibitAllWarning: string
        ccCmdOptWarningAll: string
        ccCmdOptWarningExtra: string
        ccCmdOptCheckIsoConformance: string
        ccCmdOptWarningAsError: string
        ccCmdOptAbortOnError: string
        ccCmdOptStackProtector: string
        ccCmdOptAddressSanitizer: string

        -- setting: linker

        ccCmdOptUsePipe: string
        linkCmdOptNoLinkStdlib: string
        linkCmdOptNoConsole: string
        linkCmdOptStripExe: string

        -- directory

        binDirs: {string}
        cIncludeDirs: {string}
        cxxIncludeDirs: {string}
        libDirs: {string}
        defaultLibDirs: {string}
        defaultCIncludeDirs: {string}
        defaultCxxIncludeDirs: {string}

        -- program

        cCompiler: string
        cxxCompiler: string
        make: string
        debugger: string
        debugServer: string
        resourceCompiler: string

        -- output

        preprocessingSuffix: string
        compilationProperSuffix: string
        assemblingSuffix: string
        executableSuffix: string
        compilationStage: integer

        enum CompilerType
            "GCC"
            "GCC_UTF8"
            "Clang"
        end
    end
end
