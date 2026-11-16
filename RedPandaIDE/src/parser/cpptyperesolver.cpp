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

#include <QDebug>
#include <QElapsedTimer>
#include <QStack>

namespace {
struct ResolverProfileScope;

struct ResolverProfile {
    bool enabled = qEnvironmentVariableIsSet("REDPANDA_PARSER_PROFILE");
    const bool traceEnabled = qEnvironmentVariableIsSet("REDPANDA_PARSER_TRACE");
    qint64 resolveTypeNs = 0;
    qint64 parseSegmentsNs = 0;
    qint64 classInstanceNs = 0;
    qint64 specializationNs = 0;
    qint64 memberLookupNs = 0;
    qint64 declarationLookupNs = 0;
    qint64 parseTypeTextNs = 0;
    qint64 resolveTypeCalls = 0;
    qint64 parseSegmentsCalls = 0;
    qint64 classInstanceCalls = 0;
    qint64 specializationCalls = 0;
    qint64 memberLookupCalls = 0;
    qint64 declarationLookupCalls = 0;
    qint64 parseTypeTextCalls = 0;
    qint64 keyTextNs = 0;
    qint64 keyTextCalls = 0;
    qint64 segmentsNs = 0;
    qint64 segmentsCalls = 0;
    qint64 bindingsNs = 0;
    qint64 bindingsCalls = 0;
    qint64 materializeNs = 0;
    qint64 materializeCalls = 0;
    qint64 namespaceLookupNs = 0;
    qint64 namespaceLookupCalls = 0;
    qint64 declarationRemainderNs = 0;
    qint64 declarationRemainderCalls = 0;
    qint64 memberDeclarationNs = 0;
    qint64 memberDeclarationCalls = 0;
    qint64 memberDfsNs = 0;
    qint64 memberDfsCalls = 0;
    QHash<QString, qint64> declarationTrace;
    QHash<QString, qint64> memberDeclarationTrace;
    QHash<QString, qint64> resolveTypeTrace;
    QHash<QString, qint64> tokenizeOriginTrace;
    QHash<QString, qint64> specializationTrace;
    QHash<QString, qint64> bindingTrace;
    QHash<QString, qint64> argumentTrace;
    QHash<QString, qint64> functionTrace;
    QHash<QString, qint64> memberTrace;
    QHash<QString, qint64> resolveOriginTrace;
    QHash<QString, qint64> syntaxAttemptTrace;
    QHash<QString, qint64> specializationAttemptTrace;
    QHash<const char *, qint64> selfNs;
    ResolverProfileScope *activeScope = nullptr;
    QStringList resolveOriginStack;
    void reset()
    {
        resolveTypeNs = parseSegmentsNs = classInstanceNs = specializationNs = 0;
        memberLookupNs = declarationLookupNs = parseTypeTextNs = 0;
        resolveTypeCalls = parseSegmentsCalls = classInstanceCalls = specializationCalls = 0;
        memberLookupCalls = declarationLookupCalls = parseTypeTextCalls = 0;
        keyTextNs = keyTextCalls = segmentsNs = segmentsCalls = 0;
        bindingsNs = bindingsCalls = materializeNs = materializeCalls = 0;
        namespaceLookupNs = namespaceLookupCalls = 0;
        declarationRemainderNs = declarationRemainderCalls = 0;
        memberDeclarationNs = memberDeclarationCalls = 0;
        memberDfsNs = memberDfsCalls = 0;
        declarationTrace.clear();
        memberDeclarationTrace.clear();
        resolveTypeTrace.clear();
        tokenizeOriginTrace.clear();
        specializationTrace.clear();
        bindingTrace.clear();
        argumentTrace.clear();
        functionTrace.clear();
        memberTrace.clear();
        resolveOriginTrace.clear();
        syntaxAttemptTrace.clear();
        specializationAttemptTrace.clear();
        selfNs.clear();
        activeScope = nullptr;
        resolveOriginStack.clear();
    }
    ~ResolverProfile()
    {
        if (enabled) {
            qWarning() << "RESOLVER_PROFILE resolveType" << resolveTypeNs / 1000000.0 << resolveTypeCalls
                       << "parseSegments" << parseSegmentsNs / 1000000.0 << parseSegmentsCalls
                       << "classInstance" << classInstanceNs / 1000000.0 << classInstanceCalls
                       << "specialization" << specializationNs / 1000000.0 << specializationCalls
                       << "memberLookup" << memberLookupNs / 1000000.0 << memberLookupCalls
                       << "declarationLookup" << declarationLookupNs / 1000000.0 << declarationLookupCalls
                       << "parseTypeText" << parseTypeTextNs / 1000000.0 << parseTypeTextCalls
                       << "keyText" << keyTextNs / 1000000.0 << keyTextCalls
                       << "segments" << segmentsNs / 1000000.0 << segmentsCalls
                       << "bindings" << bindingsNs / 1000000.0 << bindingsCalls
                       << "materialize" << materializeNs / 1000000.0 << materializeCalls;
            qWarning() << "RESOLVER_DETAIL namespaceLookup" << namespaceLookupNs / 1000000.0
                       << namespaceLookupCalls << "declarationRemainder"
                       << declarationRemainderNs / 1000000.0 << declarationRemainderCalls
                       << "memberDeclaration" << memberDeclarationNs / 1000000.0
                       << memberDeclarationCalls << "memberDfs"
                       << memberDfsNs / 1000000.0 << memberDfsCalls;
            for (auto it = selfNs.constBegin(); it != selfNs.constEnd(); ++it)
                qWarning().noquote() << "RESOLVER_SELF" << it.key() << it.value() / 1000000.0;
        }
        for (auto it = declarationTrace.constBegin(); it != declarationTrace.constEnd(); ++it)
            qWarning().noquote() << "FIND_DECLARATION" << it.value()
                                 << it.key().toUtf8().toBase64();
        for (auto it = memberDeclarationTrace.constBegin(); it != memberDeclarationTrace.constEnd(); ++it)
            qWarning().noquote() << "MEMBER_DECLARATION" << it.value()
                                 << it.key().toUtf8().toBase64();
        for (auto it = resolveTypeTrace.constBegin(); it != resolveTypeTrace.constEnd(); ++it)
            qWarning().noquote() << "RESOLVE_TYPE" << it.value()
                                 << it.key().toUtf8().toBase64();
        for (auto it = tokenizeOriginTrace.constBegin(); it != tokenizeOriginTrace.constEnd(); ++it)
            qWarning().noquote() << "TOKENIZE_ORIGIN" << it.value() << it.key();
        for (auto it = specializationTrace.constBegin(); it != specializationTrace.constEnd(); ++it)
            qWarning().noquote() << "SELECT_SPECIALIZATION" << it.value()
                                 << it.key().toUtf8().toBase64();
        auto outputTrace = [](const char *label, const QHash<QString, qint64> &trace) {
            for (auto it = trace.constBegin(); it != trace.constEnd(); ++it)
                qWarning().noquote() << label << it.value() << it.key().toUtf8().toBase64();
        };
        outputTrace("DECLARATION_BINDINGS", bindingTrace);
        outputTrace("RESOLVE_ARGUMENT", argumentTrace);
        outputTrace("FUNCTION_CANDIDATE", functionTrace);
        outputTrace("MEMBER_CANDIDATES", memberTrace);
        outputTrace("RESOLVE_ORIGIN", resolveOriginTrace);
        outputTrace("SYNTAX_ATTEMPT", syntaxAttemptTrace);
        outputTrace("SPECIALIZATION_ATTEMPT", specializationAttemptTrace);
    }
};

ResolverProfile resolverProfile;

void traceEvent(QHash<QString, qint64> &trace, const QString &key)
{
    if (resolverProfile.traceEnabled)
        ++trace[key];
}

void traceSyntaxAttempt(const QString &operation, const QString &text, bool cacheHit)
{
    if (resolverProfile.traceEnabled) {
        QString origin = resolverProfile.resolveOriginStack.isEmpty()
                ? "external" : resolverProfile.resolveOriginStack.join(">");
        QString key = origin + "|" + operation + "|" + (cacheHit ? "hit" : "miss") + "|" + text;
        ++resolverProfile.syntaxAttemptTrace[key];
    }
}

void traceTokenizeOrigin(const QString &origin)
{
    if (resolverProfile.traceEnabled)
        ++resolverProfile.tokenizeOriginTrace[origin];
}

void traceSpecialization(const QString &name)
{
    if (resolverProfile.traceEnabled)
        ++resolverProfile.specializationTrace[name];
}

void traceDeclaration(const QString &name, const PStatement &context, bool typesOnly)
{
    if (resolverProfile.traceEnabled) {
        QString key = (context ? context->fullName : QString()) + "|" + name
                + "|" + (typesOnly ? "types" : "all");
        ++resolverProfile.declarationTrace[key];
    }
}

void traceResolveType(const QString &text, const PStatement &context,
                      const PTemplateBindingFrame &bindings)
{
    if (resolverProfile.traceEnabled) {
        QString origin = resolverProfile.resolveOriginStack.isEmpty()
                ? "external" : resolverProfile.resolveOriginStack.join(">");
        QString key = (context ? context->fullName : QString()) + "|"
                + (bindings && bindings->owner ? bindings->owner->fullName : QString()) + "|"
                + origin + "|" + text;
        ++resolverProfile.resolveTypeTrace[key];
        ++resolverProfile.resolveOriginTrace[origin];
    }
}

struct ResolveOriginScope {
    ResolveOriginScope(const QString &name)
        : active(resolverProfile.traceEnabled)
    {
        if (active)
            resolverProfile.resolveOriginStack.append(name);
    }
    ~ResolveOriginScope()
    {
        if (active)
            resolverProfile.resolveOriginStack.removeLast();
    }
    bool active;
};

struct ResolverProfileScope {
    ResolverProfileScope(qint64 &elapsed, qint64 &calls, const char *label)
        : elapsed(elapsed), calls(calls), label(label)
    {
        if (resolverProfile.enabled) {
            ++calls;
            parent = resolverProfile.activeScope;
            resolverProfile.activeScope = this;
            timer.start();
        }
    }
    ~ResolverProfileScope()
    {
        if (resolverProfile.enabled) {
            qint64 own = timer.nsecsElapsed();
            elapsed += own;
            resolverProfile.selfNs[label] += own - childNs;
            if (parent)
                parent->childNs += own;
            resolverProfile.activeScope = parent;
        }
    }
    qint64 &elapsed;
    qint64 &calls;
    const char *label;
    ResolverProfileScope *parent = nullptr;
    qint64 childNs = 0;
    QElapsedTimer timer;
};

QString profiledTypeString(const TypeInfo &type)
{
    ResolverProfileScope profileScope(resolverProfile.keyTextNs, resolverProfile.keyTextCalls,
                                      "typeToString");
    return type.toString();
}

bool isBuiltinTypeName(const QString &name)
{
    static const QSet<QString> names{
        "void", "bool", "char", "signed char", "unsigned char", "wchar_t", "char8_t",
        "char16_t", "char32_t", "short", "short int", "signed short", "signed short int",
        "unsigned short", "unsigned short int", "int", "signed", "signed int", "unsigned",
        "unsigned int", "long", "long int", "signed long", "signed long int", "unsigned long",
        "unsigned long int", "long long", "long long int", "signed long long",
        "signed long long int", "unsigned long long", "unsigned long long int", "float",
        "double", "long double"
    };
    return names.contains(name.simplified());
}

bool isBuiltinType(const TypeInfo &type)
{
    return type.templateArguments.isEmpty() && isBuiltinTypeName(type.qualifiedName);
}

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

}

