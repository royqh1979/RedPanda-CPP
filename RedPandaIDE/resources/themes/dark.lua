function apiVersion()
   return {
      kind = "theme",
      major = 0,
      minor = 1,
   }
end

local nameMap = {
   en_US = "Dark",
   pt_BR = "Escura",
   zh_CN = "深色",
   zh_TW = "深色",
}

function main()
   local lang = C_Desktop.language()

   return {
      ["name"] = nameMap[lang] or nameMap.en_US,
      ["style"] = "RedPandaDarkFusion",
      ["default scheme"] = "VS Code",
      ["default iconset"] = "contrast",
      ["palette"] = {
         PaletteWindow = "#19232D",
         PaletteWindowText = "#E0E1E3",
         PaletteBase = "#1E1E1E",
         PaletteAlternateBase = "#303030",
         PaletteButton = "#19232D",
         PaletteButtonDisabled = "#19232D",
         PaletteBrightText = "#ff0000",
         PaletteText = "#e7e7e7",
         PaletteButtonText = "#d3d3d3",
         PaletteButtonTextDisabled = "#9DA9B5",
         PaletteHighlight = "#aa1f75cc",
         PaletteDark = "#232323",
         PaletteHighlightedText = "#e7e7e7",
         PaletteToolTipBase = "#66000000",
         PaletteToolTipText = "#e7e7e7",
         PaletteLink = "#007af4",
         PaletteLinkVisited = "#a57aff",
         PaletteWindowDisabled = "#333333",
         PaletteWindowTextDisabled = "#9DA9B5",
         PaletteHighlightDisabled = "#26486B",
         PaletteHighlightedTextDisabled = "#9DA9B5",
         PaletteBaseDisabled = "#19232D",
         PaletteTextDisabled = "#9DA9B5",
         PaletteMid = "#707070",
         PaletteLight = "#505050",
         PaletteMidlight = "#00ff00",
      },
   }
end
