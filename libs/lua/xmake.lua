target("lua")
    set_kind("static")

    if is_os("macosx") then
        add_defines("LUA_USE_MACOSX")
    elseif is_os("linux", "bsd") then
        add_defines("LUA_USE_LINUX")  -- BSDs are Linux in Lua source
    end

    add_files(
        "lua/lapi.c",
        "lua/lcode.c",
        "lua/lctype.c",
        "lua/ldebug.c",
        "lua/ldo.c",
        "lua/ldump.c",
        "lua/lfunc.c",
        "lua/lgc.c",
        "lua/llex.c",
        "lua/lmem.c",
        "lua/lobject.c",
        "lua/lopcodes.c",
        "lua/lparser.c",
        "lua/lstate.c",
        "lua/lstring.c",
        "lua/ltable.c",
        "lua/ltm.c",
        "lua/lundump.c",
        "lua/lvm.c",
        "lua/lzio.c")
    add_files(
        "lua/lauxlib.c",
        "lua/lbaselib.c",
        "lua/lcorolib.c",
        "lua/ldblib.c",
        "lua/liolib.c",
        "lua/lmathlib.c",
        "lua/loadlib.c",
        "lua/loslib.c",
        "lua/lstrlib.c",
        "lua/ltablib.c",
        "lua/lutf8lib.c",
        "lua/linit.c")

    add_includedirs(".", {public = true})
    add_headerfiles("lua/lua.h", "lua/lua.hpp")

    -- do not install
    on_install(function (target) end)