CppTypeResolver::CppTypeResolver(const StatementModel &model, EvaluationContext *evaluation)
    : mModel(model), mEvaluation(evaluation)
{
}

TypeInfo CppTypeResolver::resolve(const TypeInfo &type, const TemplateBindings &bindings,
                                  const PStatement &context,
                                  EvaluationContext *evaluationContext) const
{
    auto frame = std::make_shared<TemplateBindingFrame>();
    frame->values = bindings;
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    TypeInfo result = resolveType(type, frame, context, evaluation);
    return evaluation.resolutionFailed ? TypeInfo() : result;
}

ClassInstance CppTypeResolver::resolveInstance(
        const TypeInfo &type, const TemplateBindings &bindings, const PStatement &context,
        EvaluationContext *evaluationContext) const
{
    auto frame = std::make_shared<TemplateBindingFrame>();
    frame->values = bindings;
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    ClassInstance result = resolveTypeInstance(type, frame, context, evaluation);
    return evaluation.resolutionFailed ? ClassInstance() : result;
}

PStatement CppTypeResolver::findStatement(const QString &qualifiedName,
                                          const PStatement &context) const
{
    return findDeclaration(qualifiedName, context);
}

PStatement CppTypeResolver::memberStatement(const TypeInfo &receiver,
                                            const QString &memberName,
                                            EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    ClassInstance instance = classInstance(receiver, {}, {}, evaluation);
    return findMember(instance, memberName, evaluation).statement;
}

