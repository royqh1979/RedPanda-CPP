/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "utils/font.h"

#include <QFont>
#include <QVersionNumber>

QString defaultUiFont()
{
    QString defaultLocaleName = QLocale::system().name();
#if defined(Q_OS_WIN)
    QVersionNumber currentVersion = QVersionNumber::fromString(QSysInfo::kernelVersion());
    const QVersionNumber win10(10, 0);
    const QVersionNumber win8(6, 2);
    const QVersionNumber vista(6, 0);
    if (defaultLocaleName == "zh_CN") {
        if (currentVersion >= win8)
            return "Microsoft YaHei UI";
        else if (currentVersion >= vista)
            return "Microsoft YaHei";
        else
            // SimSun is terrible for Latin letters
            return "Tahoma";
    } else if (defaultLocaleName == "zh_TW" || defaultLocaleName == "zh_HK") {
        if (currentVersion >= win8)
            return "Microsoft JhengHei UI";
        else if (currentVersion >= vista)
            return "Microsoft JhengHei";
        else
            return "PMingLiU";
    } else if (defaultLocaleName == "ja_JP") {
        if (currentVersion >= win10)
            return "Yu Gothic UI";
        else if (currentVersion >= vista)
            return "Meiryo UI";
        else
            return "MS UI Gothic";
    } else if (defaultLocaleName == "ko_KR") {
        if (currentVersion >= vista)
            return "Malgun Gothic";
        else
            return "Gulim";
    } else {
        if (currentVersion >= vista)
            return "Segoe UI";
        else
            return "Tahoma";
    }
#elif defined(Q_OS_MACOS)
    if (defaultLocaleName == "zh_CN")
        return "PingFang SC";
    else if (defaultLocaleName == "zh_TW")
        return "PingFang TC";
    else if (defaultLocaleName == "zh_HK")
        return "PingFang HK";
    else if (defaultLocaleName == "ja_JP")
        return "Hiragino Sans";
    else if (defaultLocaleName == "ko_KR")
        return "Apple SD Gothic Neo";
    else
        return "Helvetica Neue";
#else // XDG desktop
    if (defaultLocaleName == "zh_CN")
        return "Noto Sans CJK SC";
    else if (defaultLocaleName == "zh_TW")
        return "Noto Sans CJK TC";
    else if (defaultLocaleName == "zh_HK")
        return "Noto Sans CJK HK";
    else if (defaultLocaleName == "ja_JP")
        return "Noto Sans CJK JP";
    else if (defaultLocaleName == "ko_KR")
        return "Noto Sans CJK KR";
    else
        return "Noto Sans";
#endif
}

QString defaultMonoFont()
{
    QString defaultLocaleName = QLocale::system().name();
#if defined(Q_OS_WIN)
    QVersionNumber currentVersion = QVersionNumber::fromString(QSysInfo::kernelVersion());
    const QVersionNumber vista(6, 0);
    if (currentVersion >= vista)
        return "Consolas";
    else
        return "Courier New";
#elif defined(Q_OS_MACOS)
    return "Menlo";
#else // XDG desktop
    QFont font("Noto Sans Mono");
    if (font.exactMatch())
        return "Noto Sans Mono";
    else
        return "DejaVu Sans Mono";
#endif
}

QString defaultEmojiFont()
{
#if defined(Q_OS_WIN)
    return "Segoe UI Emoji";
#elif defined(Q_OS_MACOS)
    return "Apple Color Emoji";
#else // XDG desktop
    return "Noto Color Emoji";
#endif
}

bool isCjk(const QString &locale)
{
    return locale.startsWith("zh_") || locale == "zh" ||
           locale.startsWith("ja_") || locale == "ja" ||
           locale.startsWith("ko_") || locale == "ko";
}

