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
#ifndef QSYNEDIT_NASM_SYNTAXER_H
#define QSYNEDIT_NASM_SYNTAXER_H

#include "syntaxer.h"
#include "asm.h"

namespace QSynedit {

class NASMSyntaxer : public ASMSyntaxer
{
public:
    explicit NASMSyntaxer();
    NASMSyntaxer(const NASMSyntaxer&)=delete;
    NASMSyntaxer& operator=(const NASMSyntaxer&)=delete;

    static const QSet<QString> Directives;
    static const QSet<QString> PreprocessorDirectives;
private:
    QSet<QString> mKeywordCache;
protected:
    bool isCommentStartChar(QChar ch) override;
    bool isDirective(const QString& ident) override;
    bool isPreprocessDirective(const QString& ident) override;
public:
    QString languageName() override;
    ProgrammingLanguage language() override;
    QSet<QString> keywords() override;

};

}

#endif // QSYNEDIT_NASM_SYNTAXER_H
