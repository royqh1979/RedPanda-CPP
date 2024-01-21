local env = require("defs.global_env")

global C_Debug = env.C_Debug
global C_Desktop = env.C_Desktop
global C_Util = env.C_Util

global record Theme
    name: string
    style: string
    ["default scheme"]: string
    ["default iconset"]: BuiltInIconSet
    palette: Palette

    enum BuiltInIconSet
        "newlook"
        "contrast"
        "bluesky"
    end

    record Palette
        PaletteWindow: string | nil
        PaletteWindowText: string | nil
        PaletteBase: string | nil
        PaletteAlternateBase: string | nil
        PaletteToolTipBase: string | nil
        PaletteToolTipText: string | nil
        PaletteText: string | nil
        PaletteButton: string | nil
        PaletteButtonText: string | nil
        PaletteBrightText: string | nil
        PaletteHighlight: string | nil
        PaletteHighlightedText: string | nil
        PaletteLink: string | nil
        PaletteLinkVisited: string | nil

        PaletteLight: string | nil
        PaletteMidlight: string | nil
        PaletteDark: string | nil
        PaletteMid: string | nil
        PaletteShadow: string | nil

        PaletteWindowDisabled: string | nil
        PaletteWindowTextDisabled: string | nil
        PaletteBaseDisabled: string | nil
        PaletteAlternateBaseDisabled: string | nil
        PaletteToolTipBaseDisabled: string | nil
        PaletteToolTipTextDisabled: string | nil
        PaletteTextDisabled: string | nil
        PaletteButtonDisabled: string | nil
        PaletteButtonTextDisabled: string | nil
        PaletteBrightTextDisabled: string | nil
        PaletteHighlightDisabled: string | nil
        PaletteHighlightedTextDisabled: string | nil
        PaletteLinkDisabled: string | nil
        PaletteLinkVisitedDisabled: string | nil

        PaletteLightDisabled: string | nil
        PaletteMidlightDisabled: string | nil
        PaletteDarkDisabled: string | nil
        PaletteMidDisabled: string | nil
        PaletteShadowDisabled: string | nil
    end
end
