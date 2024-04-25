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
#include <QMap>

#if defined(Q_OS_WIN)

static const QMap<QString, QStringList> uiFontsByLocale = {};

static const QMap<QLocale::Script, QStringList> uiFontsByScript = {
    {QLocale::CyrillicScript, {"Segoe UI", "Tahoma"}},
    {QLocale::SimplifiedHanScript, {"Microsoft YaHei UI", "Microsoft YaHei", "SimHei"}},
    {QLocale::TraditionalHanScript, {"Microsoft JhengHei UI", "Microsoft JhengHei", "SimHei"}},
    {QLocale::LatinScript, {"Segoe UI", "Tahoma"}},
    {QLocale::GreekScript, {"Segoe UI", "Tahoma"}},
    {QLocale::JapaneseScript, {"Yu Gothic UI", "Meiryo UI", "MS UI Gothic"}},
    {QLocale::KoreanScript, {"Malgun Gothic", "Dotum"}},
};

#elif defined(Q_OS_MACOS)

static const QMap<QString, QStringList> uiFontsByLocale = {
    {"zh_HK", {"PingFang HK"}},
};

static const QMap<QLocale::Script, QStringList> uiFontsByScript = {
    {QLocale::CyrillicScript, {"Helvetica Neue"}},
    {QLocale::SimplifiedHanScript, {"PingFang SC"}},
    {QLocale::TraditionalHanScript, {"PingFang TC"}},
    {QLocale::LatinScript, {"Helvetica Neue"}},
    {QLocale::GreekScript, {"Helvetica Neue"}},
    {QLocale::JapaneseScript, {"Hiragino Sans"}},
    {QLocale::KoreanScript, {"Apple SD Gothic Neo"}},
};

#else // XDG desktop

static const QMap<QString, QStringList> uiFontsByLocale = {
    {"zh_HK", {"Noto Sans CJK HK"}},
};

static const QMap<QLocale::Script, QStringList> uiFontsByScript = {
    {QLocale::CyrillicScript, {"Noto Sans"}},
    {QLocale::SimplifiedHanScript, {"Noto Sans CJK SC"}},
    {QLocale::TraditionalHanScript, {"Noto Sans CJK TC"}},
    {QLocale::LatinScript, {"Noto Sans"}},
    {QLocale::GreekScript, {"Noto Sans"}},
    {QLocale::JapaneseScript, {"Noto Sans CJK JP"}},
    {QLocale::KoreanScript, {"Noto Sans CJK KR"}},
};

#endif

QString defaultUiFont()
{
    QString defaultLocaleName = QLocale::system().name();
    QLocale::Script defaultScript = QLocale::system().script();
    if (uiFontsByLocale.contains(defaultLocaleName)) {
        QStringList fonts = uiFontsByLocale[defaultLocaleName];
        for (const QString &font : fonts) {
            if (QFont(font).exactMatch())
                return font;
        }
    }
    if (uiFontsByScript.contains(defaultScript)) {
        QStringList fonts = uiFontsByScript[defaultScript];
        for (const QString &font : fonts) {
            if (QFont(font).exactMatch())
                return font;
        }
    }

    // final fallback
#if defined(Q_OS_WIN)
    return "Microsoft Sans Serif";
#elif defined(Q_OS_MACOS)
    return "Helvetica Neue";
#else // XDG desktop
    return "Sans Serif";
#endif
}

QString defaultMonoFont()
{
#if defined(Q_OS_WIN)
    QFont font("Consolas");
    if (font.exactMatch())
        return "Consolas";
    else
        return "Lucida Console";
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

QStringList defaultFallbackEditorFonts()
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    // use system fallback fonts
    return {};
#else // XDG desktop
    // There is a limit of 256 fallback fonts in Qt:
    // https://bugreports.qt.io/browse/QTBUG-80434
    // As a result, on systems with Noto installed (~1000 fonts), fallback will fail.
    // Here we recommend use merged Noto fonts to reduce the number of required fonts.
    // https://github.com/notofonts/notofonts.github.io/tree/main/megamerge
    return {
        "Noto Sans Living",
        "Noto Sans Historical",
        "Noto Serif Living",
        "Noto Serif Historical",
    };
#endif
}

QStringList defaultEditorFonts()
{
    QStringList result = {
        defaultMonoFont(),
    };

    // special handling CJK fonts: they share same code points
    QString defaultLocaleName = QLocale::system().name();
    if (isCjk(defaultLocaleName)) {
        result += defaultCjkEditorFonts(defaultLocaleName);
        result += defaultFallbackEditorFonts();
    } else {
        result += defaultFallbackEditorFonts();
        result += defaultCjkEditorFonts(defaultLocaleName);
    }

    result.append(defaultEmojiFont());
    return result;
}
