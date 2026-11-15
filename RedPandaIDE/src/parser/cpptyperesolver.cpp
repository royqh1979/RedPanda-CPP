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

bool isTypeDeclaration(const PStatement &statement)
{
    if (!statement)
        return false;
    switch (statement->kind) {
    case StatementKind::Namespace:
    case StatementKind::NamespaceAlias:
    case StatementKind::Class:
    case StatementKind::EnumType:
    case StatementKind::EnumClassType:
    case StatementKind::Typedef:
    case StatementKind::Alias:
        return true;
    default:
        return false;
    }
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

struct ResolutionGuard {
    ResolutionGuard(QSet<QString> &active, const QString &key) : active(active), key(key) {
        active.insert(key);
    }
    ~ResolutionGuard() {
        active.remove(key);
    }
    QSet<QString> &active;
    QString key;
};

void materializeTemplateArguments(TypeInfo &type)
{
    if (!type.templateArguments.isEmpty() || !type.qualifiedNameText.contains('<'))
        return;
    CppTokenizer tokenizer;
    tokenizer.tokenize({type.qualifiedNameText});
    const CppTokenizer::TokenList &tokens = tokenizer.tokens();
    int templateStart = -1;
    int templateLevel = 0;
    for (int i = 0; i < tokens.count(); ++i) {
        const QString &text = tokens[i].text;
        if (text == "<") {
            if (templateLevel == 0)
                templateStart = i;
            ++templateLevel;
            continue;
        }
        if (text == ">" || text == ">>") {
            templateLevel = qMax(0, templateLevel - text.length());
            continue;
        }
        if (tokens[i].matchIndex > i)
            i = tokens[i].matchIndex;
    }
    if (templateStart < 0)
        return;
    CppTokenCursor cursor(tokenizer);
    cursor.restore({templateStart, 0});
    CppDeclarationParser parser(cursor);
    parser.parseTemplateArguments(type);
}
}

CppTypeResolver::CppTypeResolver(const StatementModel &model)
    : mModel(model)
{
}

TypeInfo CppTypeResolver::resolve(const TypeInfo &type, const TemplateBindings &bindings,
                                  const PStatement &context) const
{
    auto frame = std::make_shared<TemplateBindingFrame>();
    frame->values = bindings;
    EvaluationContext evaluation;
    TypeInfo result = resolveType(type, frame, context, evaluation);
    return evaluation.resolutionFailed ? TypeInfo() : result;
}

PStatement CppTypeResolver::findStatement(const QString &qualifiedName,
                                          const PStatement &context) const
{
    return findDeclaration(qualifiedName, context);
}

PStatement CppTypeResolver::memberStatement(const TypeInfo &receiver,
                                            const QString &memberName) const
{
    EvaluationContext evaluation;
    ClassInstance instance = classInstance(receiver, {}, {}, evaluation);
    return findMember(instance, memberName, evaluation).statement;
}

TypeInfo CppTypeResolver::memberType(const TypeInfo &receiver, const QString &memberName) const
{
    EvaluationContext evaluation;
    ClassInstance instance = classInstance(receiver, {}, {}, evaluation);
    TypeInfo trait = traitMemberType(instance, memberName, evaluation);
    if (!trait.qualifiedName.isEmpty())
        return trait;
    MemberResolution member = findMember(instance, memberName, evaluation);
    if (!member.statement)
        return {};
    return resolveType(member.statement->typeInfo, member.owner.bindings,
                       member.statement->parentScope.lock(), evaluation);
}

TypeInfo CppTypeResolver::invokeResult(const TypeInfo &receiver, const QString &memberName) const
{
    EvaluationContext evaluation;
    ClassInstance instance = classInstance(receiver, {}, {}, evaluation);
    MemberResolution member = findMember(instance, memberName, evaluation);
    if (!isInvocable(member.statement))
        return {};
    return resolveType(member.statement->typeInfo, member.owner.bindings,
                       member.statement->parentScope.lock(), evaluation);
}

TypeInfo CppTypeResolver::invokeFunction(
        const PStatement &function, const QVector<TypeInfo> &arguments,
        const QVector<TemplateArgument> &explicitArguments) const
{
    TypeInfo result;
    int matchCount = 0;
    for (const PStatement &candidate : functionCandidates(function)) {
        TypeInfo current = invokeFunctionCandidate(candidate, arguments, explicitArguments);
        if (current.qualifiedName.isEmpty())
            continue;
        ++matchCount;
        result = current;
    }
    return matchCount == 1 ? result : TypeInfo();
}

QList<PStatement> CppTypeResolver::functionCandidates(const PStatement &function) const
{
    QList<PStatement> result;
    if (!function)
        return result;
    QSet<const Statement *> seen;
    auto appendScope = [&](const PStatement &scope) {
        const auto candidates = mModel.childrenStatements(scope).values(function->command);
        for (const std::weak_ptr<Statement> &weak : candidates) {
            PStatement candidate = weak.lock();
            if (isInvocable(candidate) && !seen.contains(candidate.get())) {
                seen.insert(candidate.get());
                result.append(candidate);
            }
        }
    };
    PStatement parent = function->parentScope.lock();
    if (parent && parent->kind == StatementKind::Namespace) {
        for (const PStatement &nameSpace : mModel.namespaceStatements(parent->fullName))
            appendScope(nameSpace);
        for (const PStatement &nameSpace : mModel.transparentNamespaceStatements(parent->fullName))
            appendScope(nameSpace);
    } else {
        appendScope(parent);
    }
    return result;
}

