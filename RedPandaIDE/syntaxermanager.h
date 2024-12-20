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
#ifndef SYNTAXERMANAGER_H
#define SYNTAXERMANAGER_H
#include "qsynedit/syntaxer/syntaxer.h"
#include "qsynedit/formatter/formatter.h"
#include "utils.h"

class SyntaxerManager
{
public:
    SyntaxerManager();

    QSynedit::PSyntaxer getSyntaxer(QSynedit::ProgrammingLanguage language) const;
    QSynedit::PSyntaxer getSyntaxer(FileType fileType) const {
        return getSyntaxer(getLanguage(fileType));
    }
    QSynedit::PFormatter getFormatter(QSynedit::ProgrammingLanguage language) const;
    QSynedit::PFormatter getFormatter(FileType fileType) const {
        return getFormatter(getLanguage(fileType));
    }
    QSynedit::ProgrammingLanguage getLanguage(FileType fileType) const;
    QSynedit::PSyntaxer copy(QSynedit::PSyntaxer syntaxer) const;
    void applyColorScheme(QSynedit::PSyntaxer syntaxer, const QString& schemeName) const;
};

extern SyntaxerManager syntaxerManager;

#endif // SYNTAXERMANAGER_H
