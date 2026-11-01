#!/usr/bin/env python3
"""Generate the tiny test fonts for qsynedit's unit tests.

qsynedit-test-mono.ttf (family "QSynedit Test Mono"):
    ASCII printable chars are exactly half-width (512 units),
    CJK and emoji are exactly full-width (1024 units),
    ZWJ/ZWNJ/variation selectors are zero-width,
    so that glyph metrics are fully deterministic across environments.

qsynedit-test-prop.ttf (family "QSynedit Test Prop"):
    same codepoints, but every char gets its own deterministic
    pseudo-varied width, to cover the forceMonospace=false path.

Glyph outlines are plain rectangles; the tests measure metrics,
they never compare rendered pixels.

Requires: pip install fonttools
Run from this directory: python gen_test_font.py
"""

import os
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

UNITS_PER_EM = 1024
HALF_WIDTH = 512
FULL_WIDTH = 1024

CJK_CODEPOINTS = [ord(c) for c in "中文测试字體繁體國语漢字"]

ZERO_WIDTH_CODEPOINTS = [0x200B, 0x200C, 0x200D,  # ZWSP, ZWNJ, ZWJ
                         *range(0xFE00, 0xFE10),  # variation selectors 1-16
                         0x202A, 0x202B, 0x202C, 0x202D, 0x202E,  # bidi controls
                         ]

HERE = os.path.dirname(os.path.abspath(__file__))


def emoji_codepoints():
    path = os.path.join(HERE, "..", "resources", "emoji.txt")
    with open(path, encoding="utf-8") as f:
        text = f.read()
    return sorted({ord(c) for c in text if ord(c) > 127})


def rect_glyph(pen, width, height=768):
    if width > 0:
        pen.moveTo((0, 0))
        pen.lineTo((width, 0))
        pen.lineTo((width, height))
        pen.lineTo((0, height))
        pen.closePath()


def glyph_name(cp):
    return "uni%04X" % cp if cp <= 0xFFFF else "u%06X" % cp


def build(path, family, width_of):
    codepoints = sorted(width_of.keys())
    glyph_order = [".notdef"] + [glyph_name(cp) for cp in codepoints]

    glyphs = {}
    pen = TTGlyphPen(None)
    glyphs[".notdef"] = pen.glyph()
    for cp in codepoints:
        pen = TTGlyphPen(None)
        rect_glyph(pen, width_of[cp])
        glyphs[glyph_name(cp)] = pen.glyph()

    fb = FontBuilder(UNITS_PER_EM, isTTF=True)
    fb.setupGlyphOrder(glyph_order)
    fb.setupCharacterMap({cp: glyph_name(cp) for cp in codepoints})
    fb.setupGlyf(glyphs)
    metrics = {".notdef": (HALF_WIDTH, 0)}
    metrics.update({name: (width_of[cp], 0) for name, cp in
                    zip(glyph_order[1:], codepoints)})
    fb.setupHorizontalMetrics(metrics)
    fb.setupHorizontalHeader(ascent=820, descent=204)
    fb.setupNameTable({
        "familyName": family,
        "styleName": "Regular",
        "uniqueFontIdentifier": family,
        "fullName": family,
        "psName": family.replace(" ", ""),
        "version": "Version 1.0",
    })
    fb.setupOS2(sTypoAscender=820, sTypoDescender=-204,
                usWinAscent=820, usWinDescent=204,
                sxHeight=384, sCapHeight=768)
    fb.setupPost(isFixedPitch=1 if "Mono" in family else 0)
    fb.save(path)
    print("wrote %s (%d glyphs)" % (path, len(glyphs)))


def main():
    ascii_cps = list(range(0x20, 0x7F))
    emoji_cps = emoji_codepoints()
    wide_cps = sorted(set(CJK_CODEPOINTS) | set(emoji_cps) - set(ZERO_WIDTH_CODEPOINTS))

    mono_widths = {cp: HALF_WIDTH for cp in ascii_cps}
    mono_widths.update({cp: FULL_WIDTH for cp in wide_cps})
    mono_widths.update({cp: 0 for cp in ZERO_WIDTH_CODEPOINTS})
    build(os.path.join(HERE, "qsynedit-test-mono.ttf"),
          "QSynedit Test Mono", mono_widths)

    # deterministic pseudo-varied widths; intentionally NOT multiples
    # of each other, so no accidental monospaced alignment
    prop_widths = {cp: 260 + (cp * 37) % 380 for cp in ascii_cps}
    prop_widths.update({cp: 640 + (cp * 53) % 300 for cp in wide_cps})
    prop_widths.update({cp: 0 for cp in ZERO_WIDTH_CODEPOINTS})
    build(os.path.join(HERE, "qsynedit-test-prop.ttf"),
          "QSynedit Test Prop", prop_widths)


if __name__ == "__main__":
    main()
