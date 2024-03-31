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
#include "qsynedit/syntaxer/cpp.h"
#include "qsynedit/syntaxer/asm.h"
#include "qsynedit/syntaxer/glsl.h"
#include "qsynedit/syntaxer/lua.h"
#include "qsynedit/syntaxer/makefile.h"
#include "qsynedit/syntaxer/textfile.h"
#include "qsynedit/formatter/cppformatter.h"

#include "colorscheme.h"

SyntaxerManager syntaxerManager;

SyntaxerManager::SyntaxerManager()
{

}

QSynedit::PSyntaxer SyntaxerManager::getSyntaxer(QSynedit::ProgrammingLanguage language) const
{
    switch(language) {
    case QSynedit::ProgrammingLanguage::CPP:
        return std::make_shared<QSynedit::CppSyntaxer>();
    case QSynedit::ProgrammingLanguage::Assembly:
        return std::make_shared<QSynedit::ASMSyntaxer>();
    case QSynedit::ProgrammingLanguage::ATTAssembly:
        return std::make_shared<QSynedit::ASMSyntaxer>(true);
    case QSynedit::ProgrammingLanguage::MixedAssembly:
        return std::make_shared<QSynedit::ASMSyntaxer>(false, true);
    case QSynedit::ProgrammingLanguage::MixedATTAssembly:
        return std::make_shared<QSynedit::ASMSyntaxer>(true, true);
    case QSynedit::ProgrammingLanguage::Makefile:
        return std::make_shared<QSynedit::MakefileSyntaxer>();
    case QSynedit::ProgrammingLanguage::GLSL:
        return std::make_shared<QSynedit::GLSLSyntaxer>();
    case QSynedit::ProgrammingLanguage::LUA:
        return std::make_shared<QSynedit::LuaSyntaxer>();
    case QSynedit::ProgrammingLanguage::XMAKE: {
        auto syntaxer=getSyntaxer(QSynedit::ProgrammingLanguage::LUA);
        QSynedit::LuaSyntaxer* pSyntaxer= (QSynedit::LuaSyntaxer*)syntaxer.get();
        pSyntaxer->setUseXMakeLibs(true);
        return syntaxer;
    }
    default:
        return std::make_shared<QSynedit::TextSyntaxer>();
    }
}

QSynedit::PSyntaxer SyntaxerManager::getSyntaxer(const QString &filename) const
{
    return getSyntaxer(getLanguage(filename));
}

QSynedit::PFormatter SyntaxerManager::getFormatter(QSynedit::ProgrammingLanguage language) const
{
    switch(language) {
    case QSynedit::ProgrammingLanguage::CPP:
        return std::make_shared<QSynedit::CppFormatter>();
    default:
        return QSynedit::PFormatter();
    }
}

QSynedit::PFormatter SyntaxerManager::getFormatter(const QString &filename) const
{
    return getFormatter(getLanguage(filename));
}

QSynedit::ProgrammingLanguage SyntaxerManager::getLanguage(const QString &filename) const
{
    QFileInfo info(filename);
    QString suffix = info.suffix();
    QString basename = info.baseName();
    if (suffix == "c" || suffix == "cpp" || suffix == "cxx"
            || suffix == "cc" || suffix == "h" || suffix == "hpp"
            || suffix == "hxx" || suffix == "hh" || suffix == "C"
            || suffix == "CPP" || suffix =="H" || suffix == "c++"
            || suffix == "h++") {
        return QSynedit::ProgrammingLanguage::CPP;
    } else if (suffix == "vs" || suffix == "fs" || suffix == "frag") {
        return QSynedit::ProgrammingLanguage::GLSL;
    } else if (suffix == "asm") {
        return QSynedit::ProgrammingLanguage::Assembly;
    } else if (suffix == "s" || suffix == "S") {
        return QSynedit::ProgrammingLanguage::ATTAssembly;
    } else if (suffix == "lua") {
        if (basename=="xmake") {
            return QSynedit::ProgrammingLanguage::XMAKE;
        } else
            return QSynedit::ProgrammingLanguage::LUA;
    } else if (basename.compare("makefile", Qt::CaseInsensitive)==0) {
        return QSynedit::ProgrammingLanguage::Makefile;
    } else if (suffix.isEmpty()) {
        return QSynedit::ProgrammingLanguage::CPP;
    }
    return QSynedit::ProgrammingLanguage::Textfile;
}

QSynedit::PSyntaxer SyntaxerManager::copy(QSynedit::PSyntaxer syntaxer) const
{
    if (!syntaxer)
        return QSynedit::PSyntaxer();
    return getSyntaxer(syntaxer->language());
}

void SyntaxerManager::applyColorScheme(QSynedit::PSyntaxer syntaxer, const QString &schemeName) const
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
