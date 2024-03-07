global record ApiVersion
    kind: Kind
    major: integer
    minor: integer

    enum Kind
        "theme"
        "compiler_hint"
    end
end

local record env
    record C_Debug
        -- print message to console, with Qt-style string format
        debug: function (format: string, ...: any): nil

        -- show message box, with Qt-style string format
        messageBox: function (format: string, ...: any): nil
    end

    record C_Desktop
        -- return desktoop environment name
        desktopEnvironment: function (): DesktopEnvironment

        -- return language code, e.g. "en_US", "zh_CN"
        language: function (): string

        -- return available Qt styles, e.g. {"breeze", "fusion", "windows"}
        qtStyleList: function (): {string}

        -- return system app mode, light or dark
        systemAppMode: function (): AppMode

        -- return default Qt style, e.g. "fusion"
        systemStyle: function (): string

        enum DesktopEnvironment
            "windows" -- Windows Win32
            "macos"   -- macOS
            "xdg"     -- XDG-compliant desktop environment (e.g. GNOME, KDE Plasma)
            "unknown" -- other desktops or non-desktop environments (e.g. Windows UWP, Android)
        end

        enum AppMode
            "light"
            "dark"
        end
    end

    record C_FileSystem
        -- return whether the path exists
        exists: function (path: string): boolean
    
        -- return whether the path is executable
        isExecutable: function (path: string): boolean

        -- return matched files in the directory (non-recursive), with Qt-style regex
        -- e.g. matchFiles("/usr/bin", "^gcc-[0-9]+$") may return {"gcc-12", "gcc-13"}
        matchFiles: function (dir: string, qtRegex: string): {string}
    end

    record C_System
        -- returns the architecture of Red Panda C++, name following `QSysInfo`
        -- e.g. "i386", "x86_64", "arm", "arm64", "riscv64", "loongarch64"
        -- though unsupported, MSVC arm64ec is handled correctly (returns "arm64ec")
        appArch: function (): string

        -- returns the directory of Red Panda C++
        -- e.g. "/usr/bin", "C:/Program Files/RedPanda-Cpp", "C:/Users/中文/AppData/Local/RedPanda-CPP"
        appDir: function (): string

        -- returns the libexec directory of Red Panda C++
        -- e.g. "/usr/libexec/RedPandaCPP", "C:/Program Files/RedPanda-Cpp"
        appLibexecDir: function (): string

        -- returns the resource directory of Red Panda C++
        -- e.g. "/usr/share/RedPandaCPP", "C:/Program Files/RedPanda-Cpp"
        appResourceDir: function (): string

        -- returns the architecture of the OS, name following `QSysInfo`
        -- e.g. "i386", "x86_64", "arm", "arm64"
        -- Windows arm64 is handled correctly even if Red Panda C++ runs under emulation
        osArch: function (): string

        -- returns (1) stdout, (2) stderr and (3) exit status of the program
        popen: function(prog: string, args: {string}, option: PopenOption | nil): string, string, PopenResult

        -- returns supported application architectures by OS, name following `QSysInfo`
        -- e.g. {"i386", "x86_64", "arm64"}
        -- Windows 10 1709 or later: accurate result
        -- Legacy Windows: hardcoded result, i.e. "i386" is always included even though WoW64 is not available
        -- macOS: accurate result supposed, but not tested
        -- Linux: osArch + appArch + QEMU user mode emulation, no multilib detection
        -- other (BSD): osArch + appArch, no multilib detection
        supportedAppArchList: function (): {string}

        -- read `subKey\name` from HKEY_CURRENT_USER and HKEY_LOCAL_MACHINE in order (Windows only)
        readRegistry: function (subKey: string, name: string): string | nil

        record PopenOption
            timeout: integer | nil -- in milliseconds
        end

        record PopenResult
            exitStatus: QProcessExitStatus
            error: QProcessProcessError | nil
            exitCode: integer | nil -- only valid if exitStatus is NormalExit
        end

        enum QProcessExitStatus
            "NormalExit"
            "CrashExit"
        end

        enum QProcessProcessError
            "FailedToStart"
            "Crashed"
            "Timedout"
            "WriteError"
            "ReadError"
            "UnknownError"
        end
    end

    record C_Util
        -- Qt-style string format, replace %1, %2, etc. with arguments
        format: function (format: string, ...: any): string
    end
end

return env
