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
#include "cpptyperesolver.h"

namespace {
PStatement findInMap(const StatementMap &statements, const QString &name)
{
    const auto values = statements.values(name);
    for (const std::weak_ptr<Statement> &statement : values) {
        if (PStatement result = statement.lock())
            return result;
    }
    return nullptr;
}

bool isAlias(const PStatement &statement)
{
    return statement && (statement->kind == StatementKind::Alias
                         || statement->kind == StatementKind::Typedef);
}

bool isInvocable(const PStatement &statement)
{
    if (!statement)
        return false;
    switch (statement->kind) {
    case StatementKind::Function:
    case StatementKind::OverloadedOperator:
    case StatementKind::LiteralOperator:
        return true;
    default:
        return false;
    }
}
}

CppTypeResolver::CppTypeResolver(const StatementModel &model)
    : mModel(model)
{
}

TypeInfo CppTypeResolver::resolve(const TypeInfo &type, const TemplateBindings &bindings,
                                  const PStatement &context) const
{
    QSet<const Statement *> aliases;
    return resolveType(type, bindings, context, aliases);
}

TypeInfo CppTypeResolver::memberType(const TypeInfo &receiver, const QString &memberName) const
{
    TypeInfo resolvedReceiver = resolve(receiver);
    PStatement declaration = resolvedReceiver.declaration.lock();
    if (!declaration)
        declaration = findDeclaration(resolvedReceiver.qualifiedName, {});
    PStatement member = findMember(declaration, memberName);
    if (!member)
        return {};
    TemplateBindings bindings = declarationBindings(declaration, resolvedReceiver, {});
    return resolve(member->typeInfo, bindings, declaration);
}

TypeInfo CppTypeResolver::invokeResult(const TypeInfo &receiver, const QString &memberName) const
{
    TypeInfo resolvedReceiver = resolve(receiver);
    PStatement declaration = resolvedReceiver.declaration.lock();
    if (!declaration)
        declaration = findDeclaration(resolvedReceiver.qualifiedName, {});
    PStatement member = findMember(declaration, memberName);
    if (!isInvocable(member))
        return {};
    TemplateBindings bindings = declarationBindings(declaration, resolvedReceiver, {});
    return resolve(member->typeInfo, bindings, declaration);
}

TypeInfo CppTypeResolver::resolveType(const TypeInfo &type, const TemplateBindings &bindings,
                                      const PStatement &context, QSet<const Statement *> &aliases) const
{
    auto binding = bindings.constFind(type.qualifiedName);
    if (binding != bindings.constEnd()
            && binding->kind == TemplateArgumentKind::Type && binding->type) {
        TemplateBindings nestedBindings = bindings;
        nestedBindings.remove(type.qualifiedName);
        TypeInfo result = resolveType(*binding->type, nestedBindings, context, aliases);
        result.qualifiers |= type.qualifiers;
        result.derivations += type.derivations;
        return result;
    }

    TypeInfo result = type;
    for (TemplateArgument &argument : result.templateArguments) {
        if (argument.kind == TemplateArgumentKind::Type && argument.type)
            argument.type = std::make_shared<TypeInfo>(resolveType(*argument.type, bindings,
                                                                   context, aliases));
    }
    for (TypeDerivation &derivation : result.derivations) {
        for (ParameterInfo &parameter : derivation.parameters) {
            if (parameter.type)
                parameter.type = std::make_shared<TypeInfo>(resolveType(*parameter.type, bindings,
                                                                        context, aliases));
        }
    }

    PStatement declaration = result.declaration.lock();
    if (!declaration)
        declaration = findDeclaration(result.qualifiedName, context);
    if (isAlias(declaration)) {
        if (aliases.contains(declaration.get()))
            return result;
        aliases.insert(declaration.get());
        TemplateBindings localBindings = declarationBindings(declaration, result, bindings);
        TypeInfo target = resolveType(declaration->typeInfo, localBindings,
                                      declaration->parentScope.lock(), aliases);
        aliases.remove(declaration.get());
        target.qualifiers |= result.qualifiers;
        target.derivations += result.derivations;
        return target;
    }
    if (declaration)
        result.declaration = declaration;
    return result;
}

PStatement CppTypeResolver::findDeclaration(const QString &qualifiedName,
                                            const PStatement &context) const
{
    QString name = qualifiedName;
    PStatement initialContext = context;
    if (name.startsWith("::")) {
        name.remove(0, 2);
        initialContext.reset();
    }
    const QStringList parts = name.split("::", Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return nullptr;

    PStatement current;
    PStatement searchScope = initialContext;
    while (searchScope) {
        current = findMember(searchScope, parts.first());
        if (current)
            break;
        searchScope = searchScope->parentScope.lock();
    }
    if (!current)
        current = findInMap(mModel.childrenStatements(), parts.first());
    for (int i = 1; current && i < parts.count(); ++i)
        current = findMember(current, parts[i]);
    return current;
}

PStatement CppTypeResolver::findMember(const PStatement &scope, const QString &name) const
{
    if (!scope)
        return nullptr;
    return findInMap(scope->children, name);
}

TemplateBindings CppTypeResolver::declarationBindings(
        const PStatement &declaration, const TypeInfo &instance,
        const TemplateBindings &outerBindings) const
{
    TemplateBindings result = outerBindings;
    if (!declaration)
        return result;
    int count = qMin(declaration->templateParameters.count(), instance.templateArguments.count());
    for (int i = 0; i < count; ++i) {
        TemplateArgument argument = instance.templateArguments[i];
        if (argument.kind == TemplateArgumentKind::Type && argument.type) {
            QSet<const Statement *> aliases;
            argument.type = std::make_shared<TypeInfo>(resolveType(
                    *argument.type, outerBindings, declaration->parentScope.lock(), aliases));
        }
        result.insert(declaration->templateParameters[i].name, argument);
    }
    return result;
}