TypeInfo CppTypeResolver::invokeFunctionCandidate(
        const PStatement &function, const QVector<TypeInfo> &arguments,
        const QVector<TemplateArgument> &explicitArguments) const
{
    if (!function)
        return {};
    EvaluationContext evaluation;
    auto frame = std::make_shared<TemplateBindingFrame>();
    frame->owner = function.get();
    QSet<QString> templateParameters;
    for (const TemplateParameterInfo &parameter : function->templateParameters)
        templateParameters.insert(parameter.name);

    int explicitCount = qMin(explicitArguments.count(), function->templateParameters.count());
    for (int i = 0; i < explicitCount; ++i) {
        const TemplateParameterInfo &parameter = function->templateParameters[i];
        const TemplateArgument &argument = explicitArguments[i];
        if (parameter.kind == TemplateParameterKind::Type) {
            if (argument.kind != TemplateArgumentKind::Type || !argument.type)
                return {};
            TypeInfo actual = resolveType(*argument.type, frame, function->parentScope.lock(),
                                          evaluation);
            if (actual.qualifiedName.isEmpty())
                return {};
            frame->values.insert(parameter.name, TemplateArgument::fromType(actual));
        } else {
            QString constant = argument.constantExpression;
            if (constant.isEmpty() && argument.type)
                constant = argument.type->qualifiedNameText;
            frame->values.insert(parameter.name, TemplateArgument::fromConstant(
                    resolveConstantExpression(constant, frame, function->parentScope.lock(),
                                              evaluation)));
        }
    }

    int count = qMin(function->parameters.count(), arguments.count());
    for (int i = 0; i < count; ++i) {
        if (function->parameters[i].type) {
            bindParameterType(*function->parameters[i].type, arguments[i], templateParameters,
                              frame->values);
        }
    }
    for (int i = explicitCount; i < function->templateParameters.count(); ++i) {
        const TemplateParameterInfo &parameter = function->templateParameters[i];
        if (frame->values.contains(parameter.name))
            continue;
        QString defaultText = parameter.defaultArgumentText;
        if (defaultText.isEmpty() && i < function->inheritedDefaultArgumentTexts.count())
            defaultText = function->inheritedDefaultArgumentTexts[i];
        if (defaultText.isEmpty())
            continue;
        TemplateArgument argument = resolveArgument(defaultText, parameter.kind,
                                                    frame, function->parentScope.lock(), evaluation);
        frame->values.insert(parameter.name, argument);
    }
    bool parameterPack = false;
    for (const TemplateParameterInfo &parameter : function->templateParameters)
        parameterPack |= parameter.parameterPack;
    if ((!parameterPack && function->parameters.count() != arguments.count())
            || (parameterPack && function->parameters.count() - 1 > arguments.count())) {
        return {};
    }
    for (int i = 0; i < count; ++i) {
        if (function->parameters[i].type
                && !compatibleArgument(*function->parameters[i].type, arguments[i], frame,
                                       function->parentScope.lock(), evaluation)) {
            return {};
        }
    }
    return resolveType(function->typeInfo, frame, function->parentScope.lock(), evaluation);
}

