# Add-on Interface (Unstable)

## Basic

- Add-on is a Lua script.
- Values passed or returned to Red Panda C++ must be JSON compatible, i.e.,
  - `nil` (maps to `null`)
  - `boolean`
  - `number`
  - `string`
  - empty `table` (maps to `null`)
  - `table` with only array part (maps to `array`)
  - `table` with only hash part (maps to `object`)
- Red Panda C++ APIs exposed to add-on are organized by groups.
  - Each group is a Lua table.
  - Each API is a function in the table.
  - Available API groups vary by add-on type.
- Localization is handled by add-on. e.g.
  ```lua
  local lang = C_Desktop.language()
  local localizedName = {
      en_US = "System",
      pt_BR = "Sistema",
      zh_CN = "系统",
      zh_TW = "系統",
  }
  return {
      name = localizedName[lang] or localizedName.en_US,
      -- ...
  }
  ```

## Simple Add-on

A simple add-on is a Lua script that returns a single value.

### Compiler Hint Add-on

If `$appLibexecDir/compiler_hint.lua` exists, it will be executed as compiler hint add-on when searching for compiler.

Available API groups:
- `C_Debug`
- `C_Desktop`
- `C_FileSystem`
- `C_System`
- `C_Util`

Return value schema:
```typescript
{
    // found compiler sets
    compilerList: [compilerSet],

    // do not search in these directories anymore
    noSearch: [string],

    // prefer compiler set index (in Lua, 1-based) in compilerList
    // 0 for no preference
    preferCompiler: number,
}
```

`compilerSet` schema:
```typescript
{
    name: string,

    // internal
    dumpMachine: string, // e.g. "x86_64-linux-gnu", "x86_64-w64-mingw32"
    version: string, // e.g. "13.2.1", "17.0.6"
    type: string, // e.g. "TDM-GCC", "MinGW"
    target: string, // e.g. "x86_64", "aarch64"
    compilerType: string, // "GCC" or "Clang"

    // general
    staticLink: boolean,
    customCompileParams: [string], // automatically sets useCustomCompileParams
    customLinkParams: [string], // automatically sets useCustomLinkParams
    execCharset: string, // automatically sets autoAddCharsetParams

    // setting
    // - code generation
    ccCmdOptOptimize: string,
    ccCmdOptStd: string,
    cCmdOptStd: string,
    ccCmdOptInstruction: string,
    ccCmdOptPointerSize: string,
    ccCmdOptDebugInfo: string,
    ccCmdOptProfileInfo: string,
    ccCmdOptSyntaxOnly: string,
    // - warnings
    ccCmdOptInhibitAllWarning: string,
    ccCmdOptWarningAll: string,
    ccCmdOptWarningExtra: string,
    ccCmdOptCheckIsoConformance: string,
    ccCmdOptWarningAsError: string,
    ccCmdOptAbortOnError: string,
    ccCmdOptStackProtector: string,
    ccCmdOptAddressSanitizer: string,
    // - linker
    ccCmdOptUsePipe: string,
    linkCmdOptNoLinkStdlib: string,
    linkCmdOptNoConsole: string,
    linkCmdOptStripExe: string,

    // directory
    binDirs: [string],
    cIncludeDirs: [string],
    cxxIncludeDirs: [string],
    libDirs: [string],
    defaultLibDirs: [string],
    defaultCIncludeDirs: [string],
    defaultCxxIncludeDirs: [string],

    // program
    cCompiler: string,
    cxxCompiler: string,
    make: string,
    debugger: string,
    debugServer: string,
    resourceCompiler: string,

    // output
    preprocessingSuffix: string,
    compilationProperSuffix: string,
    assemblingSuffix: string,
    executableSuffix: string,
    compilationStage: number,
}
```

## API Groups

### `C_Debug`

`C_Debug` is available to all add-on types.

- `C_Debug.debug`: `(message: string) -> ()`, print message to console (via `qDebug()`).

### `C_Desktop`

- `C_Desktop.desktopEnvironment`: `() -> string`, return desktop environment name.
  - `windows`: Windows (Win32 only)
  - `macos`: macOS
  - `xdg`: XDG-compliant desktop environment (e.g. GNOME, KDE Plasma)
  - `unknown`: other desktops or non-desktop environments (e.g. Windows UWP, Android)
- `C_Desktop.language`: `() -> string`, return language code.
  - e.g. `en_US`, `zh_CN`
- `C_Desktop.qtStyleList`: `() -> [string]`, return available Qt styles.
  - e.g. `{"breeze", "fusion", "windows"}`
- `C_Desktop.systemAppMode`: `() -> string`, return system app mode.
  - `light`: light mode
  - `dark`: dark mode
- `C_Desktop.systemStyle`: `() -> string`, return default Qt style.
  - e.g. `fusion`

### `C_FileSystem`

- `C_FileSystem.exists`: `(path: string) -> boolean`, return whether the path exists.
- `C_FileSystem.isExecutable`: `(path: string) -> boolean`, return whether the path is executable.

### `C_System`

- `C_System.appArch`: `() -> string`, return the architecture of Red Panda C++, name following `QSysInfo`.
  - e.g. `i386`, `x86_64`, `arm`, `arm64`, `riscv64`, `loongarch64`
  - Though unsupprted, MSVC arm64ec is handled correctly (returns `arm64ec`)
- `C_System.appDir`: `() -> string`, return the directory of Red Panda C++.
  - e.g. `/usr/bin`, `C:/Program Files/RedPanda-Cpp`
- `C_System.appLibexecDir`: `() -> string`, return the libexec directory of Red Panda C++.
  - e.g. `/usr/libexec/RedPandaCPP`, `C:/Program Files/RedPanda-Cpp`
- `C_System.appResourceDir`: `() -> string`, return the resource directory of Red Panda C++.
  - e.g. `/usr/share/RedPandaCPP`, `C:/Program Files/RedPanda-Cpp`
- `C_System.osArch`: `() -> string`, return the architecture of the OS, name following `QSysInfo`.
  - e.g. `i386`, `x86_64`, `arm`, `arm64`
  - Windows arm64 is handled correctly even if Red Panda C++ runs under emulation
- `C_System.supportedAppArchList`: `() -> [string]`, return supported application architectures by OS, name following `QSysInfo`.
  - e.g. `{"i386", "x86_64", "arm64"}`
  - Windows 10 1709 or later: accurate result
  - Legacy Windows: hardcoded
    - `{"i386", "x86_64"}` for x86_64 even though WoW64 is not available
  - macOS: accurate result supposed, but not tested
  - Linux: osArch + appArch + QEMU user mode emulation
    - No multilib detection. It’s packager’s responsibility to detect multilib support in `compiler_hint.lua`
  - other (BSD): osArch + appArch (no multilib)

Windows specific:

- `C_System.readRegistry`: `(subKey: string, name: string) -> string | nil`, read `subKey\name` from `HKEY_CURRENT_USER` and `HKEY_LOCAL_MACHINE` in order.
  - `name` can be empty string for default value

### `C_Util`

- `C_Util.format`: `(format: string, ...) -> string`, Qt-style string format, replace `%1`, `%2`, etc. with arguments.
