function rgbFromString(color)
    local r, g, b = color:match("#(%x%x)(%x%x)(%x%x)")
    return {tonumber(r, 16) / 255, tonumber(g, 16) / 255, tonumber(b, 16) / 255}
end

function rgbToString(rgb)
    return string.format("#%02x%02x%02x",
        math.floor(rgb[1] * 255), math.floor(rgb[2] * 255), math.floor(rgb[3] * 255)
    )
end

function hsvToRgb(h, s, v)
    local r, g, b
    local i = math.floor(h * 6)
    local f = h * 6 - i
    local p = v * (1 - s)
    local q = v * (1 - f * s)
    local t = v * (1 - (1 - f) * s)
    i = i % 6
    if i == 0 then
        r, g, b = v, t, p
    elseif i == 1 then
        r, g, b = q, v, p
    elseif i == 2 then
        r, g, b = p, v, t
    elseif i == 3 then
        r, g, b = p, q, v
    elseif i == 4 then
        r, g, b = t, p, v
    elseif i == 5 then
        r, g, b = v, p, q
    end
    return {r, g, b}
end

function blend(lower, upper, alpha)
    local r = (1 - alpha) * lower[1] + alpha * upper[1]
    local g = (1 - alpha) * lower[2] + alpha * upper[2]
    local b = (1 - alpha) * lower[3] + alpha * upper[3]
    return {r, g, b}
end

local hue = math.random()
local upperColor = hsvToRgb(hue, 0.6, 1)

function transform(color)
    local lowerColor = rgbFromString(color)
    local blended = blend(lowerColor, upperColor, 0.1)
    return rgbToString(blended)
end

function transformPalette(palette)
    local transformed = {}
    for key, value in pairs(palette) do
        transformed[key] = transform(value)
    end
    return transformed
end

local originalPalette = {
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
    PaletteMidLight = "#cacaca",
    PaletteDark = "#9f9f9f",
    PaletteMid = "#b8b8b8",
    PaletteWindowDisabled = "#efefef",
    PaletteWindowTextDisabled = "#bebebe",
    PaletteBaseDisabled = "#efefef",
    PaletteTextDisabled = "#bebebe",
    PaletteButtonDisabled = "#efefef",
    PaletteButtonTextDisabled = "#bebebe",
    PaletteHighlight = "#dddddd",
    PaletteHighlightedText = "#000000"
}

local nameMap = {
    en_US = "Random Light",
    pt_BR = "Clara aleatória",
    zh_CN = "随机浅色",
    zh_TW = "隨機淺色"
}

local lang = C_Desktop.language()

return {
    ["name"] = nameMap[lang] or nameMap.en_US,
    ["style"] = "RedPandaLightFusion",
    ["default scheme"] = "Adaptive",
    ["default iconset"] = "newlook",
    ["palette"] = transformPalette(originalPalette)
}