TypeInfo CppTypeResolver::memberType(const TypeInfo &receiver, const QString &memberName,
                                     EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
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

TypeInfo CppTypeResolver::invokeResult(const TypeInfo &receiver, const QString &memberName,
                                       EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    return invokeResult(classInstance(receiver, {}, {}, evaluation), memberName,
                        evaluationContext).type;
}

MemberEvalResult CppTypeResolver::invokeResult(const ClassInstance &receiver,
                                               const QString &memberName,
                                               EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    ResolveOriginScope originScope("invokeResult");
    MemberResolution member = findMember(receiver, memberName, evaluation);
    if (!isInvocable(member.statement))
        return {};
    ClassInstance result = resolveTypeInstance(member.statement->typeInfo, member.owner.bindings,
                                               member.statement->parentScope.lock(), evaluation);
    return {member.statement, result.type, result};
}

MemberEvalResult CppTypeResolver::resolveMember(const TypeInfo &receiver,
                                                const QString &memberName, bool call,
                                                const QVector<TypeInfo> &arguments,
                                                EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    return resolveMember(resolveInstance(receiver, {}, {}, &evaluation), memberName, call,
                         arguments, &evaluation);
}

MemberEvalResult CppTypeResolver::resolveMember(const ClassInstance &receiver,
                                                const QString &memberName, bool call,
                                                const QVector<TypeInfo> &arguments,
                                                EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    ResolveOriginScope originScope("resolveMember");
    if (call) {
        QSet<QString> visited;
        QList<MemberResolution> candidates = memberCandidates(receiver, memberName,
                                                              visited, evaluation);
        for (int templateRank = 0; templateRank < 2; ++templateRank) {
            MemberEvalResult result;
            int matchCount = 0;
            for (const MemberResolution &member : candidates) {
                int candidateRank = member.statement->templateParameters.isEmpty() ? 0 : 1;
                if (candidateRank != templateRank || !isInvocable(member.statement))
                    continue;
                ClassInstance owner = member.owner;
                PStatement declaration = member.statement->parentScope.lock();
                if (declaration && declaration != owner.declaration) {
                    owner.declaration = declaration;
                    owner.type.declaration = declaration;
                    owner.type.qualifiedName = declaration->fullName;
                    owner.bindings = declarationBindings(declaration, owner.type,
                                                         owner.bindings->parent, evaluation);
                }
                ClassInstance resultInstance;
                TypeInfo type = invokeFunctionCandidate(member.statement, arguments, {},
                                                        evaluation, owner.bindings, &resultInstance);
                if (type.qualifiedName.isEmpty())
                    continue;
                result = {member.statement, type, resultInstance};
                if (++matchCount > 1)
                    return {};
            }
            if (matchCount == 1)
                return result;
        }
        return {};
    }
    MemberResolution member = findMember(receiver, memberName, evaluation);
    auto makeResult = [&](const TypeInfo &type) {
        PStatement context = member.statement ? member.statement->parentScope.lock()
                                              : receiver.declaration;
        ClassInstance resultInstance = classInstanceFromResolvedType(
                    type, member.owner.bindings, context, evaluation);
        if (resultInstance.type.qualifiedName.isEmpty()) {
            resultInstance.type = type;
            resultInstance.declaration = type.declaration.lock();
        }
        return MemberEvalResult{member.statement, type, resultInstance};
    };
    TypeInfo trait = traitMemberType(receiver, memberName, evaluation);
    if (!trait.qualifiedName.isEmpty())
        return makeResult(trait);
    if (!member.statement)
        return {};
    ClassInstance result = resolveTypeInstance(member.statement->typeInfo, member.owner.bindings,
                                               member.statement->parentScope.lock(), evaluation);
    return {member.statement, result.type, result};
}

TypeInfo CppTypeResolver::invokeFunction(
        const PStatement &function, const QVector<TypeInfo> &arguments,
        const QVector<TemplateArgument> &explicitArguments,
        EvaluationContext *evaluationContext, ClassInstance *resultInstance) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    TypeInfo result;
    ClassInstance instance;
    int matchCount = 0;
    for (const PStatement &candidate : functionCandidates(function)) {
        ClassInstance currentInstance;
        TypeInfo current = invokeFunctionCandidate(candidate, arguments, explicitArguments,
                                                   evaluation, {}, &currentInstance);
        if (current.qualifiedName.isEmpty())
            continue;
        ++matchCount;
        result = current;
        instance = currentInstance;
    }
    if (matchCount == 1 && resultInstance)
        *resultInstance = instance;
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
        const QVector<TemplateArgument> &explicitArguments,
        EvaluationContext &evaluation,
        const PTemplateBindingFrame &outerBindings,
        ClassInstance *resultInstance) const
{
    if (!function)
        return {};
    traceEvent(resolverProfile.functionTrace, function->fullName + function->normalizedArgs);
    ResolveOriginScope originScope("invokeFunctionCandidate");
    auto frame = std::make_shared<TemplateBindingFrame>();
    frame->owner = function.get();
    frame->parent = outerBindings;
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
                              frame->values, evaluation);
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
    ClassInstance result = resolveTypeInstance(function->typeInfo, frame,
                                               function->parentScope.lock(), evaluation);
    if (resultInstance)
        *resultInstance = result;
    return result.type;
}

