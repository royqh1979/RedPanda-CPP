function apiVersion()
   return {
      kind = "theme",
      major = 0,
      minor = 1,
   }
end

local nameMap = {
   en_US = "One Dark",
   pt_BR = "One Dark",
   zh_CN = "One Dark",
   zh_TW = "One Dark",
}

function main()
   local lang = C_Desktop.language()

   return {
      ["name"] = nameMap[lang] or nameMap.en_US,
      ["style"] = "RedPandaDarkFusion",
      ["default scheme"] = "One Dark",
      ["default iconset"] = "newlook",
      ["palette"] = {
         PaletteWindow = "#282c34",
         PaletteWindowDisabled = "#21252B",
         PaletteWindowText = "#9da5b4",
         PaletteWindowTextDisabled = "#676D7B",
         PaletteBase = "#21252B",
         PaletteBaseDisabled = "#21252B",
         PaletteAlternateBase = "#282c34",
         PaletteToolTipBase = "#21252B",
         PaletteToolTipText = "#9da5b4",
         PaletteText = "#b4b4b4",
         PaletteTextDisabled = "#676D7B",
         PaletteButton = "#282c34",
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
         PaletteMidLight = "#282c34",
         PaletteLight = "#282c34",
      },
   }
end
