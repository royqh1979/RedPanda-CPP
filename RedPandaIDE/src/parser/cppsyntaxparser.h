/*
 * Copyright (C) 2026 Roy Qu (royqh1979@gmail.com)
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
#ifndef CPPSYNTAXPARSER_H
#define CPPSYNTAXPARSER_H

#include "cppdeclarationparser.h"

class CppSyntaxParser
{
public:
    explicit CppSyntaxParser(CppTokenCursor &cursor);

    bool parseDeclaration(DeclarationInfo &result);

private:
    bool parseClass(DeclarationInfo &result);
    bool parseNamespace(DeclarationInfo &result);
    bool parseEnum(DeclarationInfo &result);
    bool parseUsing(DeclarationInfo &result);
    bool parseBlock(DeclarationInfo &result);
    bool parseFor(DeclarationInfo &result);
    bool parseCatch(DeclarationInfo &result);
    bool parseControlBlock(DeclarationInfo &result);
    bool parseStatementBody(DeclarationInfo &result);

    QString readUntil(const QSet<QString> &delimiters);
    QStringList splitTopLevel(const QString &text);
    void skipBalanced();
    void skipDeclaration();
    void finishRanges(DeclarationInfo &result, const SourcePosition &start);

    CppTokenCursor &mCursor;
    CppDeclarationParser mDeclarationParser;
    QString mRootFileName;
};

#endif // CPPSYNTAXPARSER_H
