function apiVersion()
   return {
      kind = "theme",
      major = 0,
      minor = 1,
   }
end

local nameMap = {
   en_US = "Contrast",
   pt_BR = "Contraste",
   zh_CN = "高对比度",
   zh_TW = "高對比度",
}

function main()
   local lang = C_Desktop.language()

   return {
      ["name"] = nameMap[lang] or nameMap.en_US,
      ["style"] = "RedPandaDarkFusion",
      ["default scheme"] = "Twilight",
      ["default iconset"] = "contrast",
      ["palette"] = {
         PaletteWindow = "#000000",
         PaletteWindowText = "#FFFFFF",
         PaletteBase = "#141414",
         PaletteAlternateBase = "#191919",
         PaletteButton = "#141414",
         PaletteButtonDisabled = "#141414",
         PaletteBrightText = "#ff0000",
         PaletteText = "#FFFFFF",
         PaletteButtonText = "#FFFFFF",
         PaletteButtonTextDisabled = "#9DA9B5",
         PaletteHighlight = "#aa1f75cc",
         PaletteDark = "#232323",
         PaletteHighlightedText = "#e7e7e7",
         PaletteToolTipBase = "#66000000",
         PaletteToolTipText = "#e7e7e7",
         PaletteLink = "#007af4",
         PaletteLinkVisited = "#a57aff",
         PaletteWindowDisabled = "#0A0A0A",
         PaletteWindowTextDisabled = "#9DA9B5",
         PaletteHighlightDisabled = "#9DA9B5",
         PaletteHighlightedTextDisabled = "#9DA9B5",
         PaletteBaseDisabled = "#000000",
         PaletteTextDisabled = "#9DA9B5",
         PaletteMid = "#FFFFFF",
         PaletteLight = "#505050",
         PaletteMidlight = "#00ff00",
      },
   }
end
