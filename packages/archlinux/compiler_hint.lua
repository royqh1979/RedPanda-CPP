function apiVersion()
   return {
      kind = "compiler_hint",
      major = 0,
      minor = 2,
   }
end








local compilerNameTemplate = {
   system = {
      en_US = "System %1, %2",
      pt_BR = "%1 sistema, %2",
      zh_CN = "系统 %1，%2",
      zh_TW = "系統 %1，%2",
   },
   multilib = {
      en_US = "Multilib %1, %2",
      pt_BR = "%1 multilib, %2",
      zh_CN = "Multilib %1，%2",
      zh_TW = "Multilib %1，%2",
   },
   cross = {
      en_US = "Cross %1 %3, %2",
      pt_BR = "%1 cruzado %3, %2",
      zh_CN = "交叉编译 %1 %3，%2",
      zh_TW = "交叉編譯 %1 %3，%2",
   },
   mingw = {
      en_US = "MinGW %1 %3, %2",
      pt_BR = "MinGW %1 %3, %2",
      zh_CN = "MinGW %1 %3，%2",
      zh_TW = "MinGW %1 %3，%2",
   },
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

local function nameGenerator(lang, name, kind, profile, arch)
   local template = compilerNameTemplate[kind][lang] or compilerNameTemplate[kind].en_US
   local profileName = profileNameMap[profile][lang] or profileNameMap[profile].en_US
   return C_Util.format(template, name, profileName, arch)
end

local function mergeCompilerSet(compilerSet, other)
   local c = compilerSet
   local o = other
   for k, v in pairs(o) do
      c[k] = v
   end
end








local function generateConfig(
   lang, name, kind,
   cCompiler, cxxCompiler,
   config)

   local commonOptions = {
      cCompiler = cCompiler,
      cxxCompiler = cxxCompiler,
      debugger = "/usr/bin/gdb",
      debugServer = "/usr/bin/gdbserver",
      make = "/usr/bin/make",
      compilerType = name:sub(1, 5) == "Clang" and "Clang" or "GCC_UTF8",
      preprocessingSuffix = ".i",
      compilationProperSuffix = ".s",
      assemblingSuffix = ".o",
      executableSuffix = kind == "mingw" and ".exe" or "",
      compilationStage = 3,
      ccCmdOptUsePipe = "on",
      ccCmdOptWarningAll = "on",
      ccCmdOptWarningExtra = "on",
      ccCmdOptCheckIsoConformance = "on",
      binDirs = { "/usr/bin" },
   }
   if kind == "multilib" then
      commonOptions.ccCmdOptPointerSize = "32"
   end
   if kind == "mingw" then
      commonOptions.resourceCompiler = "/usr/bin/" .. config.triplet .. "-windres"
   end
   if config.customCompileParams then
      commonOptions.customCompileParams = config.customCompileParams
   end
   if config.customLinkParams then
      commonOptions.customLinkParams = config.customLinkParams
   end
   local release = {
      name = nameGenerator(lang, name, kind, "release", config.arch),
      staticLink = true,
      linkCmdOptStripExe = "on",
      ccCmdOptOptimize = "2",
   }
   local debug_ = {
      name = nameGenerator(lang, name, kind, "debug", config.arch),
      ccCmdOptDebugInfo = "on",
   }
   local debugWithAsan = {
      name = nameGenerator(lang, name, kind, "debugWithAsan", config.arch),
      ccCmdOptDebugInfo = "on",
      ccCmdOptAddressSanitizer = "address",
   }
   mergeCompilerSet(release, commonOptions)
   mergeCompilerSet(debug_, commonOptions)
   mergeCompilerSet(debugWithAsan, commonOptions)
   return release, debug_, debugWithAsan
end

function main()
   local arch = C_System.osArch()
   local libexecDir = C_System.appLibexecDir()
   local lang = C_Desktop.language()

   local compilerList = {}

   do
      local release, debug_, debugWithAsan = generateConfig(
      lang, "GCC", "system", "/usr/bin/gcc", "/usr/bin/g++",
      {})

      table.insert(compilerList, release)
      table.insert(compilerList, debug_)
      table.insert(compilerList, debugWithAsan)
   end

   local versionedGccs = C_FileSystem.matchFiles("/usr/bin", "^gcc-[0-9]+$")
   for _, gcc in ipairs(versionedGccs) do
      local version = gcc:sub(5)
      local name = "GCC " .. version
      local release, debug_, debugWithAsan = generateConfig(
      lang, name, "system", "/usr/bin/" .. gcc, "/usr/bin/g++-" .. version,
      {})

      table.insert(compilerList, release)
      table.insert(compilerList, debug_)
      table.insert(compilerList, debugWithAsan)
   end

   if C_FileSystem.isExecutable("/usr/bin/clang") then
      local release, debug_, debugWithAsan = generateConfig(
      lang, "Clang", "system", "/usr/bin/clang", "/usr/bin/clang++",
      {})

      table.insert(compilerList, release)
      table.insert(compilerList, debug_)
      table.insert(compilerList, debugWithAsan)
   end


   if arch == "x86_64" and C_FileSystem.isExecutable("/usr/lib32/libstdc++.so") then
      do
         local release, debug_, debugWithAsan = generateConfig(
         lang, "GCC", "multilib", "/usr/bin/gcc", "/usr/bin/g++",
         {})

         table.insert(compilerList, release)
         table.insert(compilerList, debug_)
         table.insert(compilerList, debugWithAsan)
      end

      if C_FileSystem.isExecutable("/usr/bin/clang") then
         local release, debug_, debugWithAsan = generateConfig(
         lang, "Clang", "multilib", "/usr/bin/clang", "/usr/bin/clang++",
         {})

         table.insert(compilerList, release)
         table.insert(compilerList, debug_)
         table.insert(compilerList, debugWithAsan)
      end
   end


   if (
      arch == "x86_64" and
      C_FileSystem.exists("/proc/sys/fs/binfmt_misc/qemu-aarch64") and
      C_FileSystem.isExecutable("/usr/bin/aarch64-linux-gnu-gcc")) then

      local release, _, _ = generateConfig(
      lang, "GCC", "cross",
      "/usr/bin/aarch64-linux-gnu-gcc", "/usr/bin/aarch64-linux-gnu-g++",
      { arch = "aarch64" })

      table.insert(compilerList, release)
   end


   if (
      arch == "x86_64" and (
      C_FileSystem.exists("/proc/sys/fs/binfmt_misc/DOSWin") or
      C_FileSystem.exists("/proc/sys/fs/binfmt_misc/WSLInterop"))) then


      if C_FileSystem.isExecutable("/usr/bin/x86_64-w64-mingw32-gcc") then
         do
            local release, _, _ = generateConfig(
            lang, "GCC", "mingw",
            "/usr/bin/x86_64-w64-mingw32-gcc", "/usr/bin/x86_64-w64-mingw32-g++",
            {
               arch = "x86_64",
               triplet = "x86_64-w64-mingw32",
               customLinkParams = {},
            })

            table.insert(compilerList, release)
         end


         if C_FileSystem.isExecutable("/usr/bin/clang") then
            local release, _, _ = generateConfig(
            lang, "Clang", "mingw", "/usr/bin/clang", "/usr/bin/clang++",
            {
               arch = "x86_64",
               triplet = "x86_64-w64-mingw32",
               customCompileParams = { "-target", "x86_64-w64-mingw32" },
               customLinkParams = {
                  "-target", "x86_64-w64-mingw32",
                  "-lstdc++", "-lwinpthread",
               },
            })

            table.insert(compilerList, release)
         end
      end

      if C_FileSystem.isExecutable("/usr/bin/i686-w64-mingw32-gcc") then
         do
            local release, _, _ = generateConfig(
            lang, "GCC", "mingw",
            "/usr/bin/i686-w64-mingw32-gcc", "/usr/bin/i686-w64-mingw32-g++",
            {
               arch = "i686",
               triplet = "i686-w64-mingw32",
               customLinkParams = {},
            })

            table.insert(compilerList, release)
         end


         if C_FileSystem.isExecutable("/usr/bin/clang") then
            local release, _, _ = generateConfig(
            lang, "Clang", "mingw",
            "/usr/bin/clang", "/usr/bin/clang++",
            {
               arch = "i686",
               triplet = "i686-w64-mingw32",
               customCompileParams = { "-target", "i686-w64-mingw32" },
               customLinkParams = {
                  "-target", "i686-w64-mingw32",
                  "-lstdc++", "-lwinpthread",
               },
            })

            table.insert(compilerList, release)
         end
      end
   end

   local result = {
      compilerList = compilerList,
      noSearch = {
         "/usr/bin",
         "/opt/cuda/bin",
         "/usr/lib/ccache/bin",
      },
      preferCompiler = 3,
   }

   return result

end