bool CppTypeResolver::compatibleArgument(
        const TypeInfo &parameter, const TypeInfo &argument,
        const PTemplateBindingFrame &bindings, const PStatement &context,
        EvaluationContext &evaluation) const
{
    TypeInfo formal = resolveType(parameter, bindings, context, evaluation);
    auto pointerLike = [](const TypeInfo &type) {
        for (const TypeDerivation &derivation : type.derivations) {
            if (derivation.kind == TypeDerivationKind::Pointer
                    || derivation.kind == TypeDerivationKind::MemberPointer) {
                return true;
            }
        }
        return false;
    };
    return pointerLike(formal) == pointerLike(argument);
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
    return findDeclaration(baseName, scope->parentScope.lock(), true);
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
        signatures.insert(entry.statement->command + entry.statement->normalizedArgs);

    for (const std::weak_ptr<Statement> &weak : scope->children) {
        PStatement statement = weak.lock();
        if (!statement || (inherited && (statement->kind == StatementKind::Constructor
                                        || statement->kind == StatementKind::Destructor))) {
            continue;
        }
        if (signatures.contains(statement->command + statement->normalizedArgs))
            continue;
        signatures.insert(statement->command + statement->normalizedArgs);
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
            if (signatures.contains(entry.statement->command + entry.statement->normalizedArgs))
                continue;
            entry.accessibility = applyInheritance(entry.accessibility, inheritance);
            signatures.insert(entry.statement->command + entry.statement->normalizedArgs);
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

TypeInfo CppTypeResolver::resolveType(const TypeInfo &type,
                                      const PTemplateBindingFrame &bindings,
                                      const PStatement &context,
                                      EvaluationContext &evaluation) const
{
    if (++evaluation.operationCount > 4096)
        return {};
    QString resolutionKey = (context ? context->fullName : QString()) + "|" + type.toString()
            + "|" + (bindings && bindings->owner ? bindings->owner->fullName : QString());
    if (evaluation.resolvingTypes.contains(resolutionKey)) {
        evaluation.resolutionFailed = true;
        return {};
    }
    ResolutionGuard guard(evaluation.resolvingTypes, resolutionKey);

    QString text = type.qualifiedNameText;
    if (text.isEmpty() || (type.qualifiedName.contains(' ')
                           && !type.qualifiedName.startsWith("typename "))) {
        text = type.qualifiedName;
    }
    QVector<TypeNameSegment> segments;
    bool globallyQualified = false;
    if (!parseTypeNameSegments(text, segments, globallyQualified) || segments.isEmpty())
        return {};
    if (segments.count() == 1 && segments.first().arguments.isEmpty()) {
        const QString &name = segments.first().name;
        if (const TemplateArgument *binding = findBinding(bindings, name)) {
            if (binding->kind != TemplateArgumentKind::Type || !binding->type)
                return {};
            if (binding->type->toString() == type.toString())
                return {};
            TypeInfo result = resolveType(*binding->type, bindings, context, evaluation);
            applyReference(result, type);
            return result;
        }
    }
    if (segments.last().arguments.isEmpty() && !type.templateArguments.isEmpty()) {
        segments.last().arguments = type.templateArguments;
        segments.last().hasTemplateArguments = true;
    }
    for (TypeDerivation derivation : type.derivations) {
        for (ParameterInfo &parameter : derivation.parameters) {
            if (parameter.type)
                resolveType(*parameter.type, bindings, context, evaluation);
        }
    }
    return resolveSegments(type, segments, globallyQualified, bindings, context, evaluation);
}

TypeInfo CppTypeResolver::resolveSegments(
        const TypeInfo &reference, const QVector<TypeNameSegment> &segments,
        bool globallyQualified, const PTemplateBindingFrame &bindings,
        const PStatement &context, EvaluationContext &evaluation) const
{
    int index = 0;
    PStatement declaration;
    PTemplateBindingFrame environment = bindings;
    bool substitutedFirstSegment = false;
    if (segments.count() > 1 && segments.first().arguments.isEmpty()) {
        if (const TemplateArgument *binding = findBinding(bindings, segments.first().name)) {
            if (binding->kind != TemplateArgumentKind::Type || !binding->type)
                return {};
            ClassInstance owner = classInstance(*binding->type, bindings, context, evaluation);
            MemberResolution member = findMember(owner, segments[1].name, evaluation);
            if (!member.statement)
                return {};
            declaration = member.statement;
            environment = member.owner.bindings;
            index = 1;
            substitutedFirstSegment = true;
        }
    }
    if (!substitutedFirstSegment) {
        declaration = reference.declaration.lock();
        bool declarationMatches = declaration
                && (declaration->fullName == reference.qualifiedName
                    || (segments.count() == 1 && declaration->command == segments.first().name));
        if (!declarationMatches) {
            declaration = findDeclaration(segments.first().name,
                                          globallyQualified ? PStatement() : context, true);
        } else if (declaration->fullName == reference.qualifiedName) {
            index = segments.count() - 1;
        }
    }
    while (declaration && index < segments.count()) {
        if (++evaluation.operationCount > 4096)
            return {};
        const TypeNameSegment &segment = segments[index];
        TypeInfo instance;
        instance.qualifiedName = declaration->fullName;
        instance.declaration = declaration;
        instance.templateArguments = segment.arguments;
        for (TemplateArgument &argument : instance.templateArguments) {
            if (argument.kind != TemplateArgumentKind::Type || !argument.type)
                continue;
            const TemplateArgument *binding = findBinding(bindings, argument.type->qualifiedName);
            if (binding && binding->kind == TemplateArgumentKind::NonType) {
                argument.kind = TemplateArgumentKind::NonType;
                argument.constantExpression = binding->constantExpression;
                argument.type.reset();
                continue;
            }
            argument.type = std::make_shared<TypeInfo>(resolveType(
                    *argument.type, bindings, context, evaluation));
            if (argument.type->qualifiedName.isEmpty())
                return {};
        }

        if (isAlias(declaration)) {
            QString aliasKey = QString("%1:%2").arg(reinterpret_cast<quintptr>(declaration.get()),
                                                    0, 16).arg(instance.toString());
            if (evaluation.aliases.contains(aliasKey))
                return {};
            evaluation.aliases.insert(aliasKey);
            PTemplateBindingFrame localBindings = declarationBindings(
                    declaration, instance, environment, evaluation);
            TypeInfo targetType = declaration->typeInfo;
            PStatement targetDeclaration = targetType.declaration.lock();
            if (!instance.templateArguments.isEmpty() && targetType.templateArguments.isEmpty()
                    && targetDeclaration && targetDeclaration->kind == StatementKind::Class
                    && !targetDeclaration->templateParameters.isEmpty()) {
                targetType.templateArguments = instance.templateArguments;
            }
            TypeInfo target = resolveType(targetType, localBindings,
                                          declaration->parentScope.lock(), evaluation);
            evaluation.aliases.remove(aliasKey);
            if (target.qualifiedName.isEmpty())
                return {};
            if (index + 1 == segments.count()) {
                applyReference(target, reference);
                return target;
            }
            ClassInstance targetInstance = classInstance(target, localBindings,
                                                         declaration->parentScope.lock(), evaluation);
            MemberResolution member = findMember(targetInstance, segments[index + 1].name,
                                                 evaluation);
            declaration = member.statement;
            environment = member.owner.bindings;
            ++index;
            continue;
        }

        if (declaration->kind == StatementKind::Class) {
            if (!segment.hasTemplateArguments && !declaration->templateParameters.isEmpty()) {
                PStatement scope = context;
                while (scope && scope != declaration)
                    scope = scope->parentScope.lock();
                if (scope == declaration) {
                    PTemplateBindingFrame current = environment;
                    while (current && current->owner != declaration.get())
                        current = current->parent;
                    if (current) {
                        QVector<TemplateArgument> arguments;
                        for (const TemplateParameterInfo &parameter : declaration->templateParameters) {
                            auto argument = current->values.constFind(parameter.name);
                            if (argument == current->values.constEnd())
                                break;
                            arguments.append(argument.value());
                        }
                        if (arguments.count() == declaration->templateParameters.count())
                            instance.templateArguments = arguments;
                    }
                }
            }
            environment = declarationBindings(declaration, instance, environment, evaluation);
            ClassInstance selected = selectSpecialization({instance, declaration, environment},
                                                          evaluation);
            if (!selected.declaration)
                return {};
            declaration = selected.declaration;
            instance = selected.type;
            environment = selected.bindings;
        }
        if (index + 1 == segments.count()) {
            applyReference(instance, reference);
            return instance;
        }

        if (declaration->kind == StatementKind::Class && index + 2 == segments.count()) {
            TypeInfo trait = traitMemberType({instance, declaration, environment},
                                             segments[index + 1].name, evaluation);
            if (!trait.qualifiedName.isEmpty()) {
                applyReference(trait, reference);
                return trait;
            }
        }
        MemberResolution member;
        if (declaration->kind == StatementKind::Class) {
            member = findMember({instance, declaration, environment}, segments[index + 1].name,
                                evaluation);
        } else if (declaration->kind == StatementKind::Namespace) {
            QStringList qualifiedParts;
            for (int i = 0; i <= index + 1; ++i)
                qualifiedParts.append(segments[i].name);
            member.statement = findDeclaration(qualifiedParts.join("::"), context, true);
            member.owner = {instance, declaration, environment};
        } else if (declaration->kind == StatementKind::EnumType
                   || declaration->kind == StatementKind::EnumClassType) {
            member.statement = findMember(declaration, segments[index + 1].name);
            member.owner = {instance, declaration, environment};
        }
        if (!member.statement && declaration->kind == StatementKind::Class)
            return {};
        declaration = member.statement;
        environment = member.owner.bindings;
        ++index;
    }
    if (evaluation.operationCount > 4096 || evaluation.resolutionFailed)
        return {};
    TypeInfo unresolved = reference;
    unresolved.declaration.reset();
    QStringList names;
    for (const TypeNameSegment &segment : segments)
        names.append(segment.name);
    unresolved.qualifiedName = (globallyQualified ? "::" : "") + names.join("::");
    bool hasOuterArguments = false;
    for (int i = 0; i + 1 < segments.count(); ++i)
        hasOuterArguments |= !segments[i].arguments.isEmpty();
    if (hasOuterArguments) {
        unresolved.templateArguments.clear();
    } else {
        unresolved.templateArguments = segments.last().arguments;
        unresolved.qualifiedNameText.clear();
        for (TemplateArgument &argument : unresolved.templateArguments) {
            if (argument.kind == TemplateArgumentKind::Type && argument.type) {
                argument.type = std::make_shared<TypeInfo>(resolveType(
                        *argument.type, bindings, context, evaluation));
            }
        }
    }
    return unresolved;
}

bool CppTypeResolver::parseTypeNameSegments(const QString &text,
                                            QVector<TypeNameSegment> &segments,
                                            bool &globallyQualified) const
{
    CppTokenizer tokenizer;
    tokenizer.tokenize({text});
    const CppTokenizer::TokenList &tokens = tokenizer.tokens();
    int index = 0;
    globallyQualified = false;
    while (index < tokens.count() && (tokens[index].text == "typename"
                                      || tokens[index].text == "class"
                                      || tokens[index].text == "struct"
                                      || tokens[index].text == "union"
                                      || tokens[index].text == "enum")) {
        ++index;
    }
    if (index < tokens.count() && tokens[index].text == "::") {
        globallyQualified = true;
        ++index;
    }

    auto appendText = [](QString &target, const QString &part) {
        if (!target.isEmpty() && !part.isEmpty()
                && (target.back().isLetterOrNumber() || target.back() == '_')
                && (part.front().isLetterOrNumber() || part.front() == '_')) {
            target += ' ';
        }
        target += part;
    };
    auto parseArgument = [&](const QString &argumentText) {
        TypeInfo argumentType = parseTypeText(argumentText);
        if (!argumentType.qualifiedName.isEmpty()) {
            TemplateArgument result = TemplateArgument::fromType(argumentType);
            result.constantExpression = argumentText.trimmed();
            return result;
        }
        return TemplateArgument::fromConstant(argumentText.trimmed());
    };

    while (index < tokens.count()) {
        if (tokens[index].kind != CppTokenizer::TokenKind::Identifier
                && tokens[index].kind != CppTokenizer::TokenKind::Operator) {
            return false;
        }
        TypeNameSegment segment;
        segment.name = tokens[index++].text;
        while (index < tokens.count()
               && tokens[index].kind == CppTokenizer::TokenKind::Identifier) {
            appendText(segment.name, tokens[index++].text);
        }
        if (segment.name == "decltype" && index < tokens.count()
                && tokens[index].text == "(" && tokens[index].matchIndex > index) {
            int close = tokens[index].matchIndex;
            for (; index <= close && index < tokens.count(); ++index)
                appendText(segment.name, tokens[index].text);
        }
        if (index < tokens.count() && tokens[index].text == "<") {
            segment.hasTemplateArguments = true;
            ++index;
            int level = 1;
            QString argumentText;
            auto finishArgument = [&]() {
                if (argumentText.trimmed().isEmpty())
                    return false;
                segment.arguments.append(parseArgument(argumentText));
                argumentText.clear();
                return true;
            };
            bool closed = false;
            while (index < tokens.count()) {
                const QString &part = tokens[index].text;
                if (tokens[index].matchIndex > index && part != "<") {
                    int close = tokens[index].matchIndex;
                    for (; index <= close && index < tokens.count(); ++index)
                        appendText(argumentText, tokens[index].text);
                    continue;
                }
                if (part == "<") {
                    ++level;
                    appendText(argumentText, part);
                    ++index;
                    continue;
                }
                if (part == ">" || part == ">>") {
                    int closingCount = part.length();
                    ++index;
                    while (closingCount-- > 0) {
                        --level;
                        if (level == 0) {
                            if (!finishArgument())
                                return false;
                            closed = true;
                            break;
                        }
                        appendText(argumentText, ">");
                    }
                    if (closed)
                        break;
                    continue;
                }
                if (part == "," && level == 1) {
                    if (!finishArgument())
                        return false;
                    ++index;
                    continue;
                }
                appendText(argumentText, part);
                ++index;
            }
            if (!closed)
                return false;
        }
        segments.append(segment);
        if (index == tokens.count())
            break;
        if (tokens[index++].text != "::" || index == tokens.count())
            return false;
        if (tokens[index].text == "template") {
            segments.last().templateKeyword = true;
            ++index;
        }
    }
    return !segments.isEmpty();
}

TypeInfo CppTypeResolver::parseTypeText(const QString &text) const
{
    CppTokenizer tokenizer;
    tokenizer.tokenize({text});
    CppTokenCursor cursor(tokenizer);
    CppDeclarationParser parser(cursor);
    DeclSpecifierInfo specifiers;
    if (!parser.parseDeclSpecifierSeq(specifiers))
        return {};
    DeclaratorInfo declarator;
    if (!parser.parseDeclarator(declarator) || !cursor.atEnd())
        return {};
    TypeInfo result = specifiers.type;
    result.derivations += declarator.derivations;
    return result;
}

CppTypeResolver::ClassInstance CppTypeResolver::classInstance(
        const TypeInfo &type, const PTemplateBindingFrame &bindings,
        const PStatement &context, EvaluationContext &evaluation) const
{
    ClassInstance result;
    result.type = resolveType(type, bindings, context, evaluation);
    result.declaration = result.type.declaration.lock();
    if (!result.declaration)
        result.declaration = findDeclaration(result.type.qualifiedName, context, true);
    if (!result.declaration || (result.declaration->kind != StatementKind::Class
                                && result.declaration->kind != StatementKind::Lambda)) {
        return {};
    }
    if (result.declaration->kind == StatementKind::Class
            && result.declaration->templateKind != TemplateDeclarationKind::Primary) {
        QString primaryName = result.declaration->command.section('<', 0, 0);
        PStatement primary = findDeclaration(primaryName, result.declaration->parentScope.lock(),
                                             true);
        if (!primary)
            return {};
        result.declaration = primary;
        result.type.declaration = primary;
        result.type.qualifiedName = primary->fullName;
    }
    result.bindings = declarationBindings(result.declaration, result.type, bindings, evaluation);
    if (result.declaration->kind == StatementKind::Class)
        return selectSpecialization(result, evaluation);
    return result;
}

CppTypeResolver::ClassInstance CppTypeResolver::selectSpecialization(
        const ClassInstance &primary, EvaluationContext &evaluation) const
{
    if (!primary.declaration || primary.declaration->kind != StatementKind::Class
            || primary.declaration->templateKind != TemplateDeclarationKind::Primary) {
        return primary;
    }
    ClassInstance result = primary;
    result.type.templateArguments.clear();
    for (const TemplateParameterInfo &parameter : primary.declaration->templateParameters) {
        auto argument = primary.bindings->values.constFind(parameter.name);
        if (argument == primary.bindings->values.constEnd())
            return {};
        result.type.templateArguments.append(argument.value());
    }

    PStatement parent = primary.declaration->parentScope.lock();
    QList<PStatement> scopes;
    if (parent && parent->kind == StatementKind::Namespace) {
        scopes += mModel.namespaceStatements(parent->fullName);
        scopes += mModel.transparentNamespaceStatements(parent->fullName);
    } else {
        scopes.append(parent);
    }
    QString prefix = primary.declaration->command + "<";
    QList<PStatement> partialMatches;
    bool unknown = false;
    for (const PStatement &scope : scopes) {
        const StatementMap &children = mModel.childrenStatements(scope);
        for (auto it = children.lowerBound(prefix);
             it != children.end() && it.key().startsWith(prefix); ++it) {
            PStatement candidate = it.value().lock();
            if (!candidate || candidate->kind != StatementKind::Class
                    || (candidate->templateKind != TemplateDeclarationKind::ExplicitSpecialization
                        && candidate->templateKind != TemplateDeclarationKind::PartialSpecialization)) {
                continue;
            }
            TemplateBindings bindings;
            SpecializationMatch match = matchSpecialization(candidate, result.type.templateArguments,
                                                            parent, bindings);
            if (match == SpecializationMatch::Unknown) {
                unknown = true;
                continue;
            }
            if (match != SpecializationMatch::Match)
                continue;
            if (candidate->templateKind == TemplateDeclarationKind::ExplicitSpecialization) {
                auto selectedBindings = std::make_shared<TemplateBindingFrame>();
                selectedBindings->owner = candidate.get();
                selectedBindings->parent = primary.bindings->parent;
                result.declaration = candidate;
                result.bindings = selectedBindings;
                result.type.declaration = candidate;
                return result;
            }
            partialMatches.append(candidate);
            if (partialMatches.count() == 1) {
                auto selectedBindings = std::make_shared<TemplateBindingFrame>();
                selectedBindings->owner = candidate.get();
                selectedBindings->values = bindings;
                selectedBindings->parent = primary.bindings->parent;
                result.declaration = candidate;
                result.bindings = selectedBindings;
                result.type.declaration = candidate;
            }
        }
    }
    if (unknown || partialMatches.count() > 1)
        return {};
    if (partialMatches.isEmpty()) {
        result.declaration = primary.declaration;
        result.bindings = primary.bindings;
        result.type.declaration = primary.declaration;
    }
    return result;
}

CppTypeResolver::SpecializationMatch CppTypeResolver::matchSpecialization(
        const PStatement &specialization, const QVector<TemplateArgument> &arguments,
        const PStatement &context, TemplateBindings &bindings) const
{
    TypeInfo pattern = parseTypeText(specialization->command);
    materializeTemplateArguments(pattern);
    if (pattern.templateArguments.count() != arguments.count())
        return SpecializationMatch::NoMatch;
    QSet<QString> parameters;
    for (const TemplateParameterInfo &parameter : specialization->templateParameters)
        parameters.insert(parameter.name);
    SpecializationMatch combined = SpecializationMatch::Match;
    for (int i = 0; i < arguments.count(); ++i) {
        SpecializationMatch match = matchArgument(pattern.templateArguments[i], arguments[i],
                                                  parameters, bindings, context);
        if (match == SpecializationMatch::NoMatch)
            return match;
        if (match == SpecializationMatch::Unknown)
            combined = match;
    }
    return combined;
}

CppTypeResolver::SpecializationMatch CppTypeResolver::matchArgument(
        const TemplateArgument &pattern, const TemplateArgument &actual,
        const QSet<QString> &parameters, TemplateBindings &bindings,
        const PStatement &context) const
{
    QString patternText = pattern.constantExpression.trimmed();
    if (patternText.isEmpty() && pattern.type) {
        patternText = pattern.type->qualifiedNameText.isEmpty()
                ? pattern.type->qualifiedName : pattern.type->qualifiedNameText;
    }
    if (actual.kind == TemplateArgumentKind::NonType && parameters.contains(patternText)) {
        TemplateArgument captured = TemplateArgument::fromConstant(actual.constantExpression);
        auto existing = bindings.constFind(patternText);
        if (existing != bindings.constEnd()) {
            return existing->constantExpression == captured.constantExpression
                    ? SpecializationMatch::Match : SpecializationMatch::NoMatch;
        }
        bindings.insert(patternText, captured);
        return SpecializationMatch::Match;
    }
    if (pattern.kind == TemplateArgumentKind::Type && actual.kind == TemplateArgumentKind::Type
            && pattern.type && actual.type) {
        return matchType(*pattern.type, *actual.type, parameters, bindings, context);
    }
    if (actual.kind != TemplateArgumentKind::NonType || patternText.isEmpty())
        return SpecializationMatch::NoMatch;

    QString actualText = actual.constantExpression.trimmed();
    if (patternText == actualText)
        return SpecializationMatch::Match;
    bool patternLiteral = false;
    bool actualLiteral = false;
    qlonglong patternValue = patternText.toLongLong(&patternLiteral, 0);
    qlonglong actualValue = actualText.toLongLong(&actualLiteral, 0);
    if (patternLiteral && actualLiteral)
        return patternValue == actualValue ? SpecializationMatch::Match
                                           : SpecializationMatch::NoMatch;
    if ((patternText == "true" || patternText == "false")
            && (actualText == "true" || actualText == "false")) {
        return SpecializationMatch::NoMatch;
    }
    return SpecializationMatch::Unknown;
}

CppTypeResolver::SpecializationMatch CppTypeResolver::matchType(
        const TypeInfo &pattern, const TypeInfo &actual, const QSet<QString> &parameters,
        TemplateBindings &bindings, const PStatement &context) const
{
    TypeInfo formal = pattern;
    TypeInfo real = actual;
    materializeTemplateArguments(formal);
    materializeTemplateArguments(real);
    if (formal.qualifiers.testFlag(TypeQualifier::Const)
            && !real.qualifiers.testFlag(TypeQualifier::Const)) {
        return SpecializationMatch::NoMatch;
    }
    if (formal.qualifiers.testFlag(TypeQualifier::Volatile)
            && !real.qualifiers.testFlag(TypeQualifier::Volatile)) {
        return SpecializationMatch::NoMatch;
    }
    formal.qualifiers = TypeQualifier::None;
    real.qualifiers = TypeQualifier::None;
    while (!formal.derivations.isEmpty()) {
        if (real.derivations.isEmpty())
            return SpecializationMatch::NoMatch;
        const TypeDerivation formalDerivation = formal.derivations.takeFirst();
        const TypeDerivation realDerivation = real.derivations.takeFirst();
        if (formalDerivation.kind != realDerivation.kind
                || (formalDerivation.qualifiers != realDerivation.qualifiers)) {
            return SpecializationMatch::NoMatch;
        }
        if (formalDerivation.kind == TypeDerivationKind::Array
                && !formalDerivation.text.isEmpty() && !realDerivation.text.isEmpty()
                && formalDerivation.text != realDerivation.text) {
            return SpecializationMatch::Unknown;
        }
    }
    if (parameters.contains(formal.qualifiedName)) {
        auto existing = bindings.constFind(formal.qualifiedName);
        TemplateArgument captured = TemplateArgument::fromType(real);
        if (existing != bindings.constEnd()) {
            if (!existing->type)
                return SpecializationMatch::NoMatch;
            return existing->type->toString() == real.toString()
                    ? SpecializationMatch::Match : SpecializationMatch::NoMatch;
        }
        bindings.insert(formal.qualifiedName, captured);
        return SpecializationMatch::Match;
    }
    if (!real.derivations.isEmpty())
        return SpecializationMatch::NoMatch;

    PStatement formalDeclaration = findDeclaration(formal.qualifiedName, context, true);
    PStatement realDeclaration = real.declaration.lock();
    if (formalDeclaration && realDeclaration) {
        if (formalDeclaration != realDeclaration)
            return SpecializationMatch::NoMatch;
    } else if (formal.qualifiedName != real.qualifiedName) {
        return SpecializationMatch::NoMatch;
    }
    if (formal.templateArguments.count() != real.templateArguments.count())
        return SpecializationMatch::NoMatch;
    SpecializationMatch combined = SpecializationMatch::Match;
    for (int i = 0; i < formal.templateArguments.count(); ++i) {
        SpecializationMatch match = matchArgument(formal.templateArguments[i],
                                                  real.templateArguments[i], parameters,
                                                  bindings, context);
        if (match == SpecializationMatch::NoMatch)
            return match;
        if (match == SpecializationMatch::Unknown)
            combined = match;
    }
    return combined;
}

CppTypeResolver::MemberResolution CppTypeResolver::findMember(
        const ClassInstance &instance, const QString &name, EvaluationContext &evaluation) const
{
    QSet<QString> visited;
    return findMember(instance, name, visited, evaluation);
}

CppTypeResolver::MemberResolution CppTypeResolver::findMember(
        const ClassInstance &instance, const QString &name, QSet<QString> &visited,
        EvaluationContext &evaluation) const
{
    if (!instance.declaration)
        return {};
    QString instanceKey = instance.declaration->fullName + "<" + instance.type.toString() + ">";
    if (visited.contains(instanceKey))
        return {};
    visited.insert(instanceKey);
    const auto candidates = instance.declaration->children.values(name);
    PStatement constFallback;
    for (const std::weak_ptr<Statement> &weak : candidates) {
        PStatement candidate = weak.lock();
        if (!candidate)
            continue;
        bool qualifiedMember = (candidate->kind == StatementKind::Function
                                || candidate->kind == StatementKind::OverloadedOperator)
                && !candidate->isStatic();
        bool constMember = candidate->properties.testFlag(StatementProperty::Const);
        if (qualifiedMember && instance.type.qualifiers.testFlag(TypeQualifier::Const)
                && !constMember) {
            continue;
        }
        if (qualifiedMember && constMember
                && !instance.type.qualifiers.testFlag(TypeQualifier::Const)) {
            if (!constFallback)
                constFallback = candidate;
            continue;
        }
        return {candidate, instance, candidate->accessibility};
    }
    if (constFallback)
        return {constFallback, instance, constFallback->accessibility};
    for (QString baseText : instance.declaration->baseClasses) {
        baseText = baseText.trimmed();
        StatementAccessibility inheritance = instance.declaration->classKey == ClassKey::Struct
                ? StatementAccessibility::Public : StatementAccessibility::Private;
        for (const QString &modifier : {QString("public"), QString("protected"),
                                       QString("private"), QString("virtual")}) {
            if (!baseText.startsWith(modifier + ' '))
                continue;
            if (modifier == "public")
                inheritance = StatementAccessibility::Public;
            else if (modifier == "protected")
                inheritance = StatementAccessibility::Protected;
            else if (modifier == "private")
                inheritance = StatementAccessibility::Private;
            baseText = baseText.mid(modifier.length()).trimmed();
        }
        TypeInfo baseType = parseTypeText(baseText);
        if (baseType.qualifiedName.isEmpty())
            continue;
        ClassInstance base = classInstance(baseType, instance.bindings,
                                           instance.declaration->parentScope.lock(), evaluation);
        MemberResolution member = findMember(base, name, visited, evaluation);
        if (!member.statement)
            continue;
        member.accessibility = applyInheritance(member.accessibility, inheritance);
        return member;
    }
    return {};
}

void CppTypeResolver::applyReference(TypeInfo &target, const TypeInfo &reference) const
{
    target.qualifiers |= reference.qualifiers;
    QVector<TypeDerivation> additions = reference.derivations;
    if (!additions.isEmpty()
            && (additions.first().kind == TypeDerivationKind::LvalueReference
                || additions.first().kind == TypeDerivationKind::RvalueReference)
            && !target.derivations.isEmpty()
            && (target.derivations.first().kind == TypeDerivationKind::LvalueReference
                || target.derivations.first().kind == TypeDerivationKind::RvalueReference)) {
        bool rvalue = additions.first().kind == TypeDerivationKind::RvalueReference
                && target.derivations.first().kind == TypeDerivationKind::RvalueReference;
        target.derivations.removeFirst();
        additions.removeFirst();
        target.derivations.prepend(rvalue ? TypeDerivation::rvalueReference()
                                          : TypeDerivation::lvalueReference());
    }
    target.derivations += additions;
}

PStatement CppTypeResolver::findDeclaration(const QString &qualifiedName,
                                            const PStatement &context, bool typesOnly) const
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
        if (typesOnly && searchScope->kind == StatementKind::Class
                && searchScope->command.section('<', 0, 0) == parts.first()) {
            return findDeclarationRemainder(searchScope, parts, typesOnly);
        }
        if (searchScope->kind == StatementKind::Namespace) {
            bool firstPartFound = false;
            PStatement result = findInNamespaceScope(searchScope, parts, firstPartFound, typesOnly);
            if (result || firstPartFound)
                return result;
        } else if (PStatement current = findMember(searchScope, parts.first(), typesOnly)) {
            return findDeclarationRemainder(current, parts, typesOnly);
        }
        searchScope = searchScope->parentScope.lock();
    }
    bool firstPartFound = false;
    return findInNamespaceScope({}, parts, firstPartFound, typesOnly);
}

