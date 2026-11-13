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
#ifndef CPPDECLARATIONPARSER_H
#define CPPDECLARATIONPARSER_H

#include <QSet>
#include "cpptokencursor.h"
#include "cpptypes.h"

class CppDeclarationParser
{
public:
    explicit CppDeclarationParser(CppTokenCursor &cursor);

    bool parseDeclaration(DeclarationInfo &result);
    bool parseSimpleDeclaration(DeclarationInfo &result);
    bool parseTemplatePrefix(DeclarationInfo &result);
    bool parseDeclSpecifierSeq(DeclSpecifierInfo &result);
    bool parseDeclarator(DeclaratorInfo &result);
    bool parseParameter(ParameterInfo &result);
    bool parseTemplateArguments(TypeInfo &result);
    QString parseQualifiedNameText();
    void setCurrentClassName(const QString &className);
    QString currentClassName() const { return mClassName; }

private:
    bool parseTypeName(TypeInfo &result);
    bool isConstructorDeclaration();
    bool parseStructuredBinding(DeclarationInfo &result);
    QString parseOperatorName();
    QString readBalancedToken(const QString &opening, const QString &closing);
    QString readUntilDelimiter(const QSet<QString> &delimiters);
    void skipExpression(const QSet<QString> &delimiters);
    void skipTemplateDefaultArgument();
    void applyTrailingReturn(TypeDerivation &function);
    bool isBuiltinType(const QString &text) const;
    bool isDeclarationDelimiter(const QString &text) const;

    CppTokenCursor &mCursor;
    QString mClassName;
};

#endif // CPPDECLARATIONPARSER_H