QStringList defaultCjkEditorFonts(const QString &locale)
{
#if defined(Q_OS_WIN)
    QVersionNumber currentVersion = QVersionNumber::fromString(QSysInfo::kernelVersion());
    const QVersionNumber vista(6, 0);
    if (locale == "zh_TW" || locale == "zh_HK"){
        if (currentVersion >= vista)
            return {"Microsoft JhengHei"};
        else
            return {"SimHei"};
    } else if (locale == "ja_JP") {
        if (currentVersion >= vista)
            // prefer Meiryo over Yu Gothic, and YaHei over JhengHei
            // they are bolder and more readable
            return {"Meiryo", "Microsoft YaHei"};
        else
            return {"MS Gothic", "SimHei"};
    } else if (locale == "ko_KR") {
        if (currentVersion >= vista)
            return {"Malgun Gothic", "Microsoft YaHei"};
        else
            return {"Dotum", "SimHei"};
    } else {
        // finally fallback to zh_CN
        // with largest coverage, CJK ideographs have uniform look
        if (currentVersion >= vista)
            return {"Microsoft YaHei"};
        else
            return {"SimHei"};
    }
#elif defined(Q_OS_MACOS)
    // TODO: coverage is not verified
    if (locale == "zh_TW")
        return {"PingFang TC"};
    else if (locale == "zh_HK")
        return {"PingFang HK"};
    else if (locale == "ja_JP")
        // prefer Hiragino Sans GB for uniform look of CJK ideographs
        return {"Hiragino Sans", "Hiragino Sans GB"};
    else if (locale == "ko_KR")
        return {"Apple SD Gothic Neo", "PingFang SC"};
    else
        return {"PingFang SC"};
#else // XDG desktop
    // Noto Sans CJK have same coverage, add one of them is enough
    // intentionally: the "Mono" variant is not strictly monospaced either, and has less weights
    if (locale == "zh_TW")
        return {"Noto Sans CJK TC"};
    else if (locale == "zh_HK")
        return {"Noto Sans CJK HK"};
    else if (locale == "ja_JP")
        return {"Noto Sans CJK JP"};
    else if (locale == "ko_KR")
        return {"Noto Sans CJK KR"};
    else
        return {"Noto Sans CJK SC"};
#endif
}

