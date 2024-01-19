target("redpanda-win-git-askpass")
    set_kind("binary")

    add_files("main.cpp")

    add_files(
        -- "redpanda-git-askpass_private.rc",  -- drives xmake crazy
        "resource.rc")

    add_links("user32")
