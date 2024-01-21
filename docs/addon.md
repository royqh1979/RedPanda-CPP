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
  - Available API groups vary by add-on kind.
  - See [`env.d.tl`](../addon/defs/global_env.d.tl) for API definitions.
- Add-on can be executed only if the API version is compatible.
- Localization is handled by add-on.

### API Version Check

Add-on must implement `apiVersion` that takes no argument and returns `ApiVersion` ([`env.d.tl`](../addon/defs/global_env.d.tl)).

Before execution, Red Panda C++ will call `apiVersion()` _without injecting any API group_[^1] to check whether the add-on is compatible with current Red Panda C++ (host).

[^1]: Thus do not call any API (incl. Lua standard library) in `apiVersion` and file scope.

Add-on is compatible with host if and only if:
```
(add-on kind = host kind) ∧ (
  ((add-on major = host major = 0) ∧ (add-on minor = host minor)) ∨
  ((add-on major = host major ≥ 1) ∧ (add-on minor ≤ host minor))
)
```

That is to say:
- API version is kind-specific.
- For a given kind, API major reported by add-on must be equal to host major.
  - API major = 0 means unstable, minor updates may break backward compatibility.
  - API major ≥ 1 means stable, minor updates keep backward compatibility.

### Types

Types in Red Panda C++ add-on interface are defined in [Teal](https://github.com/teal-language/tl) language, a typed dialect of Lua.

To make use of the type definitions, add-on can be written in Teal. To check and compile Teal add-on:
```bash
tl check --include-dir /path/to/RedPanda-CPP/addon --global-env-def defs/theme addon.tl
tl gen --include-dir /path/to/RedPanda-CPP/addon --global-env-def defs/theme --gen-compat off --gen-target 5.4 addon.tl
```

### Localization

Example:

```lua
local lang = C_Desktop.language()
-- note: explicitly declare as `{string:string}` for Teal
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

A simple add-on is a Lua script with a `main` function returning single value.

### Theme Add-on

Current API version: `theme:0.1`.

Available API groups:
- `C_Debug`
- `C_Desktop`
- `C_Util`

`main` function takes no argument and returns `Theme` ([`theme.d.tl`](../addon/defs/theme.d.tl)).

### Compiler Hint Add-on

Current API version: `compiler_hint:0.1`.

If `$appLibexecDir/compiler_hint.lua` exists, it will be executed as compiler hint add-on when searching for compiler.

Available API groups:
- `C_Debug`
- `C_Desktop`
- `C_FileSystem`
- `C_System`
- `C_Util`

`main` function takes no argument and returns `CompilerHint` ([`compiler_hint.d.tl`](../addon/defs/compiler_hint.d.tl)).
