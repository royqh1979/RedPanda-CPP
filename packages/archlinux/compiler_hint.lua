function apiVersion()
   return {
      kind = "compiler_hint",
      major = 0,
      minor = 1,
   }
end

local nameMap = {
   systemGcc = {
      en_US = "System GCC",
      pt_BR = "GCC do sistema",
      zh_CN = "系统 GCC",
      zh_TW = "系統 GCC",
   },
   systemClang = {
      en_US = "System Clang",
      pt_BR = "Clang do sistema",
      zh_CN = "系统 Clang",
      zh_TW = "系統 Clang",
   },
   multilibGcc = {
      en_US = "Multilib GCC",
      pt_BR = "GCC multilib",
      zh_CN = "Multilib GCC",
      zh_TW = "Multilib GCC",
   },
   multilibClang = {
      en_US = "Multilib Clang",
      pt_BR = "Clang multilib",
      zh_CN = "Multilib Clang",
      zh_TW = "Multilib Clang",
   },
   crossGcc = {
      en_US = "Cross GCC",
      pt_BR = "GCC cruzado",
      zh_CN = "交叉编译 GCC",
      zh_TW = "交叉編譯 GCC",
   },
   mingwGcc = {
      en_US = "MinGW GCC",
      pt_BR = "GCC MinGW",
      zh_CN = "MinGW GCC",
      zh_TW = "MinGW GCC",
   },
   mingwClang = {
      en_US = "MinGW Clang",
      pt_BR = "Clang MinGW",
      zh_CN = "MinGW Clang",
      zh_TW = "MinGW Clang",
   },
   release = {
      en_US = ", release",
      pt_BR = ", lançamento",
      zh_CN = "，发布",
      zh_TW = "，發佈",
   },
   debug = {
      en_US = ", debug",
      pt_BR = ", depuração",
      zh_CN = "，调试",
      zh_TW = "，偵錯",
   },
   debugWithAsan = {
      en_US = ", debug with ASan",
      pt_BR = ", depuração com ASan",
      zh_CN = "，ASan 调试",
      zh_TW = "，ASan 偵錯",
   },
}

local function mergeCompilerSet(compilerSet, other)
   local c = compilerSet
   local o = other
   for k, v in pairs(o) do
      c[k] = v
   end
end