PStatement CppTypeResolver::findInNamespaceScope(const PStatement &scope,
                                                 const QStringList &parts,
                                                 bool &firstPartFound, bool typesOnly) const
{
    for (int namespacePartCount = parts.count(); namespacePartCount >= 0; --namespacePartCount) {
        QString namespaceName = scope ? scope->fullName : QString();
        if (namespacePartCount > 0) {
            if (!namespaceName.isEmpty())
                namespaceName += "::";
            namespaceName += parts.mid(0, namespacePartCount).join("::");
        }
        const QList<PStatement> namespaces = mModel.namespaceStatements(namespaceName);
        if (namespaces.isEmpty())
            continue;
        if (namespacePartCount > 0)
            firstPartFound = true;
        if (namespacePartCount == parts.count()) {
            for (const PStatement &nameSpace : namespaces) {
                if (nameSpace->fullName == namespaceName)
                    return nameSpace;
            }
        }
        for (const PStatement &nameSpace : namespaces) {
            PStatement current = nameSpace;
            for (int i = namespacePartCount; current && i < parts.count(); ++i)
                current = findMember(current, parts[i], typesOnly);
            if (current)
                return current;
        }
    }

    if (!scope) {
        const auto candidates = mModel.childrenStatements().values(parts.first());
        for (const std::weak_ptr<Statement> &weak : candidates) {
            if (PStatement candidate = weak.lock()) {
                if (typesOnly && !isTypeDeclaration(candidate))
                    continue;
                if (PStatement result = findDeclarationRemainder(candidate, parts, typesOnly))
                    return result;
            }
        }
    }
    return nullptr;
}

