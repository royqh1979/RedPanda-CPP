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
#include "syntaxermanager.h"
#include <QFileInfo>
#include <QObject>
#include "qsynedit/constants.h"
#include "qsynedit/syntaxer/cpp.h"
#include "qsynedit/syntaxer/asm.h"
#include "qsynedit/syntaxer/glsl.h"
#include "qsynedit/syntaxer/makefile.h"

#include "qsynedit/constants.h"
#include "colorscheme.h"

SyntaxerManager syntaxerManager;

SyntaxerManager::SyntaxerManager()
{

}

QSynedit::PSyntaxer SyntaxerManager::getSyntaxer(QSynedit::ProgrammingLanguage language)
{
    switch(language) {
    case QSynedit::ProgrammingLanguage::CPP:
        return std::make_shared<QSynedit::CppSyntaxer>();
    case QSynedit::ProgrammingLanguage::Assembly:
        return std::make_shared<QSynedit::ASMSyntaxer>();
    case QSynedit::ProgrammingLanguage::Makefile:
        return std::make_shared<QSynedit::MakefileSyntaxer>();
    case QSynedit::ProgrammingLanguage::GLSL:
        return std::make_shared<QSynedit::GLSLSyntaxer>();
    default:
        return QSynedit::PSyntaxer();
    }
}

QSynedit::PSyntaxer SyntaxerManager::getSyntaxer(const QString &filename)
{
    QFileInfo info(filename);
    QString suffix = info.suffix();
    QString basename = info.baseName();
    if (suffix == "c" || suffix == "cpp" || suffix == "cxx"
            || suffix == "cc" || suffix == "h" || suffix == "hpp"
            || suffix == "hxx" || suffix == "hh" || suffix == "C"
            || suffix == "CPP" || suffix =="H" || suffix == "c++"
            || suffix == "h++") {
        return getSyntaxer(QSynedit::ProgrammingLanguage::CPP);
    } else if (suffix == "vs" || suffix == "fs" || suffix == "frag") {
        return getSyntaxer(QSynedit::ProgrammingLanguage::GLSL);
    } else if (suffix == "s" || suffix == "asm") {
        return getSyntaxer(QSynedit::ProgrammingLanguage::Assembly);
    } else if (basename.compare("makefile", Qt::CaseInsensitive)==0) {
        return getSyntaxer(QSynedit::ProgrammingLanguage::Makefile);
    } else if (suffix.isEmpty()) {
        return getSyntaxer(QSynedit::ProgrammingLanguage::CPP);
    }
    return QSynedit::PSyntaxer();
}

QSynedit::PSyntaxer SyntaxerManager::copy(QSynedit::PSyntaxer syntaxer)
{
    if (!syntaxer)
        return QSynedit::PSyntaxer();
    return getSyntaxer(syntaxer->language());
}

void SyntaxerManager::applyColorScheme(QSynedit::PSyntaxer syntaxer, const QString &schemeName)
{
    if (!syntaxer)
        return;

    for (QString name: syntaxer->attributes().keys()) {
        PColorSchemeItem item = pColorManager->getItem(schemeName,name);
        if (item) {
            QSynedit::PTokenAttribute attr = syntaxer->attributes()[name];
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