bool CppTypeResolver::compatibleArgument(
        const TypeInfo &parameter, const TypeInfo &argument,
        const PTemplateBindingFrame &bindings, const PStatement &context,
        EvaluationContext &evaluation) const
{
    ResolveOriginScope originScope("compatibleArgument");
    ClassInstance formalInstance = resolveTypeInstance(parameter, bindings, context, evaluation);
    const TypeInfo &formal = formalInstance.type;
    auto pointerLike = [](const TypeInfo &type) {
        for (const TypeDerivation &derivation : type.derivations) {
            if (derivation.kind == TypeDerivationKind::Pointer
                    || derivation.kind == TypeDerivationKind::MemberPointer) {
                return true;
            }
        }
        return false;
    };
    if (pointerLike(formal) == pointerLike(argument))
        return true;
    if (!pointerLike(argument))
        return false;
    if (!formalInstance.declaration)
        return false;
    for (const std::weak_ptr<Statement> &weak : formalInstance.declaration->children) {
        PStatement constructor = weak.lock();
        if (!constructor || constructor->kind != StatementKind::Constructor
                || constructor->parameters.isEmpty()
                || constructor->declSpecifierProperties.testFlag(DeclSpecifierProperty::Explicit)
                || !constructor->parameters.first().type) {
            continue;
        }
        bool defaultsMatch = true;
        for (int i = 1; i < constructor->parameters.count(); ++i) {
            if (constructor->parameters[i].defaultArgumentText.isEmpty()) {
                defaultsMatch = false;
                break;
            }
        }
        if (!defaultsMatch)
            continue;
        const TypeInfo &constructorParameter = *constructor->parameters.first().type;
        if (pointerLike(constructorParameter))
            return true;
        PStatement parameterDeclaration = constructorParameter.declaration.lock();
        if (!parameterDeclaration) {
            parameterDeclaration = findDeclaration(constructorParameter.qualifiedName,
                                                   constructor->parentScope.lock(), true);
        }
        if (parameterDeclaration && parameterDeclaration->kind == StatementKind::Class)
            continue;
        TypeInfo constructorFormal = resolveType(constructorParameter, formalInstance.bindings,
                                                 constructor->parentScope.lock(), evaluation);
        if (pointerLike(constructorFormal) == pointerLike(argument))
            return true;
    }
    return false;
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
                                        TemplateBindings &bindings,
                                        EvaluationContext &evaluation) const
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
        bindParameterType(formal, actual, templateParameters, bindings, evaluation);
        return;
    }
    if (formal.qualifiedName != actual.qualifiedName)
        return;
    materializeTemplateArguments(formal, evaluation);
    materializeTemplateArguments(actual, evaluation);
    int count = qMin(formal.templateArguments.count(), actual.templateArguments.count());
    for (int i = 0; i < count; ++i) {
        const TemplateArgument &formalArgument = formal.templateArguments[i];
        const TemplateArgument &actualArgument = actual.templateArguments[i];
        if (formalArgument.kind == TemplateArgumentKind::Type && formalArgument.type
                && actualArgument.kind == TemplateArgumentKind::Type && actualArgument.type)
            bindParameterType(*formalArgument.type, *actualArgument.type, templateParameters,
                              bindings, evaluation);
    }
}

TypeInfo CppTypeResolver::resolveType(const TypeInfo &type,
                                      const PTemplateBindingFrame &bindings,
                                      const PStatement &context,
                                      EvaluationContext &evaluation) const
{
    return resolveTypeInstance(type, bindings, context, evaluation).type;
}

