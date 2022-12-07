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
#include "HighlighterManager.h"
#include <QFileInfo>
#include <QObject>
#include "qsynedit/Constants.h"
#include "qsynedit/highlighter/cpp.h"
#include "qsynedit/highlighter/asm.h"
#include "qsynedit/highlighter/glsl.h"
#include "qsynedit/highlighter/makefilehighlighter.h"

#include "qsynedit/Constants.h"
#include "colorscheme.h"

HighlighterManager highlighterManager;

HighlighterManager::HighlighterManager()
{

}

QSynedit::PHighlighter HighlighterManager::getHighlighter(QSynedit::HighlighterLanguage language)
{
    switch(language) {
    case QSynedit::HighlighterLanguage::Cpp:
        return getCppHighlighter();
    case QSynedit::HighlighterLanguage::Asssembly:
        return getAsmHighlighter();
    case QSynedit::HighlighterLanguage::Makefile:
        return getMakefileHighlighter();
    case QSynedit::HighlighterLanguage::GLSL:
        return getGLSLHighlighter();
    default:
        return QSynedit::PHighlighter();
    }
}

QSynedit::PHighlighter HighlighterManager::getHighlighter(const QString &filename)
{
    QFileInfo info(filename);
    QString suffix = info.suffix();
    QString basename = info.baseName();
    if (suffix == "c" || suffix == "cpp" || suffix == "cxx"
            || suffix == "cc" || suffix == "h" || suffix == "hpp"
            || suffix == "hxx" || suffix == "hh" || suffix == "C"
            || suffix == "CPP" || suffix =="H" || suffix == "c++"
            || suffix == "h++") {
        return getCppHighlighter();
    } else if (suffix == "vs" || suffix == "fs" || suffix == "frag") {
        return getGLSLHighlighter();
    } else if (suffix == "s" || suffix == "asm") {
        return getAsmHighlighter();
    } else if (basename.compare("makefile", Qt::CaseInsensitive)==0) {
        return getMakefileHighlighter();
    } else if (suffix.isEmpty()) {
        return getCppHighlighter();
    }
    return QSynedit::PHighlighter();
}

QSynedit::PHighlighter HighlighterManager::copyHighlighter(QSynedit::PHighlighter highlighter)
{
    if (!highlighter)
        return QSynedit::PHighlighter();
    return getHighlighter(highlighter->language());
}

QSynedit::PHighlighter HighlighterManager::getCppHighlighter()
{
    std::shared_ptr<QSynedit::CppHighlighter> highlighter = std::make_shared<QSynedit::CppHighlighter>();
    return highlighter;
}

QSynedit::PHighlighter HighlighterManager::getAsmHighlighter()
{
    std::shared_ptr<QSynedit::ASMHighlighter> highlighter=std::make_shared<QSynedit::ASMHighlighter>();
    return highlighter;
}

QSynedit::PHighlighter HighlighterManager::getGLSLHighlighter()
{
    std::shared_ptr<QSynedit::GLSLHighlighter> highlighter=std::make_shared<QSynedit::GLSLHighlighter>();
    return highlighter;
}

QSynedit::PHighlighter HighlighterManager::getMakefileHighlighter()
{
    std::shared_ptr<QSynedit::MakefileHighlighter> highlighter=std::make_shared<QSynedit::MakefileHighlighter>();
    return highlighter;
}

void HighlighterManager::applyColorScheme(QSynedit::PHighlighter highlighter, const QString &schemeName)
{
    if (!highlighter)
        return;

    for (QString name: highlighter->attributes().keys()) {
        PColorSchemeItem item = pColorManager->getItem(schemeName,name);
        if (item) {
            QSynedit::PHighlighterAttribute attr = highlighter->attributes()[name];
            attr->setBackground(item->background());
            attr->setForeground(item->foreground());
            QSynedit::FontStyles styles = QSynedit::FontStyle::fsNone;
            styles.setFlag(QSynedit::FontStyle::fsBold, item->bold());
            styles.setFlag(QSynedit::FontStyle::fsItalic, item->italic());
            styles.setFlag(QSynedit::FontStyle::fsUnderline, item->underlined());
            styles.setFlag(QSynedit::FontStyle::fsStrikeOut, item->strikeout());
            attr->setStyles(styles);
        }
    }
}
