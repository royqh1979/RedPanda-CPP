global function apiVersion(): ApiVersion
    return {
         kind = "theme",
         major = 0,
         minor = 1,
    }
end

global function main(): Theme
    return {
        ["name"] = "One Dark",  -- do not translate
        ["style"] = "RedPandaDarkFusion",
        ["default scheme"] = "One Dark",
        ["default iconset"] = "contrast",
        ["palette"] = {
            PaletteWindow = "#282c34",
            PaletteWindowDisabled = "#21252B",
            PaletteWindowText = "#9da5b4",
            PaletteWindowTextDisabled = "#676D7B",
            PaletteBase = "#21252B",
            PaletteBaseDisabled = "#1d2026",
            PaletteAlternateBase = "#282c34",
            PaletteToolTipBase = "#21252B",
            PaletteToolTipText = "#9da5b4",
            PaletteText = "#b4b4b4",
            PaletteTextDisabled = "#676D7B",
            PaletteButton = "#2c313c",
            PaletteButtonDisabled = "#21252B",
            PaletteButtonText = "#9da5b4",
            PaletteButtonTextDisabled = "#676D7B",
            PaletteHighlight = "#3b4048",
            PaletteHighlightDisabled = "#21252B",
            PaletteHighlightedText = "#d7dae0",
            PaletteHighlightedTextDisabled = "#676D7B",
            PaletteLink = "#98c379",
            PaletteLinkVisited = "#c9c900",
            PaletteBrightText = "#E0E1E3",
            PaletteDark = "#282c34",
            PaletteMid = "#282c34",
            PaletteMidlight = "#282c34",
            PaletteLight = "#282c34",
        },
    }
end