local function generateConfig(
   name, lang,
   cCompiler, cxxCompiler,
   config)

   local commonOptions = {
      cCompiler = cCompiler,
      cxxCompiler = cxxCompiler,
      debugger = "/usr/bin/gdb",
      debugServer = "/usr/bin/gdbserver",
      make = "/usr/bin/make",
      compilerType = config.isClang and "Clang" or "GCC_UTF8",
      preprocessingSuffix = ".i",
      compilationProperSuffix = ".s",
      assemblingSuffix = ".o",
      executableSuffix = config.isMingw and ".exe" or "",
      compilationStage = 3,
      ccCmdOptUsePipe = "on",
      ccCmdOptWarningAll = "on",
      ccCmdOptWarningExtra = "on",
      ccCmdOptCheckIsoConformance = "on",
      binDirs = { "/usr/bin" },
   }
   if config.isMultilib then
      commonOptions.ccCmdOptPointerSize = "32"
   end
   if config.isMingw then
      commonOptions.resourceCompiler = "/usr/bin/" .. config.triplet .. "-windres"
   end
   if config.customCompileParams then
      commonOptions.customCompileParams = config.customCompileParams
   end
   if config.customLinkParams then
      commonOptions.customLinkParams = config.customLinkParams
   end
   local release = {
      name = name .. (nameMap.release[lang] or nameMap.release.en_US),
      staticLink = true,
      linkCmdOptStripExe = "on",
      ccCmdOptOptimize = "2",
   }
   local debug_ = {
      name = name .. (nameMap.debug[lang] or nameMap.debug.en_US),
      ccCmdOptDebugInfo = "on",
   }
   local debugWithAsan = {
      name = name .. (nameMap.debugWithAsan[lang] or nameMap.debugWithAsan.en_US),
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
      nameMap.systemGcc[lang] or nameMap.systemGcc.en_US, lang,
      "/usr/bin/gcc", "/usr/bin/g++",
      {})

      table.insert(compilerList, release)
      table.insert(compilerList, debug_)
      table.insert(compilerList, debugWithAsan)
   end

   if C_FileSystem.isExecutable("/usr/bin/clang") then
      local release, debug_, debugWithAsan = generateConfig(
      nameMap.systemClang[lang] or nameMap.systemClang.en_US, lang,
      "/usr/bin/clang", "/usr/bin/clang++",
      { isClang = true })

      table.insert(compilerList, release)
      table.insert(compilerList, debug_)
      table.insert(compilerList, debugWithAsan)
   end


   if arch == "x86_64" and C_FileSystem.isExecutable("/usr/lib32/libstdc++.so") then
      do
         local release, debug_, debugWithAsan = generateConfig(
         nameMap.multilibGcc[lang] or nameMap.multilibGcc.en_US, lang,
         "/usr/bin/gcc", "/usr/bin/g++",
         { isMultilib = true })

         table.insert(compilerList, release)
         table.insert(compilerList, debug_)
         table.insert(compilerList, debugWithAsan)
      end

      if C_FileSystem.isExecutable("/usr/bin/clang") then
         local release, debug_, debugWithAsan = generateConfig(
         nameMap.multilibClang[lang] or nameMap.multilibClang.en_US, lang,
         "/usr/bin/clang", "/usr/bin/clang++",
         { isClang = true, isMultilib = true })

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
      (nameMap.crossGcc[lang] or nameMap.crossGcc.en_US) .. " aarch64", lang,
      "/usr/bin/aarch64-linux-gnu-gcc", "/usr/bin/aarch64-linux-gnu-g++",
      {})

      table.insert(compilerList, release)
   end


   if (
      arch == "x86_64" and (
      C_FileSystem.exists("/proc/sys/fs/binfmt_misc/DOSWin") or
      C_FileSystem.exists("/proc/sys/fs/binfmt_misc/WSLInterop"))) then


      if C_FileSystem.isExecutable("/usr/bin/x86_64-w64-mingw32-gcc") then
         local extraObjects = {
            utf8init = libexecDir .. "/x86_64-w64-mingw32/utf8init.o",
            utf8manifest = libexecDir .. "/x86_64-w64-mingw32/utf8manifest.o",
         }

         do
            local release, _, _ = generateConfig(
            (nameMap.mingwGcc[lang] or nameMap.mingwGcc.en_US) .. " x86_64", lang,
            "/usr/bin/x86_64-w64-mingw32-gcc", "/usr/bin/x86_64-w64-mingw32-g++",
            {
               isMingw = true,
               triplet = "x86_64-w64-mingw32",
               customLinkParams = { extraObjects.utf8init, extraObjects.utf8manifest },
            })

            table.insert(compilerList, release)
         end


         if C_FileSystem.isExecutable("/usr/bin/clang") then
            local release, _, _ = generateConfig(
            (nameMap.mingwClang[lang] or nameMap.mingwClang.en_US) .. " x86_64", lang,
            "/usr/bin/clang", "/usr/bin/clang++",
            {
               isClang = true,
               isMingw = true,
               triplet = "x86_64-w64-mingw32",
               customCompileParams = { "-target", "x86_64-w64-mingw32" },
               customLinkParams = {
                  "-target", "x86_64-w64-mingw32",
                  extraObjects.utf8init, extraObjects.utf8manifest,
                  "-lstdc++", "-lwinpthread",
               },
            })

            table.insert(compilerList, release)
         end
      end

      if C_FileSystem.isExecutable("/usr/bin/i686-w64-mingw32-gcc") then
         local extraObjects = {
            utf8init = libexecDir .. "/i686-w64-mingw32/utf8init.o",
            utf8manifest = libexecDir .. "/i686-w64-mingw32/utf8manifest.o",
         }

         do
            local release, _, _ = generateConfig(
            (nameMap.mingwGcc[lang] or nameMap.mingwGcc.en_US) .. " i686", lang,
            "/usr/bin/i686-w64-mingw32-gcc", "/usr/bin/i686-w64-mingw32-g++",
            {
               isMingw = true,
               triplet = "i686-w64-mingw32",
               customLinkParams = { extraObjects.utf8init, extraObjects.utf8manifest },
            })

            table.insert(compilerList, release)
         end


         if C_FileSystem.isExecutable("/usr/bin/clang") then
            local release, _, _ = generateConfig(
            (nameMap.mingwClang[lang] or nameMap.mingwClang.en_US) .. " i686", lang,
            "/usr/bin/clang", "/usr/bin/clang++",
            {
               isClang = true,
               isMingw = true,
               triplet = "i686-w64-mingw32",
               customCompileParams = { "-target", "i686-w64-mingw32" },
               customLinkParams = {
                  "-target", "i686-w64-mingw32",
                  extraObjects.utf8init, extraObjects.utf8manifest,
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
