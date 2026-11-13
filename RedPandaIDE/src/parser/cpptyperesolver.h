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
#ifndef CPPTYPERESOLVER_H
#define CPPTYPERESOLVER_H

#include "statementmodel.h"

using TemplateBindings = QHash<QString, TemplateArgument>;

struct EvalResult {
    TypeInfo type;
    EvalStatementKind kind = EvalStatementKind::Literal;
    PStatement baseStatement;
    PStatement statement;
    TemplateBindings templateBindings;

    bool isValid() const { return !type.qualifiedName.isEmpty(); }
};

class CppTypeResolver
{
public:
    explicit CppTypeResolver(const StatementModel &model);

    TypeInfo resolve(const TypeInfo &type, const TemplateBindings &bindings = {},
                     const PStatement &context = {}) const;
    PStatement findStatement(const QString &qualifiedName, const PStatement &context = {}) const;
    PStatement memberStatement(const TypeInfo &receiver, const QString &memberName) const;
    TypeInfo memberType(const TypeInfo &receiver, const QString &memberName) const;
    TypeInfo invokeResult(const TypeInfo &receiver, const QString &memberName) const;
    TypeInfo invokeFunction(const PStatement &function, const QVector<TypeInfo> &arguments) const;

private:
    void bindParameterType(const TypeInfo &parameter, const TypeInfo &argument,
                           const QSet<QString> &templateParameters, TemplateBindings &bindings) const;
    TypeInfo resolveType(const TypeInfo &type, const TemplateBindings &bindings,
                         const PStatement &context, QSet<const Statement *> &aliases) const;
    PStatement findDeclaration(const QString &qualifiedName, const PStatement &context) const;
    PStatement findMember(const PStatement &scope, const QString &name) const;
    PStatement findMemberRecursive(const PStatement &scope, const QString &name,
                                   QSet<const Statement *> &visited) const;
    TemplateBindings declarationBindings(const PStatement &declaration, const TypeInfo &instance,
                                         const TemplateBindings &outerBindings) const;

    const StatementModel &mModel;
};

#endif // CPPTYPERESOLVER_H
