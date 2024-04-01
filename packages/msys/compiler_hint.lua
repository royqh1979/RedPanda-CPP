function apiVersion()
   return {
      kind = "compiler_hint",
      major = 0,
      minor = 2,
   }
end

local gnuArchMap = {
   i386 = "i686",
   x86_64 = "x86_64",
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

local function nameGeneratorClang(lang, arch, profile, isMingw)
   local template = {
      en_US = "%1 Clang %2, %3",
      pt_BR = "Clang %2 %1, %3",
      zh_CN = "%1 Clang %2，%3",
      zh_TW = "%1 Clang %2，%3",
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
   profileNameMap[profile][lang] or profileNameMap[profile].en_US)

end

local function mergeCompilerSet(compilerSet, other)
   local c = compilerSet
   local o = other
   for k, v in pairs(o) do
      c[k] = v
   end
end




















local function generateConfig(
   nameGen,
   programs,
   config)

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
   local debug_ = {
      name = nameGen(config.arch, "debug"),
      ccCmdOptDebugInfo = "on",
   }
   local debugWithAsan = {
      name = nameGen(config.arch, "debugWithAsan"),
      ccCmdOptDebugInfo = "on",
      ccCmdOptAddressSanitizer = "address",
   }
   mergeCompilerSet(release, commonOptions)
   mergeCompilerSet(debug_, commonOptions)
   mergeCompilerSet(debugWithAsan, commonOptions)
   return release, debug_, debugWithAsan
end

function main()
   local appArch = C_System.appArch()
   local libexecDir = C_System.appLibexecDir()
   local lang = C_Desktop.language()
   local supportedAppArches = C_System.supportedAppArchList()

   local compilerList = {}
   local noSearch = {}
   local preferCompiler = 0

   local function checkAndAddClang()
      if not C_FileSystem.isExecutable(libexecDir .. "/llvm-mingw/bin/clang.exe") then
         return
      end

      local binDir = libexecDir .. "/llvm-mingw/bin"
      local appTriplet = gnuArchMap[appArch] .. "-w64-mingw32"
      local appDllDir = libexecDir .. "/llvm-mingw/" .. appTriplet .. "/bin"
      do
         local libDir = libexecDir .. "/llvm-mingw/" .. appTriplet .. "/lib"

         local programs = {
            cCompiler = binDir .. "/" .. appTriplet .. "-clang.exe",
            cxxCompiler = binDir .. "/" .. appTriplet .. "-clang++.exe",
            make = binDir .. "/mingw32-make.exe",
            debugger = binDir .. "/lldb-mi.exe",
            debugServer = binDir .. "/lldb-server.exe",
            resourceCompiler = binDir .. "/" .. appTriplet .. "-windres.exe",
            binDirs = { binDir, appDllDir },
         }
         local customLinkParams = nil
         if C_FileSystem.exists(libDir .. "/utf8init.o") then
            customLinkParams = { "-Wl,utf8init.o", "-Wl,utf8manifest.o" }
         end
         local release, debug_, debugWithAsan = generateConfig(
         function(arch_, profile)
            return nameGeneratorClang(lang, arch_, profile, true)
         end,
         programs,
         {
            arch = appArch,
            customLinkParams = customLinkParams,
            isClang = true,
         })

         table.insert(compilerList, release)
         table.insert(compilerList, debug_)
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
      end

      for _, foreignArch in ipairs(supportedAppArches) do
         local gnuArch = gnuArchMap[foreignArch]
         if foreignArch ~= appArch and gnuArch ~= nil then
            local foreignTriplet = gnuArch .. "-w64-mingw32"
            local foreignDllDir = libexecDir .. "/llvm-mingw/" .. foreignTriplet .. "/bin"
            local libDir = libexecDir .. "/llvm-mingw/" .. foreignTriplet .. "/lib"
            local programs = {
               cCompiler = binDir .. "/" .. foreignTriplet .. "-clang.exe",
               cxxCompiler = binDir .. "/" .. foreignTriplet .. "-clang++.exe",
               make = binDir .. "/mingw32-make.exe",
               debugger = binDir .. "/lldb-mi.exe",
               debugServer = binDir .. "/lldb-server.exe",
               resourceCompiler = binDir .. "/" .. foreignTriplet .. "-windres.exe",
               binDirs = { binDir, foreignDllDir },
            }
            local customLinkParams = nil
            if C_FileSystem.exists(libDir .. "/utf8init.o") then
               customLinkParams = { "-Wl,utf8init.o", "-Wl,utf8manifest.o" }
            end
            local release, _, _ = generateConfig(
            function(arch_, profile)
               return nameGeneratorClang(lang, arch_, profile, true)
            end,
            programs,
            {
               arch = foreignArch,
               customLinkParams = customLinkParams,
               isClang = true,
            })

            table.insert(compilerList, release)
         end
      end

      table.insert(noSearch, binDir)

      local llvmOrgPath = C_System.readRegistry([[Software\LLVM\LLVM]], "") or C_System.readRegistry([[Software\Wow6432Node\LLVM\LLVM]], "")
      if not llvmOrgPath then
         return
      end
      local llvmOrgBinDir = llvmOrgPath .. "/bin"

      do
         local msvcTriplet = gnuArchMap[appArch] .. "-pc-windows-msvc"
         local libDir = libexecDir .. "/llvm-mingw/" .. msvcTriplet .. "/lib"
         local programs = {
            cCompiler = llvmOrgBinDir .. "/clang.exe",
            cxxCompiler = llvmOrgBinDir .. "/clang++.exe",
            make = binDir .. "/mingw32-make.exe",
            debugger = binDir .. "/lldb-mi.exe",
            debugServer = binDir .. "/lldb-server.exe",
            resourceCompiler = binDir .. "/" .. appTriplet .. "-windres.exe",
            binDirs = { llvmOrgBinDir },
            libDirs = { libDir },
         }
         local customLinkParams = { "-target", msvcTriplet }
         if C_FileSystem.exists(libDir .. "/utf8init.o") then
            table.insert(customLinkParams, "-Wl,utf8init.o")
            table.insert(customLinkParams, "-Wl,utf8manifest.o")
         end
         local release, debug_, _ = generateConfig(
         function(arch, profile)
            return nameGeneratorClang(lang, arch, profile, false)
         end,
         programs,
         {
            arch = appArch,
            customCompileParams = {
               "-target", msvcTriplet,
               "-fms-extensions",
               "-fms-compatibility",
               "-fdelayed-template-parsing",
            },
            customLinkParams = customLinkParams,
            isClang = true,
         })

         table.insert(compilerList, release)
         table.insert(compilerList, debug_)
      end

      for _, foreignArch in ipairs(supportedAppArches) do
         local gnuArch = gnuArchMap[foreignArch]
         if foreignArch ~= appArch and gnuArch ~= nil then
            local foreignTriplet = gnuArch .. "-w64-mingw32"
            local msvcTriplet = gnuArchMap[foreignArch] .. "-pc-windows-msvc"
            local libDir = libexecDir .. "/llvm-mingw/" .. msvcTriplet .. "/lib"
            local programs = {
               cCompiler = llvmOrgBinDir .. "/clang.exe",
               cxxCompiler = llvmOrgBinDir .. "/clang++.exe",
               make = binDir .. "/mingw32-make.exe",
               debugger = binDir .. "/lldb-mi.exe",
               debugServer = binDir .. "/lldb-server.exe",
               resourceCompiler = binDir .. "/" .. foreignTriplet .. "-windres.exe",
               binDirs = { llvmOrgBinDir },
               libDirs = { libDir },
            }
            local customLinkParams = { "-target", msvcTriplet }
            if C_FileSystem.exists(libDir .. "/utf8init.o") then
               table.insert(customLinkParams, "-Wl,utf8init.o")
               table.insert(customLinkParams, "-Wl,utf8manifest.o")
            end
            local release, _, _ = generateConfig(
            function(arch, profile)
               return nameGeneratorClang(lang, arch, profile, false)
            end,
            programs,
            {
               arch = foreignArch,
               customCompileParams = {
                  "-target", msvcTriplet,
                  "-fms-extensions",
                  "-fms-compatibility",
                  "-fdelayed-template-parsing",
               },
               customLinkParams = customLinkParams,
               isClang = true,
            })

            table.insert(compilerList, release)
         end
      end
      table.insert(noSearch, llvmOrgBinDir)
   end

   checkAndAddClang()

   local result = {
      compilerList = compilerList,
      noSearch = noSearch,
      preferCompiler = preferCompiler,
   }

   return result
end
