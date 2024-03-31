function apiVersion()
   return {
      kind = "theme",
      major = 0,
      minor = 1,
   }
end

local nameMap = {
   en_US = "Light",
   pt_BR = "Clara",
   zh_CN = "浅色",
   zh_TW = "淺色",
}

function main()
   local lang = C_Desktop.language()

   return {
      ["name"] = nameMap[lang] or nameMap.en_US,
      ["style"] = "RedPandaLightFusion",
      ["default scheme"] = "Intellij Classic",
      ["default iconset"] = "newlook",
      ["palette"] = {
         PaletteWindow = "#efefef",
         PaletteWindowText = "#000000",
         PaletteBase = "#ffffff",
         PaletteAlternateBase = "#f7f7f7",
         PaletteToolTipBase = "#ffffdc",
         PaletteToolTipText = "#000000",
         PaletteText = "#000000",
         PaletteButton = "#efefef",
         PaletteButtonText = "#000000",
         PaletteBrightText = "#ffffff",
         PaletteLink = "#0000ff",
         PaletteLinkVisited = "#ff00ff",
         PaletteLight = "#ffffff",
         PaletteMidlight = "#cacaca",
         PaletteDark = "#9f9f9f",
         PaletteMid = "#b8b8b8",
         PaletteWindowDisabled = "#efefef",
         PaletteWindowTextDisabled = "#bebebe",
         PaletteBaseDisabled = "#efefef",
         PaletteTextDisabled = "#bebebe",
         PaletteButtonDisabled = "#efefef",
         PaletteButtonTextDisabled = "#bebebe",
         PaletteHighlight = "#688DB2",
         PaletteHighlightedText = "#000000",
      },
   }
end