CppTypeResolver::ClassInstance CppTypeResolver::resolveTypeInstance(
        const TypeInfo &type, const PTemplateBindingFrame &bindings,
        const PStatement &context, EvaluationContext &evaluation) const
{
    if (qEnvironmentVariableIsSet("REDPANDA_PARSER_PROFILE_RESET_RESOLVER")) {
        resolverProfile.reset();
        qunsetenv("REDPANDA_PARSER_PROFILE_RESET_RESOLVER");
    }
    ResolverProfileScope profileScope(resolverProfile.resolveTypeNs,
                                      resolverProfile.resolveTypeCalls, "resolveType");
    if (++evaluation.operationCount > 4096)
        return {};
    QString resolutionKey = (context ? context->fullName : QString()) + "|" + profiledTypeString(type)
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
    traceResolveType(text, context, bindings);
    if (isBuiltinType(type)) {
        TypeInfo result = type;
        result.declaration.reset();
        return {result, {}, {}};
    }
    QVector<TypeNameSegment> segments;
    bool globallyQualified = false;
    if (!parseTypeNameSegments(text, segments, globallyQualified, evaluation) || segments.isEmpty())
        return {};
    if (segments.count() == 1 && segments.first().arguments.isEmpty()) {
        const QString &name = segments.first().name;
        PTemplateBindingFrame bindingFrame = bindings;
        const TemplateArgument *binding = nullptr;
        ClassInstance result;
        bool hasResolvedValue = false;
        for (; bindingFrame; bindingFrame = bindingFrame->parent) {
            auto argument = bindingFrame->values.constFind(name);
            if (argument == bindingFrame->values.constEnd())
                continue;
            binding = &argument.value();
            auto resolved = bindingFrame->resolvedValues.constFind(name);
            if (resolved != bindingFrame->resolvedValues.constEnd()) {
                result = resolved.value();
                hasResolvedValue = true;
            }
            break;
        }
        if (binding) {
            if (binding->kind != TemplateArgumentKind::Type || !binding->type)
                return {};
            if (binding->type->toString() == profiledTypeString(type))
                return {};
            if (!hasResolvedValue) {
                result = resolveTypeInstance(*binding->type, bindings, context, evaluation);
                if (!result.type.qualifiedName.isEmpty())
                    bindingFrame->resolvedValues.insert(name, result);
            }
            applyReference(result.type, type);
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

CppTypeResolver::ClassInstance CppTypeResolver::resolveSegments(
        const TypeInfo &reference, const QVector<TypeNameSegment> &segments,
        bool globallyQualified, const PTemplateBindingFrame &bindings,
        const PStatement &context, EvaluationContext &evaluation) const
{
    ResolverProfileScope profileScope(resolverProfile.segmentsNs, resolverProfile.segmentsCalls,
                                      "resolveSegments");
    ResolveOriginScope originScope("resolveSegments");
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
        for (int argumentIndex = 0; argumentIndex < instance.templateArguments.count();
             ++argumentIndex) {
            TemplateArgument &argument = instance.templateArguments[argumentIndex];
            if (argumentIndex < declaration->templateParameters.count()
                    && declaration->templateParameters[argumentIndex].kind
                    == TemplateParameterKind::NonType) {
                if (argument.constantExpression.isEmpty() && argument.type) {
                    argument.constantExpression = argument.type->qualifiedNameText.isEmpty()
                            ? argument.type->qualifiedName : argument.type->qualifiedNameText;
                }
                argument.kind = TemplateArgumentKind::NonType;
                argument.type.reset();
            }
            if (argument.kind == TemplateArgumentKind::NonType) {
                argument.constantExpression = resolveConstantExpression(
                        argument.constantExpression, bindings, context, evaluation);
                continue;
            }
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
                                                    0, 16).arg(profiledTypeString(instance));
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
            ClassInstance target = resolveTypeInstance(targetType, localBindings,
                                                       declaration->parentScope.lock(), evaluation);
            evaluation.aliases.remove(aliasKey);
            if (target.type.qualifiedName.isEmpty())
                return {};
            if (index + 1 == segments.count()) {
                applyReference(target.type, reference);
                return target;
            }
            MemberResolution member = findMember(target, segments[index + 1].name,
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
            return {instance, declaration, environment};
        }

        if (declaration->kind == StatementKind::Class && index + 2 == segments.count()) {
            TypeInfo trait = traitMemberType({instance, declaration, environment},
                                             segments[index + 1].name, evaluation);
            if (!trait.qualifiedName.isEmpty()) {
                ClassInstance result = classInstanceFromResolvedType(trait, environment, context,
                                                                     evaluation);
                if (result.type.qualifiedName.isEmpty()) {
                    result.type = trait;
                    result.declaration = trait.declaration.lock();
                }
                applyReference(result.type, reference);
                return result;
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
    return {unresolved, {}, {}};
}

bool CppTypeResolver::parseTypeNameSegments(const QString &text,
                                            QVector<TypeNameSegment> &segments,
                                            bool &globallyQualified,
                                            EvaluationContext &evaluation,
                                            bool materializeArguments) const
{
    const auto &cache = materializeArguments ? evaluation.parsedTypeNames
                                             : evaluation.parsedTypePatterns;
    auto cached = cache->constFind(text);
    traceSyntaxAttempt(materializeArguments ? "parseTypeNameSegments/eager"
                                            : "parseTypeNameSegments/lazy",
                       text, cached != cache->constEnd());
    if (cached != cache->constEnd()) {
        segments = cached->segments;
        globallyQualified = cached->globallyQualified;
        return !segments.isEmpty();
    }
    ResolverProfileScope profileScope(resolverProfile.parseSegmentsNs,
                                      resolverProfile.parseSegmentsCalls, "parseTypeNameSegments");
    traceTokenizeOrigin("parseTypeNameSegments");
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
        if (!materializeArguments) {
            TemplateArgument result;
            result.kind = TemplateArgumentKind::Type;
            result.constantExpression = argumentText.trimmed();
            return result;
        }
        TypeInfo argumentType = parseTypeText(argumentText, evaluation);
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
    if (segments.isEmpty())
        return false;
    cache->insert(text, {segments, globallyQualified});
    return true;
}

void CppTypeResolver::materializeTemplateArguments(TypeInfo &type,
                                                   EvaluationContext &evaluation) const
{
    ResolverProfileScope profileScope(resolverProfile.materializeNs,
                                      resolverProfile.materializeCalls, "materialize");
    if (!type.templateArguments.isEmpty() || !type.qualifiedNameText.contains('<'))
        return;
    QVector<TypeNameSegment> segments;
    bool globallyQualified = false;
    if (!parseTypeNameSegments(type.qualifiedNameText, segments, globallyQualified, evaluation))
        return;
    for (const TypeNameSegment &segment : segments) {
        if (segment.hasTemplateArguments) {
            type.templateArguments = segment.arguments;
            return;
        }
    }
}

void CppTypeResolver::materializeTemplateArgumentPatterns(
        TypeInfo &type, EvaluationContext &evaluation) const
{
    if (!type.templateArguments.isEmpty() || !type.qualifiedNameText.contains('<'))
        return;
    QVector<TypeNameSegment> segments;
    bool globallyQualified = false;
    if (!parseTypeNameSegments(type.qualifiedNameText, segments, globallyQualified, evaluation,
                               false)) {
        return;
    }
    for (const TypeNameSegment &segment : segments) {
        if (segment.hasTemplateArguments) {
            type.templateArguments = segment.arguments;
            return;
        }
    }
}

TypeInfo CppTypeResolver::parseTypeText(const QString &text, EvaluationContext &evaluation,
                                        bool materializeArguments) const
{
    auto cached = evaluation.parsedTypeTexts->constFind(text);
    traceSyntaxAttempt("parseTypeText", text, cached != evaluation.parsedTypeTexts->constEnd());
    if (cached != evaluation.parsedTypeTexts->constEnd())
        return cached.value();
    bool simpleNameCandidate = !text.isEmpty()
            && (text.front().isLetter() || text.front() == '_' || text.front() == ':')
            && (text.back().isLetterOrNumber() || text.back() == '_' || text.back() == '>')
            && text != "false" && text != "true" && text != "nullptr";
    if (simpleNameCandidate) {
        QVector<TypeNameSegment> segments;
        bool globallyQualified = false;
        if (parseTypeNameSegments(text, segments, globallyQualified, evaluation,
                                  materializeArguments)) {
            bool simpleType = true;
            TypeQualifiers qualifiers = TypeQualifier::None;
            QStringList names;
            for (int index = 0; index < segments.count(); ++index) {
                QString name = segments[index].name;
                if (index == 0) {
                    bool qualifier = true;
                    while (qualifier) {
                        if (name.startsWith("const ")) {
                            qualifiers.setFlag(TypeQualifier::Const);
                            name = name.mid(6);
                        } else if (name.startsWith("volatile ")) {
                            qualifiers.setFlag(TypeQualifier::Volatile);
                            name = name.mid(9);
                        } else {
                            qualifier = false;
                        }
                    }
                }
                simpleType &= !name.isEmpty() && !name.startsWith("decltype")
                        && (!name.contains(' ') || isBuiltinTypeName(name));
                names.append(name);
            }
            if (simpleType) {
                TypeInfo result;
                result.qualifiedName = (globallyQualified ? "::" : "") + names.join("::");
                result.qualifiedNameText = text;
                result.qualifiers = qualifiers;
                evaluation.parsedTypeTexts->insert(text, result);
                return result;
            }
        }
    }
    ResolverProfileScope profileScope(resolverProfile.parseTypeTextNs,
                                      resolverProfile.parseTypeTextCalls, "parseTypeText");
    traceTokenizeOrigin("parseTypeText");
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
    if (!result.qualifiedName.isEmpty())
        evaluation.parsedTypeTexts->insert(text, result);
    return result;
}

CppTypeResolver::ClassInstance CppTypeResolver::classInstance(
        const TypeInfo &type, const PTemplateBindingFrame &bindings,
        const PStatement &context, EvaluationContext &evaluation) const
{
    ResolveOriginScope originScope("classInstance");
    return resolveTypeInstance(type, bindings, context, evaluation);
}

CppTypeResolver::ClassInstance CppTypeResolver::classInstanceFromResolvedType(
        const TypeInfo &type, const PTemplateBindingFrame &bindings,
        const PStatement &context, EvaluationContext &evaluation) const
{
    ResolverProfileScope profileScope(resolverProfile.classInstanceNs,
                                      resolverProfile.classInstanceCalls, "classInstance");
    ClassInstance result;
    result.type = type;
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
    ResolverProfileScope profileScope(resolverProfile.specializationNs,
                                      resolverProfile.specializationCalls, "selectSpecialization");
    if (!primary.declaration || primary.declaration->kind != StatementKind::Class
            || primary.declaration->templateKind != TemplateDeclarationKind::Primary) {
        return primary;
    }
    QString instanceText = profiledTypeString(primary.type);
    QString cacheKey = QString::number(reinterpret_cast<quintptr>(primary.declaration.get()), 16) + "|"
            + QString::number(reinterpret_cast<quintptr>(primary.bindings.get()), 16) + "|" + instanceText;
    auto cached = evaluation.selectedSpecializations->constFind(cacheKey);
    traceSyntaxAttempt("selectSpecialization", cacheKey,
                       cached != evaluation.selectedSpecializations->constEnd());
    if (cached != evaluation.selectedSpecializations->constEnd())
        return cached.value();
    auto cacheResult = [&](const ClassInstance &selected) {
        if (selected.declaration && evaluation.operationCount <= 4096 && !evaluation.resolutionFailed)
            evaluation.selectedSpecializations->insert(cacheKey, selected);
        return selected;
    };
    traceSpecialization(primary.declaration->fullName + "|" + instanceText);
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
                                                            parent, bindings, evaluation);
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
                return cacheResult(result);
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
    return cacheResult(result);
}

CppTypeResolver::SpecializationMatch CppTypeResolver::matchSpecialization(
        const PStatement &specialization, const QVector<TemplateArgument> &arguments,
        const PStatement &context, TemplateBindings &bindings,
        EvaluationContext &evaluation) const
{
    traceEvent(resolverProfile.specializationAttemptTrace, specialization->command);
    TypeInfo pattern = specialization->templatePattern;
    Q_ASSERT(!pattern.qualifiedName.isEmpty());
    if (pattern.qualifiedName.isEmpty())
        return SpecializationMatch::Unknown;
    if (pattern.templateArguments.count() != arguments.count())
        return SpecializationMatch::NoMatch;
    QSet<QString> parameters;
    for (const TemplateParameterInfo &parameter : specialization->templateParameters)
        parameters.insert(parameter.name);
    SpecializationMatch combined = SpecializationMatch::Match;
    for (int i = 0; i < arguments.count(); ++i) {
        SpecializationMatch match = matchArgument(pattern.templateArguments[i], arguments[i],
                                                  parameters, bindings, context, evaluation);
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
        const PStatement &context, EvaluationContext &evaluation) const
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
    TemplateArgument resolvedPattern = pattern;
    if (actual.kind == TemplateArgumentKind::Type && actual.type
            && pattern.kind == TemplateArgumentKind::Type && !pattern.type
            && !patternText.isEmpty()) {
        TypeInfo patternType = parseTypeText(patternText, evaluation, false);
        if (!patternType.qualifiedName.isEmpty()) {
            resolvedPattern = TemplateArgument::fromType(patternType);
            resolvedPattern.constantExpression = patternText;
        }
    }
    if (resolvedPattern.kind == TemplateArgumentKind::Type
            && actual.kind == TemplateArgumentKind::Type
            && resolvedPattern.type && actual.type) {
        return matchType(*resolvedPattern.type, *actual.type, parameters, bindings, context,
                         evaluation);
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
        TemplateBindings &bindings, const PStatement &context, EvaluationContext &evaluation) const
{
    TypeInfo formal = pattern;
    TypeInfo real = actual;
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

    if (formal.qualifiedName != real.qualifiedName) {
        if (isBuiltinType(formal) || isBuiltinType(real))
            return SpecializationMatch::NoMatch;
        PStatement formalDeclaration = findDeclaration(formal.qualifiedName, context, true);
        PStatement realDeclaration = real.declaration.lock();
        if (!formalDeclaration || !realDeclaration || formalDeclaration != realDeclaration)
            return SpecializationMatch::NoMatch;
    }
    materializeTemplateArgumentPatterns(formal, evaluation);
    materializeTemplateArguments(real, evaluation);
    if (formal.templateArguments.count() != real.templateArguments.count())
        return SpecializationMatch::NoMatch;
    SpecializationMatch combined = SpecializationMatch::Match;
    for (int i = 0; i < formal.templateArguments.count(); ++i) {
        SpecializationMatch match = matchArgument(formal.templateArguments[i],
                                                  real.templateArguments[i], parameters,
                                                  bindings, context, evaluation);
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
    const QList<MemberResolution> candidates = memberCandidates(instance, name, visited,
                                                                evaluation, true);
    return candidates.isEmpty() ? MemberResolution{} : candidates.first();
}

QList<CppTypeResolver::MemberResolution> CppTypeResolver::memberCandidates(
        const ClassInstance &instance, const QString &name, QSet<QString> &visited,
        EvaluationContext &evaluation, bool stopAtDirectMember) const
{
    ResolverProfileScope profileScope(resolverProfile.memberLookupNs,
                                      resolverProfile.memberLookupCalls, "memberCandidates");
    QList<MemberResolution> result;
    if (!instance.declaration)
        return result;
    traceEvent(resolverProfile.memberTrace, instance.declaration->fullName + "::" + name);
    QString instanceKey = instance.declaration->fullName + "<" + profiledTypeString(instance.type) + ">";
    if (visited.contains(instanceKey))
        return result;
    visited.insert(instanceKey);
    const auto candidates = instance.declaration->children.values(name);
    QList<MemberResolution> constFallback;
    for (const std::weak_ptr<Statement> &weak : candidates) {
        PStatement candidate = weak.lock();
        if (!candidate)
            continue;
        bool qualifiedMember = (candidate->kind == StatementKind::Function
                                || candidate->kind == StatementKind::OverloadedOperator)
                && !candidate->isStatic();
        if (qualifiedMember
                && candidate->properties.testFlag(StatementProperty::RvalueReference)) {
            continue;
        }
        bool constMember = candidate->properties.testFlag(StatementProperty::Const);
        if (qualifiedMember && instance.type.qualifiers.testFlag(TypeQualifier::Const)
                && !constMember) {
            continue;
        }
        if (qualifiedMember && constMember
                && !instance.type.qualifiers.testFlag(TypeQualifier::Const)) {
            constFallback.append({candidate, instance, candidate->accessibility});
        } else {
            result.append({candidate, instance, candidate->accessibility});
        }
    }
    if (result.isEmpty())
        result = constFallback;
    if (stopAtDirectMember && !result.isEmpty())
        return result;

    QSet<QString> signatures;
    for (const MemberResolution &member : result)
        signatures.insert(member.statement->command + member.statement->normalizedArgs);
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
        TypeInfo baseType = parseTypeText(baseText, evaluation);
        if (baseType.qualifiedName.isEmpty())
            continue;
        ClassInstance base = classInstance(baseType, instance.bindings,
                                           instance.declaration->parentScope.lock(), evaluation);
        const QList<MemberResolution> baseMembers = memberCandidates(base, name, visited, evaluation);
        for (MemberResolution member : baseMembers) {
            QString signature = member.statement->command + member.statement->normalizedArgs;
            if (signatures.contains(signature))
                continue;
            signatures.insert(signature);
            member.accessibility = applyInheritance(member.accessibility, inheritance);
            result.append(member);
        }
    }
    return result;
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
    ResolverProfileScope profileScope(resolverProfile.declarationLookupNs,
                                      resolverProfile.declarationLookupCalls, "findDeclaration");
    ResolveOriginScope originScope("findDeclaration(" + qualifiedName + ")");
    traceDeclaration(qualifiedName, context, typesOnly);
    QString name = qualifiedName;
    PStatement initialContext = context;
    if (name.startsWith("::")) {
        name.remove(0, 2);
        initialContext.reset();
    }
    const QStringList parts = name.split("::", Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return nullptr;
    bool cacheEnabled = mEvaluation && !mEvaluation->allowModelChanges;
    QString cacheKey;
    if (cacheEnabled) {
        cacheKey = QString::number(reinterpret_cast<quintptr>(initialContext.get()), 16) + "|"
                + name + "|" + (typesOnly ? "types" : "all");
        auto cached = mEvaluation->declarationResults->constFind(cacheKey);
        traceSyntaxAttempt("findDeclaration", cacheKey,
                           cached != mEvaluation->declarationResults->constEnd());
        if (cached != mEvaluation->declarationResults->constEnd())
            return cached.value();
    }
    auto finish = [&](PStatement result) {
        if (cacheEnabled)
            mEvaluation->declarationResults->insert(cacheKey, result);
        return result;
    };

    PStatement searchScope = initialContext;
    while (searchScope) {
        if (typesOnly && searchScope->kind == StatementKind::Class
                && searchScope->command.section('<', 0, 0) == parts.first()) {
            return finish(findDeclarationRemainder(searchScope, parts, typesOnly));
        }
        if (searchScope->kind == StatementKind::Namespace) {
            bool firstPartFound = false;
            PStatement result = findInNamespaceScope(searchScope, parts, firstPartFound, typesOnly);
            if (result || firstPartFound)
                return finish(result);
        } else if (PStatement current = findMember(searchScope, parts.first(), typesOnly)) {
            return finish(findDeclarationRemainder(current, parts, typesOnly));
        }
        searchScope = searchScope->parentScope.lock();
    }
    bool firstPartFound = false;
    return finish(findInNamespaceScope({}, parts, firstPartFound, typesOnly));
}

PStatement CppTypeResolver::findInNamespaceScope(const PStatement &scope,
                                                 const QStringList &parts,
                                                 bool &firstPartFound, bool typesOnly) const
{
    ResolverProfileScope profileScope(resolverProfile.namespaceLookupNs,
                                      resolverProfile.namespaceLookupCalls, "findInNamespaceScope");
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
    ResolverProfileScope profileScope(resolverProfile.declarationRemainderNs,
                                      resolverProfile.declarationRemainderCalls,
                                      "findDeclarationRemainder");
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
    ResolverProfileScope profileScope(resolverProfile.memberDeclarationNs,
                                      resolverProfile.memberDeclarationCalls, "findMember");
    if (resolverProfile.traceEnabled) {
        QString origin = resolverProfile.resolveOriginStack.isEmpty()
                ? "external" : resolverProfile.resolveOriginStack.join(">");
        QString key = origin + "|" + QString::number(reinterpret_cast<quintptr>(scope.get()), 16)
                + "|" + (scope ? scope->fullName : QString()) + "|" + name + "|"
                + (typesOnly ? "types" : "all");
        ++resolverProfile.memberDeclarationTrace[key];
    }
    bool cacheEnabled = mEvaluation && !mEvaluation->allowModelChanges;
    QString cacheKey;
    if (cacheEnabled) {
        cacheKey = QString::number(reinterpret_cast<quintptr>(scope.get()), 16) + "|"
                + name + "|" + (typesOnly ? "types" : "all");
        auto cached = mEvaluation->memberResults->constFind(cacheKey);
        traceSyntaxAttempt("findMember", cacheKey, cached != mEvaluation->memberResults->constEnd());
        if (cached != mEvaluation->memberResults->constEnd())
            return cached.value();
    }
    auto finish = [&](PStatement result) {
        if (cacheEnabled)
            mEvaluation->memberResults->insert(cacheKey, result);
        return result;
    };
    QSet<const Statement *> visited;
    QStack<PStatement> pending;
    pending.push(scope);
    while (!pending.isEmpty()) {
        PStatement current = pending.pop();
        if (!current || visited.contains(current.get()))
            continue;
        visited.insert(current.get());
        ResolverProfileScope profileScope(resolverProfile.memberDfsNs,
                                          resolverProfile.memberDfsCalls, "memberDfs");
        const auto directCandidates = current->children.values(name);
        for (int i = directCandidates.count() - 1; i >= 0; --i) {
            PStatement candidate = directCandidates[i].lock();
            if (!candidate || (typesOnly && !isTypeDeclaration(candidate)))
                continue;
            if (candidate->kind != StatementKind::Namespace || candidate->command == name)
                return finish(candidate);
            pending.push(candidate);
        }
        if (current->kind == StatementKind::Namespace) {
            const QList<PStatement> transparentNamespaces =
                    mModel.transparentNamespaceStatements(current->fullName);
            for (int i = transparentNamespaces.count() - 1; i >= 0; --i)
                pending.push(transparentNamespaces[i]);
        }
        QList<PStatement> bases;
        for (QString baseName : current->baseClasses) {
            baseName = baseName.trimmed();
            for (const QString &modifier : {QString("public"), QString("protected"),
                                           QString("private"), QString("virtual")}) {
                if (baseName.startsWith(modifier + ' '))
                    baseName = baseName.mid(modifier.length()).trimmed();
            }
            int templateStart = baseName.indexOf('<');
            if (templateStart >= 0)
                baseName.truncate(templateStart);
            if (PStatement base = findDeclaration(baseName, current->parentScope.lock(), typesOnly))
                bases.append(base);
        }
        for (int i = bases.count() - 1; i >= 0; --i)
            pending.push(bases[i]);
    }
    return finish(nullptr);
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
    ResolveOriginScope originScope("resolveConstantExpression");
    QString expression = text.trimmed();
    QSet<QString> substitutedNames;
    while (const TemplateArgument *binding = findBinding(bindings, expression)) {
        if (binding->kind != TemplateArgumentKind::NonType
                || binding->constantExpression.isEmpty()
                || substitutedNames.contains(expression)) {
            break;
        }
        substitutedNames.insert(expression);
        expression = binding->constantExpression.trimmed();
    }
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
    TypeInfo traitType = parseTypeText(expression, evaluation);
    if (traitType.qualifiedName.isEmpty())
        return text.trimmed();
    EvaluationContext traitEvaluation = evaluation;
    ClassInstance trait = classInstance(traitType, bindings, context, traitEvaluation);
    if (!trait.declaration || trait.type.templateArguments.isEmpty()
            || trait.type.templateArguments.first().kind != TemplateArgumentKind::Type
            || !trait.type.templateArguments.first().type) {
        return text.trimmed();
    }
    const TypeInfo &argument = *trait.type.templateArguments.first().type;
    if (trait.declaration->fullName == "std::integral_constant"
            && trait.type.templateArguments.count() == 2
            && trait.type.templateArguments[1].kind == TemplateArgumentKind::NonType) {
        return trait.type.templateArguments[1].constantExpression;
    }
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
    traceEvent(resolverProfile.argumentTrace,
               (bindings && bindings->owner ? bindings->owner->fullName : QString()) + "|" + text);
    ResolveOriginScope originScope("resolveArgument");
    if (kind != TemplateParameterKind::Type) {
        if (const TemplateArgument *binding = findBinding(bindings, text.trimmed()))
            return *binding;
        return TemplateArgument::fromConstant(resolveConstantExpression(
                text, bindings, context, evaluation));
    }

    TypeInfo type = parseTypeText(text, evaluation);
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
    ResolverProfileScope profileScope(resolverProfile.bindingsNs, resolverProfile.bindingsCalls,
                                      "declarationBindings");
    ResolveOriginScope originScope("declarationBindings");
    if (!declaration) {
        auto result = std::make_shared<TemplateBindingFrame>();
        result->parent = outerBindings;
        return result;
    }
    PStatement declarationScope = declaration->parentScope.lock();
    PTemplateBindingFrame lexicalParent;
    if (declarationScope && declarationScope->kind != StatementKind::Namespace)
        lexicalParent = outerBindings;
    QString instanceText = profiledTypeString(instance);
    QString cacheKey = QString::number(reinterpret_cast<quintptr>(declaration.get()), 16) + "|"
            + QString::number(reinterpret_cast<quintptr>(lexicalParent.get()), 16) + "|" + instanceText;
    auto cached = evaluation.bindingFrames->constFind(cacheKey);
    traceSyntaxAttempt("declarationBindings", cacheKey, cached != evaluation.bindingFrames->constEnd());
    if (cached != evaluation.bindingFrames->constEnd())
        return cached.value();
    auto result = std::make_shared<TemplateBindingFrame>();
    result->owner = declaration.get();
    result->parent = lexicalParent;
    traceEvent(resolverProfile.bindingTrace,
               declaration->fullName + "|" + instanceText + "|"
               + (outerBindings && outerBindings->owner ? outerBindings->owner->fullName : QString()));

    TypeInfo materialized = instance;
    materializeTemplateArguments(materialized, evaluation);
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
    if (evaluation.operationCount <= 4096 && !evaluation.resolutionFailed)
        evaluation.bindingFrames->insert(cacheKey, result);
    return result;
}