PStatement CppTypeResolver::findDeclarationRemainder(const PStatement &statement,
                                                     const QStringList &parts,
                                                     bool typesOnly) const
{
    if (typesOnly && !isTypeDeclaration(statement))
        return nullptr;
    if (parts.count() > 1 && statement
            && statement->kind != StatementKind::Class
            && statement->kind != StatementKind::EnumType
            && statement->kind != StatementKind::EnumClassType) {
        return nullptr;
    }
    PStatement current = statement;
    for (int i = 1; current && i < parts.count(); ++i)
        current = findMember(current, parts[i], typesOnly);
    return current;
}

PStatement CppTypeResolver::findMember(const PStatement &scope, const QString &name,
                                       bool typesOnly) const
{
    QSet<const Statement *> visited;
    return findMemberRecursive(scope, name, visited, typesOnly);
}

PStatement CppTypeResolver::findMemberRecursive(const PStatement &scope, const QString &name,
                                                QSet<const Statement *> &visited,
                                                bool typesOnly) const
{
    if (!scope || visited.contains(scope.get()))
        return nullptr;
    visited.insert(scope.get());
    const auto directCandidates = scope->children.values(name);
    for (const std::weak_ptr<Statement> &weak : directCandidates) {
        PStatement candidate = weak.lock();
        if (!candidate || (typesOnly && !isTypeDeclaration(candidate)))
            continue;
        if (candidate->kind != StatementKind::Namespace || candidate->command == name)
            return candidate;
        if (PStatement result = findMemberRecursive(candidate, name, visited, typesOnly))
            return result;
    }
    if (scope->kind == StatementKind::Namespace) {
        const QList<PStatement> transparentNamespaces =
                mModel.transparentNamespaceStatements(scope->fullName);
        for (const PStatement &child : transparentNamespaces) {
            if (PStatement result = findMemberRecursive(child, name, visited, typesOnly))
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
        PStatement base = findDeclaration(baseName, scope->parentScope.lock(), typesOnly);
        if (PStatement result = findMemberRecursive(base, name, visited, typesOnly))
            return result;
    }
    return nullptr;
}

const TemplateArgument *CppTypeResolver::findBinding(
        const PTemplateBindingFrame &bindings, const QString &name) const
{
    for (PTemplateBindingFrame frame = bindings; frame; frame = frame->parent) {
        auto binding = frame->values.constFind(name);
        if (binding != frame->values.constEnd())
            return &binding.value();
    }
    return nullptr;
}

QString CppTypeResolver::resolveConstantExpression(
        const QString &text, const PTemplateBindingFrame &bindings,
        const PStatement &context, EvaluationContext &evaluation) const
{
    QString expression = text.trimmed();
    if (expression == "true" || expression == "false")
        return expression;
    if (expression.startsWith('!')) {
        QString nested = resolveConstantExpression(expression.mid(1), bindings, context, evaluation);
        if (nested == "true")
            return "false";
        if (nested == "false")
            return "true";
        return expression;
    }
    if (!expression.endsWith("::value"))
        return expression;
    expression.truncate(expression.length() - QString("::value").length());
    TypeInfo traitType = parseTypeText(expression);
    if (traitType.qualifiedName.isEmpty())
        return text.trimmed();
    ClassInstance trait = classInstance(traitType, bindings, context, evaluation);
    if (!trait.declaration || trait.type.templateArguments.isEmpty()
            || trait.type.templateArguments.first().kind != TemplateArgumentKind::Type
            || !trait.type.templateArguments.first().type) {
        return text.trimmed();
    }
    const TypeInfo &argument = *trait.type.templateArguments.first().type;
    if (trait.declaration->fullName == "std::is_array") {
        for (const TypeDerivation &derivation : argument.derivations) {
            if (derivation.kind == TypeDerivationKind::Array)
                return "true";
        }
        return "false";
    }
    if (trait.declaration->fullName == "std::is_void")
        return argument.qualifiedName == "void" && argument.derivations.isEmpty()
                ? "true" : "false";
    return text.trimmed();
}

TypeInfo CppTypeResolver::traitMemberType(const ClassInstance &instance,
                                          const QString &memberName,
                                          EvaluationContext &evaluation) const
{
    Q_UNUSED(evaluation);
    if (!instance.declaration || memberName != "type"
            || instance.declaration->fullName != "std::add_lvalue_reference"
            || instance.type.templateArguments.count() != 1) {
        return {};
    }
    const TemplateArgument &argument = instance.type.templateArguments.first();
    if (argument.kind != TemplateArgumentKind::Type || !argument.type)
        return {};
    TypeInfo result = *argument.type;
    if (!result.derivations.isEmpty()
            && (result.derivations.first().kind == TypeDerivationKind::LvalueReference
                || result.derivations.first().kind == TypeDerivationKind::RvalueReference)) {
        result.derivations.removeFirst();
    }
    result.derivations.prepend(TypeDerivation::lvalueReference());
    return result;
}

TemplateArgument CppTypeResolver::resolveArgument(
        const QString &text, TemplateParameterKind kind,
        const PTemplateBindingFrame &bindings, const PStatement &context,
        EvaluationContext &evaluation) const
{
    if (kind != TemplateParameterKind::Type) {
        if (const TemplateArgument *binding = findBinding(bindings, text.trimmed()))
            return *binding;
        return TemplateArgument::fromConstant(resolveConstantExpression(
                text, bindings, context, evaluation));
    }

    TypeInfo type = parseTypeText(text);
    if (type.qualifiedName.isEmpty())
        return {};
    type = resolveType(type, bindings, context, evaluation);
    if (type.qualifiedName.isEmpty())
        return {};
    return TemplateArgument::fromType(type);
}

CppTypeResolver::PTemplateBindingFrame CppTypeResolver::declarationBindings(
        const PStatement &declaration, const TypeInfo &instance,
        const PTemplateBindingFrame &outerBindings, EvaluationContext &evaluation) const
{
    auto result = std::make_shared<TemplateBindingFrame>();
    result->owner = declaration.get();
    result->parent = outerBindings;
    if (!declaration)
        return result;

    TypeInfo materialized = instance;
    materializeTemplateArguments(materialized);
    int parameterCount = declaration->templateParameters.count();
    for (int i = 0; i < parameterCount; ++i) {
        const TemplateParameterInfo &parameter = declaration->templateParameters[i];
        if (parameter.parameterPack) {
            if (i < materialized.templateArguments.count())
                evaluation.operationCount = 4097;
            continue;
        }
        TemplateArgument argument;
        if (i < materialized.templateArguments.count()) {
            argument = materialized.templateArguments[i];
            if (parameter.kind == TemplateParameterKind::Type
                    && argument.kind == TemplateArgumentKind::Type && argument.type) {
                argument.type = std::make_shared<TypeInfo>(resolveType(
                        *argument.type, outerBindings, declaration->parentScope.lock(), evaluation));
            }
        } else {
            QString defaultText = parameter.defaultArgumentText;
            if (defaultText.isEmpty() && i < declaration->inheritedDefaultArgumentTexts.count())
                defaultText = declaration->inheritedDefaultArgumentTexts[i];
            if (!defaultText.isEmpty()) {
                argument = resolveArgument(defaultText, parameter.kind, result,
                                           declaration->parentScope.lock(), evaluation);
            } else {
                evaluation.operationCount = 4097;
                continue;
            }
        }
        if ((parameter.kind == TemplateParameterKind::Type
             && (argument.kind != TemplateArgumentKind::Type || !argument.type
                 || argument.type->qualifiedName.isEmpty()))
                || (parameter.kind != TemplateParameterKind::Type
                    && argument.constantExpression.isEmpty())) {
            evaluation.operationCount = 4097;
            continue;
        }
        result->values.insert(parameter.name, argument);
    }
    return result;
}
