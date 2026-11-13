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
#include "cppdeclarationparser.h"
#include "cpptokencursor.h"

namespace {
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

void materializeTemplateArguments(TypeInfo &type)
{
    if (!type.templateArguments.isEmpty() || type.templateArgumentsText.isEmpty())
        return;
    CppTokenizer tokenizer;
    tokenizer.tokenize({type.templateArgumentsText});
    CppTokenCursor cursor(tokenizer);
    CppDeclarationParser parser(cursor);
    if (parser.parseTemplateArguments(type))
        type.templateArgumentsText.clear();
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

PStatement CppTypeResolver::findStatement(const QString &qualifiedName,
                                          const PStatement &context) const
{
    return findDeclaration(qualifiedName, context);
}

PStatement CppTypeResolver::memberStatement(const TypeInfo &receiver,
                                            const QString &memberName) const
{
    TypeInfo resolvedReceiver = resolve(receiver);
    PStatement declaration = resolvedReceiver.declaration.lock();
    if (!declaration)
        declaration = findDeclaration(resolvedReceiver.qualifiedName, {});
    return findMember(declaration, memberName);
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

TypeInfo CppTypeResolver::invokeFunction(const PStatement &function,
                                         const QVector<TypeInfo> &arguments) const
{
    if (!function)
        return {};
    QSet<QString> templateParameters;
    for (const TemplateParameterInfo &parameter : function->templateParameters)
        templateParameters.insert(parameter.name);
    TemplateBindings bindings;
    int count = qMin(function->parameters.count(), arguments.count());
    for (int i = 0; i < count; ++i) {
        if (function->parameters[i].type)
            bindParameterType(*function->parameters[i].type, arguments[i], templateParameters, bindings);
    }
    return resolve(function->typeInfo, bindings, function->parentScope.lock());
}

QList<PStatement> CppTypeResolver::membersOf(const PStatement &scope) const
{
    QList<PStatement> result;
    if (!scope) {
        for (const std::weak_ptr<Statement> &weak : mModel.childrenStatements()) {
            if (PStatement statement = weak.lock())
                result.append(statement);
        }
        return result;
    }
    QSet<const Statement *> visited;
    MemberEntryList entries;
    appendMembers(scope, false, visited, entries);
    for (const MemberEntry &entry : entries)
        result.append(entry.statement);
    return result;
}

StatementAccessibility CppTypeResolver::memberAccessibility(
        const PStatement &scope, const PStatement &member) const
{
    if (!scope)
        return member ? member->accessibility : StatementAccessibility::None;
    QSet<const Statement *> visited;
    MemberEntryList entries;
    appendMembers(scope, false, visited, entries);
    for (const MemberEntry &entry : entries) {
        if (entry.statement == member)
            return entry.accessibility;
    }
    return StatementAccessibility::None;
}

PStatement CppTypeResolver::baseClass(const PStatement &scope, const QString &baseSpecifier,
                                      StatementAccessibility &inheritance) const
{
    QString baseName = baseSpecifier.trimmed();
    inheritance = scope->classKey == ClassKey::Struct
            ? StatementAccessibility::Public : StatementAccessibility::Private;
    for (const QString &modifier : {QString("public"), QString("protected"),
                                   QString("private"), QString("virtual")}) {
        if (!baseName.startsWith(modifier + ' '))
            continue;
        if (modifier == "public")
            inheritance = StatementAccessibility::Public;
        else if (modifier == "protected")
            inheritance = StatementAccessibility::Protected;
        else if (modifier == "private")
            inheritance = StatementAccessibility::Private;
        baseName = baseName.mid(modifier.length()).trimmed();
    }
    int templateStart = baseName.indexOf('<');
    if (templateStart >= 0)
        baseName.truncate(templateStart);
    return findDeclaration(baseName, scope->parentScope.lock());
}

StatementAccessibility CppTypeResolver::applyInheritance(
        StatementAccessibility member, StatementAccessibility inheritance) const
{
    if (member == StatementAccessibility::None || inheritance == StatementAccessibility::None)
        return StatementAccessibility::None;
    if (member == StatementAccessibility::Private || inheritance == StatementAccessibility::Private)
        return StatementAccessibility::Private;
    if (member == StatementAccessibility::Protected || inheritance == StatementAccessibility::Protected)
        return StatementAccessibility::Protected;
    return StatementAccessibility::Public;
}

void CppTypeResolver::appendMembers(const PStatement &scope, bool inherited,
                                    QSet<const Statement *> &visited,
                                    MemberEntryList &result) const
{
    if (!scope || visited.contains(scope.get()))
        return;
    visited.insert(scope.get());
    QSet<QString> signatures;
    for (const MemberEntry &entry : result)
        signatures.insert(entry.statement->command + entry.statement->noNameArgs);

    for (const std::weak_ptr<Statement> &weak : scope->children) {
        PStatement statement = weak.lock();
        if (!statement || (inherited && (statement->kind == StatementKind::Constructor
                                        || statement->kind == StatementKind::Destructor))) {
            continue;
        }
        if (signatures.contains(statement->command + statement->noNameArgs))
            continue;
        signatures.insert(statement->command + statement->noNameArgs);
        result.append({statement, statement->accessibility});
    }

    for (const QString &baseSpecifier : scope->baseClasses) {
        StatementAccessibility inheritance;
        PStatement base = baseClass(scope, baseSpecifier, inheritance);
        if (!base)
            continue;
        MemberEntryList baseMembers;
        appendMembers(base, true, visited, baseMembers);
        for (MemberEntry entry : baseMembers) {
            if (signatures.contains(entry.statement->command + entry.statement->noNameArgs))
                continue;
            entry.accessibility = applyInheritance(entry.accessibility, inheritance);
            signatures.insert(entry.statement->command + entry.statement->noNameArgs);
            result.append(entry);
        }
    }
}

void CppTypeResolver::bindParameterType(const TypeInfo &parameter, const TypeInfo &argument,
                                        const QSet<QString> &templateParameters,
                                        TemplateBindings &bindings) const
{
    TypeInfo formal = parameter;
    TypeInfo actual = argument;
    auto removeReferences = [](TypeInfo &type) {
        while (!type.derivations.isEmpty()
                && (type.derivations.first().kind == TypeDerivationKind::LvalueReference
                    || type.derivations.first().kind == TypeDerivationKind::RvalueReference))
            type.derivations.removeFirst();
    };
    removeReferences(formal);
    removeReferences(actual);
    actual.qualifiers = TypeQualifier::None;

    if (templateParameters.contains(formal.qualifiedName)) {
        if (!bindings.contains(formal.qualifiedName))
            bindings.insert(formal.qualifiedName, TemplateArgument::fromType(actual));
        return;
    }
    if (!formal.derivations.isEmpty() && !actual.derivations.isEmpty()
            && formal.derivations.first().kind == actual.derivations.first().kind
            && (formal.derivations.first().kind == TypeDerivationKind::Pointer
                || formal.derivations.first().kind == TypeDerivationKind::Array)) {
        formal.derivations.removeFirst();
        actual.derivations.removeFirst();
        bindParameterType(formal, actual, templateParameters, bindings);
        return;
    }
    if (formal.qualifiedName != actual.qualifiedName)
        return;
    materializeTemplateArguments(formal);
    materializeTemplateArguments(actual);
    int count = qMin(formal.templateArguments.count(), actual.templateArguments.count());
    for (int i = 0; i < count; ++i) {
        const TemplateArgument &formalArgument = formal.templateArguments[i];
        const TemplateArgument &actualArgument = actual.templateArguments[i];
        if (formalArgument.kind == TemplateArgumentKind::Type && formalArgument.type
                && actualArgument.kind == TemplateArgumentKind::Type && actualArgument.type)
            bindParameterType(*formalArgument.type, *actualArgument.type, templateParameters, bindings);
    }
}

TypeInfo CppTypeResolver::resolveType(const TypeInfo &type, const TemplateBindings &bindings,
                                      const PStatement &context, QSet<const Statement *> &aliases) const
{
    TypeInfo result = type;
    materializeTemplateArguments(result);
    auto binding = bindings.constFind(result.qualifiedName);
    if (binding != bindings.constEnd()
            && binding->kind == TemplateArgumentKind::Type && binding->type) {
        TemplateBindings nestedBindings = bindings;
        nestedBindings.remove(type.qualifiedName);
        TypeInfo result = resolveType(*binding->type, nestedBindings, context, aliases);
        result.qualifiers |= type.qualifiers;
        result.derivations += type.derivations;
        return result;
    }

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
    if (declaration) {
        result.declaration = declaration;
        result.qualifiedName = declaration->fullName;
    }
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

    PStatement searchScope = initialContext;
    while (searchScope) {
        if (searchScope->kind == StatementKind::Namespace) {
            bool firstPartFound = false;
            PStatement result = findInNamespaceScope(searchScope, parts, firstPartFound);
            if (result || firstPartFound)
                return result;
        } else if (PStatement current = findMember(searchScope, parts.first())) {
            return findDeclarationRemainder(current, parts);
        }
        searchScope = searchScope->parentScope.lock();
    }
    bool firstPartFound = false;
    return findInNamespaceScope({}, parts, firstPartFound);
}

PStatement CppTypeResolver::findInNamespaceScope(const PStatement &scope,
                                                 const QStringList &parts,
                                                 bool &firstPartFound) const
{
    struct WorkItem {
        PStatement scope;
        int partIndex;
    };
    QVector<WorkItem> stack{{scope, 0}};
    QSet<QString> inlineNamespaces;
    bool inlineNamespacesLoaded = false;
    auto isTransparent = [&](const PStatement &statement) {
        if (statement->command.isEmpty()
                || statement->declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline)) {
            return true;
        }
        if (!inlineNamespacesLoaded) {
            for (const PStatement &candidate : mModel.statements()) {
                if (candidate->kind == StatementKind::Namespace
                        && candidate->declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline)) {
                    inlineNamespaces.insert(candidate->fullName);
                }
            }
            inlineNamespacesLoaded = true;
        }
        return inlineNamespaces.contains(statement->fullName);
    };
    while (!stack.isEmpty()) {
        WorkItem item = stack.takeLast();
        const auto candidates = mModel.childrenStatements(item.scope).values(
                    parts[item.partIndex]);
        if (item.partIndex == 0 && !candidates.isEmpty())
            firstPartFound = true;

        QVector<WorkItem> namespaceBranches;
        for (const std::weak_ptr<Statement> &weak : candidates) {
            PStatement candidate = weak.lock();
            if (!candidate)
                continue;
            if (candidate->kind != StatementKind::Namespace) {
                return findDeclarationRemainder(candidate, parts.mid(item.partIndex));
            }
            if (item.partIndex + 1 == parts.count())
                return candidate;
            namespaceBranches.append({candidate, item.partIndex + 1});
        }

        QVector<WorkItem> transparentBranches;
        for (const std::weak_ptr<Statement> &weak : mModel.childrenStatements(item.scope)) {
            PStatement child = weak.lock();
            if (child && child->kind == StatementKind::Namespace
                    && isTransparent(child)) {
                transparentBranches.append({child, item.partIndex});
            }
        }
        for (int i = transparentBranches.count() - 1; i >= 0; --i)
            stack.append(transparentBranches[i]);
        for (int i = namespaceBranches.count() - 1; i >= 0; --i)
            stack.append(namespaceBranches[i]);
    }
    return nullptr;
}

PStatement CppTypeResolver::findDeclarationRemainder(const PStatement &statement,
                                                     const QStringList &parts) const
{
    if (parts.count() > 1 && statement
            && statement->kind != StatementKind::Class
            && statement->kind != StatementKind::EnumType
            && statement->kind != StatementKind::EnumClassType) {
        return nullptr;
    }
    PStatement current = statement;
    for (int i = 1; current && i < parts.count(); ++i)
        current = findMember(current, parts[i]);
    return current;
}

PStatement CppTypeResolver::findMember(const PStatement &scope, const QString &name) const
{
    QSet<const Statement *> visited;
    return findMemberRecursive(scope, name, visited);
}

PStatement CppTypeResolver::findMemberRecursive(const PStatement &scope, const QString &name,
                                                QSet<const Statement *> &visited) const
{
    if (!scope || visited.contains(scope.get()))
        return nullptr;
    visited.insert(scope.get());
    const auto directCandidates = scope->children.values(name);
    for (const std::weak_ptr<Statement> &weak : directCandidates) {
        PStatement candidate = weak.lock();
        if (!candidate)
            continue;
        if (candidate->kind != StatementKind::Namespace || candidate->command == name)
            return candidate;
        if (PStatement result = findMemberRecursive(candidate, name, visited))
            return result;
    }
    for (const std::weak_ptr<Statement> &weakChild : scope->children) {
        PStatement child = weakChild.lock();
        if (child && child->kind == StatementKind::Namespace
                && (child->declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline)
                    || child->command.isEmpty())) {
            if (PStatement result = findMemberRecursive(child, name, visited))
                return result;
        }
    }
    for (QString baseName : scope->baseClasses) {
        baseName = baseName.trimmed();
        for (const QString &modifier : {QString("public"), QString("protected"),
                                       QString("private"), QString("virtual")}) {
            if (baseName.startsWith(modifier + ' '))
                baseName = baseName.mid(modifier.length()).trimmed();
        }
        int templateStart = baseName.indexOf('<');
        if (templateStart >= 0)
            baseName.truncate(templateStart);
        PStatement base = findDeclaration(baseName, scope->parentScope.lock());
        if (PStatement result = findMemberRecursive(base, name, visited))
            return result;
    }
    return nullptr;
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