QStringList defaultFallbackEditorFonts(UnicodeSupportLevel level)
{
#if defined(Q_OS_WIN)
    // Use system fallback fonts
    return {};
#elif defined(Q_OS_MACOS)
    // Use system fallback fonts
    return {};
#else // XDG desktop
    static QStringList ltrSimple = {
        "Noto Sans Armenian",
        "Noto Sans Bamum",
        "Noto Sans Bassa Vah",
        "Noto Sans Buginese",
        "Noto Sans Buhid",
        "Noto Sans Canadian Aboriginal",
        "Noto Sans Cherokee",
        "Noto Sans Coptic",
        "Noto Sans Georgian",
        "Noto Sans Kayah Li",
        "Noto Sans Lisu",
        "Noto Sans Math",
        "Noto Sans Miao",
        "Noto Sans Mro",
        "Noto Sans Nag Mundari",
        "Noto Serif Nyiakeng Puachue Hmong",
        "Noto Sans Ol Chiki",
        "Noto Sans Osage",
        "Noto Sans Pahawh Hmong",
        "Noto Sans Pau Cin Hau",
        "Noto Sans Rejang",
        "Noto Sans Saurashtra",
        "Noto Sans Sora Sompeng",
        "Noto Sans Symbols",
        "Noto Sans Symbols2",
        "Noto Sans Tagbanwa",
        "Noto Sans Tai Le",
        "Noto Sans Tai Viet",
        "Noto Sans Tangsa",
        "Noto Serif Toto",
        "Noto Sans Vai",
        "Noto Sans Wancho",
        "Noto Serif Yezidi",
        "Noto Sans Yi",
    };

    static QStringList ltrContextual = {
        "Noto Sans Balinese",
        "Noto Sans Batak",
        "Noto Sans Bengali",
        "Noto Sans Chakma",
        "Noto Sans Cham",
        "Noto Sans Devanagari",
        "Noto Sans Duployan",
        "Noto Sans Ethiopic",
        "Noto Sans Grantha",
        "Noto Sans Gujarati",
        "Noto Sans Gunjala Gondi",
        "Noto Sans Gurmukhi",
        "Noto Sans Hanunoo", // actually bt-lr
        "Noto Sans Javanese",
        "Noto Sans Kannada",
        "Noto Sans Khmer",
        "Noto Sans Khojki",
        "Noto Sans Lao",
        "Noto Sans Lepcha",
        "Noto Sans Limbu",
        "Noto Sans Malayalam",
        "Noto Sans Masaram Gondi",
        "Noto Sans Medefaidrin",
        "Noto Sans MeeteiMayek",
        "Noto Sans Mongolian", // actually tb-lr
        "Noto Sans Myanmar",
        "Noto Sans New Tai Lue",
        "Noto Sans Newa",
        "Noto Sans Oriya",
        "Noto Sans Sharada",
        "Noto Sans Sinhala",
        "Noto Sans Sundanese",
        "Noto Sans Syloti Nagri",
        "Noto Sans Tai Tham",
        "Noto Sans Tamil",
        "Noto Sans Tamil Supplement", // not complex, but to be combined with Tamil
        "Noto Sans Telugu",
        "Noto Sans Thai",
        "Noto Serif Tibetan",
        "Noto Sans Tifinagh",
        "Noto Sans Tirhuta",
        "Noto Sans Warang Citi",
    };

    static QStringList rtl = {
        "Noto Sans Adlam",
        "Noto Sans Arabic",
        "Noto Sans Hanifi Rohingya",
        "Noto Sans Hebrew",
        "Noto Sans Mende Kikakui",
        "Noto Sans NKo",
        "Noto Sans Samaritan",
        "Noto Sans Syriac",
        "Noto Sans Thaana",
    };

    static QStringList historical [[maybe_unused]] = {
        "Ahom",
        "Anatolian Hieroglyphs",
        "Avestan",
        "Bhaiksuki",
        "Brahmi",
        "Carian",
        "Caucasian Albanian",
        "Chorasmian",
        "Cuneiform",
        "Cypriot",
        "Cypro Minoan",
        "Deseret",
        "Dives Akuru",
        "Dogra",
        "Egyptian Hieroglyphs",
        "Elbasan",
        "Elymaic",
        "Glagolitic",
        "Gothic",
        "Hatran",
        "Imperial Aramaic",
        "Indic Siyaq Numbers",
        "Inscriptional Pahlavi",
        "Inscriptional Parthian",
        "Kaithi",
        "Kawi",
        "Kharoshthi",
        "Khitan Small Script",
        "Khudawadi",
        "Linear A",
        "Linear B",
        "Lycian",
        "Lydian",
        "Mahajani",
        "Makasar",
        "Mandaic",
        "Manichaean",
        "Marchen",
        "Mayan Numerals",
        "Meroitic",
        "Modi",
        "Multani",
        "Nabataean",
        "Nandinagari",
        "Nushu",
        "Ogham",
        "Old Hungarian",
        "Old Italic",
        "Old North Arabian",
        "Old Permic",
        "Old Persian",
        "Old Sogdian",
        "Old South Arabian",
        "Old Turkic",
        "Old Uyghur",
        "Osmanya",
        "Ottoman Siyaq Numbers",
        "Palmyrene",
        "Phags Pa",
        "Phoenician",
        "Psalter Pahlavi",
        "Runic",
        "Shavian",
        "Siddham",
        "Sogdian",
        "Soyombo",
        "Tagalog",
        "Takri",
        "Tangut",
        "Ugaritic",
        "Vithkuqi",
        "Zanabazar Square",
    };

    static QStringList notApplicable [[maybe_unused]] = {
        "Music", // domain-specific
        "Nastaliq", // stylized Arabic
        "SignWriting", // domain-specific
    };

    QStringList result = ltrSimple;
    if (level >= UnicodeSupportLevel::Contextual)
        result += ltrContextual;
    if (level >= UnicodeSupportLevel::Bidirectional)
        result += rtl;
    return result;
#endif
}

QStringList defaultEditorFonts(UnicodeSupportLevel level)
{
    QStringList result = {
        defaultMonoFont(),
    };

    // special handling CJK fonts: they share same code points
    QString defaultLocaleName = QLocale::system().name();
    if (isCjk(defaultLocaleName)) {
        result += defaultCjkEditorFonts(defaultLocaleName);
        result += defaultFallbackEditorFonts(level);
    } else {
        result += defaultFallbackEditorFonts(level);
        result += defaultCjkEditorFonts(defaultLocaleName);
    }

    result += {
        defaultEmojiFont(),
        "Red Panda Control",
    };
    return result;
}
