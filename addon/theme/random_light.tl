global function apiVersion(): ApiVersion
    return {
        kind = "theme",
        major = 0,
        minor = 1,
    }
end

local record Rgb
    r: number
    g: number
    b: number
end

local record Hsv
    h: number
    s: number
    v: number
end

local function rgbFromString(color: string): Rgb
    local r, g, b = color:match("#(%x%x)(%x%x)(%x%x)")
    return {
        r = tonumber(r, 16) / 255,
        g = tonumber(g, 16) / 255,
        b = tonumber(b, 16) / 255,
    }
end

local function rgbToString(rgb: Rgb): string
    return string.format(
        "#%02x%02x%02x",
        math.floor(rgb.r * 255),
        math.floor(rgb.g * 255),
        math.floor(rgb.b * 255)
    )
end

local function hsvToRgb(hsv: Hsv): Rgb
    local r, g, b: number, number, number
    local h, s, v = hsv.h, hsv.s, hsv.v
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
    return {r = r, g = g, b = b}
end

local function blend(lower: Rgb, upper: Rgb, alpha: number): Rgb
    local r = (1 - alpha) * lower.r + alpha * upper.r
    local g = (1 - alpha) * lower.g + alpha * upper.g
    local b = (1 - alpha) * lower.b + alpha * upper.b
    return {r = r, g = g, b = b}
end

local function transform(color: string, upperColor: Rgb): string
    local lowerColor = rgbFromString(color)
    local blended = blend(lowerColor, upperColor, 0.1)
    return rgbToString(blended)
end

local function transformPalette(palette: Theme.Palette, upperColor: Rgb): Theme.Palette
    local transformed = {}
    for key, value in pairs(palette as {string:string}) do
        transformed[key] = transform(value, upperColor)
    end
    return transformed as Theme.Palette
end

local originalPalette: Theme.Palette = {
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
    PaletteHighlight = "#dddddd",
    PaletteHighlightedText = "#000000",
}

local nameMap: {string:string} = {
    en_US = "Random Light",
    pt_BR = "Clara aleatória",
    zh_CN = "随机浅色",
    zh_TW = "隨機淺色",
}

global function main(): Theme
    local hue = math.random()
    local upperColor = hsvToRgb({h = hue, s = 0.6, v = 1})

    local lang = C_Desktop.language()

    return {
        ["name"] = nameMap[lang] or nameMap.en_US,
        ["style"] = "RedPandaLightFusion",
        ["default scheme"] = "Adaptive",
        ["default iconset"] = "newlook",
        ["palette"] = transformPalette(originalPalette, upperColor),
    }
end
