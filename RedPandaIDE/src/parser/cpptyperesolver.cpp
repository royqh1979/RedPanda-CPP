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
#include "cppexpressionevaluator.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QStack>
#include <algorithm>
#include <limits>

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
    qint64 memberCacheNs = 0;
    qint64 memberCacheCalls = 0;
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
        memberCacheNs = memberCacheCalls = 0;
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
                       << memberDfsNs / 1000000.0 << memberDfsCalls
                       << "memberCache" << memberCacheNs / 1000000.0 << memberCacheCalls;
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

QString lookupKeyText(const LookupKey &key)
{
    return QString::number(reinterpret_cast<quintptr>(key.scope), 16) + "|" + key.name + "|"
            + (key.typesOnly ? "types" : "all");
}

QString substituteIdentifier(QString text, const QString &name, const QString &replacement)
{
    CppTokenizer tokenizer;
    tokenizer.tokenize({text});
    for (int i = tokenizer.tokenCount() - 1; i >= 0; --i) {
        const auto &token = tokenizer[i];
        if (token.kind == CppTokenizer::TokenKind::Identifier && token.text == name)
            text.replace(token.position.ch, name.size(), replacement);
    }
    return text;
}

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

TypeInfo applyTypeBuiltin(const QString &name, TypeInfo result)
{
    if (name == "add_lvalue_reference" || name == "__add_lvalue_reference") {
        if (!result.derivations.isEmpty()
                && (result.derivations.first().kind == TypeDerivationKind::LvalueReference
                    || result.derivations.first().kind == TypeDerivationKind::RvalueReference)) {
            result.derivations.removeFirst();
        }
        result.derivations.prepend(TypeDerivation::lvalueReference());
    } else if (name == "add_rvalue_reference" || name == "__add_rvalue_reference") {
        if (!result.derivations.isEmpty()
                && (result.derivations.first().kind == TypeDerivationKind::LvalueReference
                    || result.derivations.first().kind == TypeDerivationKind::RvalueReference)) {
            result.derivations.removeFirst();
        }
        result.derivations.prepend(TypeDerivation::rvalueReference());
    } else if (name == "add_pointer" || name == "__add_pointer") {
        result.derivations.prepend(TypeDerivation::pointer());
    } else if (name == "remove_reference" || name == "__remove_reference_t") {
        if (!result.derivations.isEmpty()
                && (result.derivations.first().kind == TypeDerivationKind::LvalueReference
                    || result.derivations.first().kind == TypeDerivationKind::RvalueReference)) {
            result.derivations.removeFirst();
        }
    } else if (name == "remove_cv" || name == "__remove_cv" || name == "__remove_cv_t") {
        result.qualifiers = TypeQualifier::None;
        while (result.qualifiedName.startsWith("const ")
               || result.qualifiedName.startsWith("volatile ")) {
            result.qualifiedName.remove(0, result.qualifiedName.startsWith("const ") ? 6 : 9);
        }
        while (result.qualifiedName.endsWith(" const")
               || result.qualifiedName.endsWith(" volatile")) {
            result.qualifiedName.chop(result.qualifiedName.endsWith(" const") ? 6 : 9);
        }
    } else if ((name == "remove_extent" || name == "__remove_extent")
               && !result.derivations.isEmpty()
               && result.derivations.first().kind == TypeDerivationKind::Array) {
        result.derivations.removeFirst();
    }
    result.qualifiedNameText = result.qualifiedName;
    return result;
}

const QStringList &typeBuiltinsForCompiler(CppCompilerType compilerType)
{
    static const QStringList gcc;
    static const QStringList clang{
        "__add_lvalue_reference", "__add_rvalue_reference", "__add_pointer",
        "__remove_extent", "__remove_reference_t", "__remove_cv", "__remove_cv_t"
    };
    return compilerType == CppCompilerType::Clang ? clang : gcc;
}

const QStringList &constantBuiltinsForCompiler(CppCompilerType compilerType)
{
    static const QStringList gcc{"__is_array(", "__is_constructible("};
    static const QStringList clang{
        "__is_array(", "__is_constructible(", "__is_trivially_destructible(",
        "__is_trivially_constructible(", "__is_trivially_assignable(", "__is_trivially_copyable("
    };
    return compilerType == CppCompilerType::Clang ? clang : gcc;
}

const QSet<QString> &constantTraitsForCompiler(CppCompilerType compilerType)
{
    static const QSet<QString> gcc{
        "integral_constant", "is_array", "is_void", "is_reference",
        "is_lvalue_reference", "is_rvalue_reference", "is_convertible",
        "__is_nothrow_invocable", "is_nothrow_invocable", "__and_", "__or_", "__not_",
        "__is_fast_hash", "__all"
    };
    static const QSet<QString> clang{
        "integral_constant", "is_array", "is_void", "is_reference",
        "is_lvalue_reference", "is_rvalue_reference", "is_convertible", "__has_iterator_typedefs",
        "is_trivially_destructible", "is_trivially_constructible",
        "is_trivially_default_constructible", "is_trivially_copy_constructible",
        "is_trivially_move_constructible", "is_trivially_assignable",
        "is_trivially_copy_assignable", "is_trivially_move_assignable", "is_trivially_copyable",
        "is_copy_constructible", "is_move_constructible", "is_default_constructible",
        "__is_nothrow_invocable", "is_nothrow_invocable", "__and_", "__or_", "__not_",
        "__is_fast_hash", "__all"
    };
    return compilerType == CppCompilerType::Clang ? clang : gcc;
}

const QSet<QString> &typeTraitsForCompiler(CppCompilerType compilerType)
{
    static const QSet<QString> gcc{
        "add_lvalue_reference", "add_rvalue_reference", "add_pointer",
        "remove_extent", "remove_reference", "remove_cv"
    };
    static const QSet<QString> clang{
        "add_lvalue_reference", "add_rvalue_reference", "add_pointer",
        "remove_extent", "remove_reference", "remove_cv"
    };
    return compilerType == CppCompilerType::Clang ? clang : gcc;
}

bool isAlias(const PStatement &statement)
{
    return statement && (statement->kind == StatementKind::Alias
                         || statement->kind == StatementKind::InlineAlias
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
    case StatementKind::InlineAlias:
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
    case StatementKind::Constructor:
    case StatementKind::OverloadedOperator:
    case StatementKind::LiteralOperator:
        return true;
    default:
        return false;
    }
}

TypeInfo invocableReturnType(const PStatement &statement)
{
    auto normal = std::dynamic_pointer_cast<Statement>(statement);
    if (!normal)
        return {};
    if (statement->kind == StatementKind::Constructor) {
        PStatement parent = statement->parentScope.lock();
        if (!parent)
            return {};
        TypeInfo result;
        result.qualifiedName = parent->fullName;
        result.declaration = parent;
        return result;
    }
    TypeInfo result = normal->typeInfo;
    if (result.qualifiedName == "auto" && !normal->trailingReturnType.isEmpty()) {
        result.qualifiedName = normal->trailingReturnType;
        result.qualifiedNameText = normal->trailingReturnType;
        result.declaration.reset();
    }
    return result;
}

PStatement invocableReturnContext(const PStatement &statement)
{
    auto normal = std::dynamic_pointer_cast<Statement>(statement);
    return normal && !normal->trailingReturnType.isEmpty()
            ? statement : statement->parentScope.lock();
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

void appendConstraintToken(QString &target, const QString &text)
{
    if (!target.isEmpty() && !text.isEmpty()
            && (target.back().isLetterOrNumber() || target.back() == '_')
            && (text.front().isLetterOrNumber() || text.front() == '_')) {
        target += ' ';
    }
    target += text;
}

ConstraintSatisfaction constraintAnd(ConstraintSatisfaction left, ConstraintSatisfaction right)
{
    if (left == ConstraintSatisfaction::Unsatisfied || right == ConstraintSatisfaction::Unsatisfied)
        return ConstraintSatisfaction::Unsatisfied;
    if (left == ConstraintSatisfaction::Satisfied && right == ConstraintSatisfaction::Satisfied)
        return ConstraintSatisfaction::Satisfied;
    return ConstraintSatisfaction::Unknown;
}

ConstraintSatisfaction constraintOr(ConstraintSatisfaction left, ConstraintSatisfaction right)
{
    if (left == ConstraintSatisfaction::Satisfied || right == ConstraintSatisfaction::Satisfied)
        return ConstraintSatisfaction::Satisfied;
    if (left == ConstraintSatisfaction::Unsatisfied && right == ConstraintSatisfaction::Unsatisfied)
        return ConstraintSatisfaction::Unsatisfied;
    return ConstraintSatisfaction::Unknown;
}

class ConstraintParser {
public:
    using AtomEvaluator = std::function<ConstraintSatisfaction(const QString &)>;

    ConstraintParser(const QString &text, const AtomEvaluator &evaluateAtom)
        : mEvaluateAtom(evaluateAtom)
    {
        mTokenizer.tokenize(text.split('\n'));
    }

    ConstraintSatisfaction evaluate()
    {
        ConstraintSatisfaction result = parseOr();
        return mIndex == mTokenizer.tokenCount() ? result : ConstraintSatisfaction::Unknown;
    }

private:
    ConstraintSatisfaction parseOr()
    {
        ConstraintSatisfaction result = parseAnd();
        while (consume("||"))
            result = constraintOr(result, parseAnd());
        return result;
    }

    ConstraintSatisfaction parseAnd()
    {
        ConstraintSatisfaction result = parseUnary();
        while (consume("&&"))
            result = constraintAnd(result, parseUnary());
        return result;
    }

    ConstraintSatisfaction parseUnary()
    {
        if (consume("!")) {
            ConstraintSatisfaction result = parseUnary();
            if (result == ConstraintSatisfaction::Satisfied)
                return ConstraintSatisfaction::Unsatisfied;
            if (result == ConstraintSatisfaction::Unsatisfied)
                return ConstraintSatisfaction::Satisfied;
            return ConstraintSatisfaction::Unknown;
        }
        return parsePrimary();
    }

    ConstraintSatisfaction parsePrimary()
    {
        if (consume("(")) {
            ConstraintSatisfaction result = parseOr();
            return consume(")") ? result : ConstraintSatisfaction::Unknown;
        }
        if (consume("true"))
            return ConstraintSatisfaction::Satisfied;
        if (consume("false"))
            return ConstraintSatisfaction::Unsatisfied;

        QString atom;
        int angleLevel = 0;
        while (mIndex < mTokenizer.tokenCount()) {
            const CppTokenizer::Token &token = mTokenizer[mIndex];
            const QString &text = token.text;
            if (angleLevel == 0 && (text == ")" || text == "&&" || text == "||"))
                break;
            if (token.matchIndex > mIndex) {
                int end = qMin(token.matchIndex, mTokenizer.tokenCount() - 1);
                while (mIndex <= end) {
                    appendConstraintToken(atom, mTokenizer[mIndex].text);
                    ++mIndex;
                }
                continue;
            }
            if (text == "<") {
                ++angleLevel;
            } else if (text == ">") {
                angleLevel = qMax(0, angleLevel - 1);
            } else if (text == ">>") {
                angleLevel = qMax(0, angleLevel - 2);
            }
            appendConstraintToken(atom, text);
            ++mIndex;
        }
        return atom.isEmpty() ? ConstraintSatisfaction::Unknown : mEvaluateAtom(atom);
    }

    bool consume(const QString &text)
    {
        if (mIndex >= mTokenizer.tokenCount() || mTokenizer[mIndex].text != text)
            return false;
        ++mIndex;
        return true;
    }

    CppTokenizer mTokenizer;
    AtomEvaluator mEvaluateAtom;
    int mIndex = 0;
};

}

CppTypeResolver::EvaluationContext::EvaluationContext()
{
    const QByteArrayList disabled = qgetenv("REDPANDA_PARSER_DISABLE_CACHES").split(',');
    for (QByteArray name : disabled) {
        name = name.trimmed();
        if (name == "all")
            caches = CacheLevel::None;
        else if (name == "type-text")
            caches.setFlag(CacheLevel::ParsedTypeText, false);
        else if (name == "type-name")
            caches.setFlag(CacheLevel::ParsedTypeName, false);
        else if (name == "type-pattern")
            caches.setFlag(CacheLevel::ParsedTypePattern, false);
        else if (name == "binding-frame")
            caches.setFlag(CacheLevel::BindingFrame, false);
        else if (name == "resolved-value")
            caches.setFlag(CacheLevel::ResolvedValue, false);
        else if (name == "specialization")
            caches.setFlag(CacheLevel::SelectedSpecialization, false);
        else if (name == "declaration")
            caches.setFlag(CacheLevel::Declaration, false);
        else if (name == "member")
            caches.setFlag(CacheLevel::Member, false);
        else if (name == "resolved-type")
            caches.setFlag(CacheLevel::ResolvedType, false);
    }
}

CppTypeResolver::CppTypeResolver(const StatementModel &model, EvaluationContext *evaluation,
                                 const CppEvaluationOptions &options)
    : mModel(model), mEvaluation(evaluation), mOptions(options)
{
}

bool CppTypeResolver::supportsConstantTrait(const QString &traitName) const
{
    return mOptions.cppVersion >= CppVersion::Cpp11
            && constantTraitsForCompiler(mOptions.compilerType).contains(traitName);
}

bool CppTypeResolver::supportsTypeTrait(const QString &traitName) const
{
    return mOptions.cppVersion >= CppVersion::Cpp11
            && typeTraitsForCompiler(mOptions.compilerType).contains(traitName);
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

ClassInstance CppTypeResolver::resolveInContext(
        const TypeInfo &type, const PTemplateBindingFrame &bindings, const PStatement &context,
        EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    ClassInstance result = resolveTypeInstance(type, bindings, context, evaluation);
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
    ClassInstance instance = resolveTypeInstance(receiver, {}, {}, evaluation);
    return findMember(instance, memberName, evaluation).statement;
}

TypeInfo CppTypeResolver::memberType(const TypeInfo &receiver, const QString &memberName,
                                     EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    ClassInstance instance = resolveTypeInstance(receiver, {}, {}, evaluation);
    TypeInfo trait = traitMemberType(instance, memberName, evaluation);
    if (!trait.qualifiedName.isEmpty())
        return trait;
    MemberResolution member = findMember(instance, memberName, evaluation);
    if (!member.statement)
        return {};
    return resolveType(std::dynamic_pointer_cast<Statement>(member.statement)->typeInfo, member.owner.bindings,
                       member.statement->parentScope.lock(), evaluation);
}

TypeInfo CppTypeResolver::invokeResult(const TypeInfo &receiver, const QString &memberName,
                                       EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    return invokeResult(resolveTypeInstance(receiver, {}, {}, evaluation), memberName,
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
    ClassInstance result = resolveTypeInstance(std::dynamic_pointer_cast<Statement>(member.statement)->typeInfo, member.owner.bindings,
                                               member.statement->parentScope.lock(), evaluation);
    return {member.statement, result.type, result, {member.statement, member.owner.bindings}};
}

MemberEvalResult CppTypeResolver::memberAccess(
        const TypeInfo &receiver, const QString &memberName,
        EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    return memberAccess(resolveInstance(receiver, {}, {}, &evaluation), memberName, &evaluation);
}

MemberEvalResult CppTypeResolver::memberAccess(
        const ClassInstance &receiver, const QString &memberName,
        EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    return resolveMember(receiver, memberName, false, {}, evaluation);
}

MemberEvalResult CppTypeResolver::invokeMember(
        const TypeInfo &receiver, const QString &memberName, const QVector<TypeInfo> &arguments,
        EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    return invokeMember(resolveInstance(receiver, {}, {}, &evaluation), memberName,
                        arguments, &evaluation);
}

MemberEvalResult CppTypeResolver::invokeMember(
        const ClassInstance &receiver, const QString &memberName, const QVector<TypeInfo> &arguments,
        EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    return resolveMember(receiver, memberName, true, arguments, evaluation);
}

MemberEvalResult CppTypeResolver::resolveMember(const ClassInstance &receiver,
                                                const QString &memberName, bool call,
                                                const QVector<TypeInfo> &arguments,
                                                EvaluationContext &evaluation) const
{
    ResolveOriginScope originScope("resolveMember");
    if (call) {
        QSet<QString> visited;
        QList<MemberResolution> candidates = memberCandidates(receiver, memberName,
                                                              visited, evaluation);
        struct MatchedCandidate {
            PStatement statement;
            PTemplateBindingFrame bindings;
            int score;
        };
        QVector<MatchedCandidate> matches;
        for (const MemberResolution &member : candidates) {
            if (!isInvocable(member.statement))
                continue;
            ClassInstance owner = member.owner;
            PStatement declaration = member.statement->parentScope.lock();
            if (declaration && declaration != owner.declaration) {
                owner.declaration = declaration;
                owner.type.declaration = declaration;
                owner.type.qualifiedName = declaration->fullName;
                owner.bindings = declarationBindings(declaration, owner.type,
                                                     owner.bindings->parent, evaluation);
                if (!owner.bindings->complete)
                    continue;
            }
            int score = 0;
            PTemplateBindingFrame currentBindings = matchFunctionCandidate(
                        member.statement, arguments, {}, evaluation, owner.bindings, &score, false);
            if (currentBindings)
                matches.append({member.statement, currentBindings, score});
        }
        auto equivalentDeclaration = [&](const MatchedCandidate &left, const MatchedCandidate &right) {
            if (left.statement == right.statement)
                return true;
            if (left.statement->command != right.statement->command)
                return false;
            auto leftNormal = std::dynamic_pointer_cast<Statement>(left.statement);
            auto rightNormal = std::dynamic_pointer_cast<Statement>(right.statement);
            const StatementProperties qualifiers = StatementProperty::Const | StatementProperty::Volatile
                    | StatementProperty::LvalueReference | StatementProperty::RvalueReference;
            if (leftNormal->normalizedArgs.contains('@') || rightNormal->normalizedArgs.contains('@')
                    || (left.statement->properties & qualifiers) != (right.statement->properties & qualifiers)
                    || leftNormal->parameters.count() != rightNormal->parameters.count()) {
                return false;
            }
            for (int i = 0; i < leftNormal->parameters.count(); ++i) {
                const PTypeInfo &leftType = leftNormal->parameters[i].type;
                const PTypeInfo &rightType = rightNormal->parameters[i].type;
                if (!leftType || !rightType)
                    return leftType == rightType;
                TypeInfo resolvedLeft = resolveType(*leftType, left.bindings,
                                                    left.statement->parentScope.lock(), evaluation);
                TypeInfo resolvedRight = resolveType(*rightType, right.bindings,
                                                     right.statement->parentScope.lock(), evaluation);
                if (resolvedLeft.qualifiedName.isEmpty()
                        || resolvedLeft.toString() != resolvedRight.toString()) {
                    return false;
                }
            }
            return true;
        };
        int previousScore = std::numeric_limits<int>::max();
        while (true) {
            int currentScore = std::numeric_limits<int>::min();
            for (const MatchedCandidate &match : matches) {
                if (match.score < previousScore && match.score > currentScore)
                    currentScore = match.score;
            }
            if (currentScore == std::numeric_limits<int>::min())
                break;
            previousScore = currentScore;
            MemberEvalResult result;
            const MatchedCandidate *resultMatch = nullptr;
            bool tied = false;
            for (const MatchedCandidate &match : matches) {
                if (match.score != currentScore)
                    continue;
                ClassInstance resultInstance = resolveTypeInstance(
                            invocableReturnType(match.statement), match.bindings,
                            invocableReturnContext(match.statement), evaluation);
                if (resultInstance.type.qualifiedName.isEmpty())
                    continue;
                if (result.statement) {
                    if (equivalentDeclaration(*resultMatch, match)) {
                        if (!statementHasDefinition(result.statement)
                                && statementHasDefinition(match.statement)) {
                            result = {match.statement, resultInstance.type, resultInstance,
                                      {match.statement, match.bindings}};
                            resultMatch = &match;
                        }
                        continue;
                    }
                    tied = true;
                } else {
                    result = {match.statement, resultInstance.type, resultInstance,
                              {match.statement, match.bindings}};
                    resultMatch = &match;
                }
            }
            if (tied)
                return {};
            if (result.statement)
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
    ClassInstance result = resolveTypeInstance(std::dynamic_pointer_cast<Statement>(member.statement)->typeInfo, member.owner.bindings,
                                               member.statement->parentScope.lock(), evaluation);
    return {member.statement, result.type, result};
}

EvalResult CppTypeResolver::evaluateInvocation(
        const QVector<TemplateArgument> &arguments, int functionIndex, const PStatement &context,
        const PTemplateBindingFrame &bindings, EvaluationContext &evaluation) const
{
    if (functionIndex < 0 || functionIndex >= arguments.count())
        return {};
    for (const TemplateArgument &argument : arguments) {
        if (argument.kind != TemplateArgumentKind::Type || !argument.type)
            return {};
    }
    ClassInstance callable = resolveTypeInstance(
                *arguments[functionIndex].type, bindings, context, evaluation);
    if (callable.declaration
            && (callable.declaration->kind == StatementKind::Class
                || callable.declaration->kind == StatementKind::Lambda)) {
        QVector<TypeInfo> callArguments;
        for (int i = functionIndex + 1; i < arguments.count(); ++i) {
            TypeInfo argument = resolveType(*arguments[i].type, bindings, context, evaluation);
            if (argument.qualifiedName.isEmpty())
                return {};
            callArguments.append(argument);
        }
        MemberEvalResult member = invokeMember(callable, "operator()", callArguments, &evaluation);
        EvalResult result;
        result.type = member.type;
        result.instance = member.instance;
        result.kind = EvalStatementKind::Variable;
        result.baseStatement = member.statement;
        result.statement = member.statement;
        result.invocation = member.invocation;
        result.type.valueCategory = !result.type.derivations.isEmpty()
                && result.type.derivations.first().kind == TypeDerivationKind::LvalueReference
                ? ReferenceQualifier::Lvalue : ReferenceQualifier::Rvalue;
        return result;
    }
    QString expression = "std::declval<"
            + arguments[functionIndex].toString() + ">()(";
    QStringList callArguments;
    for (int i = functionIndex + 1; i < arguments.count(); ++i)
        callArguments.append("std::declval<" + arguments[i].toString() + ">()");
    expression += callArguments.join(",") + ")";
    CppTokenizer tokenizer;
    tokenizer.tokenize({expression});
    TypeOperationRegistry operations(*this);
    CppExpressionEvaluator evaluator(const_cast<StatementModel &>(mModel), *this, operations,
                                     bindings);
    return evaluator.evaluate(tokenizer, context, &evaluation);
}

ConstraintSatisfaction CppTypeResolver::evaluateLogicalTrait(
        const QVector<TemplateArgument> &arguments, bool conjunction, const PStatement &context,
        const PTemplateBindingFrame &bindings, EvaluationContext &evaluation) const
{
    auto evaluateOne = [&](const TemplateArgument &argument) {
        QString text = argument.toString();
        if (argument.kind == TemplateArgumentKind::Type && !text.endsWith("::value"))
            text += "::value";
        QString value = evaluateConstantValue(text, bindings, context, evaluation);
        if (value == "true")
            return ConstraintSatisfaction::Satisfied;
        if (value == "false")
            return ConstraintSatisfaction::Unsatisfied;
        return ConstraintSatisfaction::Unknown;
    };
    bool unknown = false;
    auto combine = [&](ConstraintSatisfaction result) {
        if (result == ConstraintSatisfaction::Unknown) {
            unknown = true;
            return false;
        }
        return conjunction ? result == ConstraintSatisfaction::Unsatisfied
                           : result == ConstraintSatisfaction::Satisfied;
    };
    for (const TemplateArgument &argument : arguments) {
        QString pattern = argument.constantExpression.trimmed();
        if (argument.kind != TemplateArgumentKind::NonType || !pattern.endsWith("...")) {
            if (combine(evaluateOne(argument)))
                return conjunction ? ConstraintSatisfaction::Unsatisfied
                                   : ConstraintSatisfaction::Satisfied;
            continue;
        }
        pattern.chop(3);
        CppTokenizer tokenizer;
        tokenizer.tokenize({pattern});
        QSet<QString> identifiers;
        for (const auto &token : tokenizer.tokens()) {
            if (token.kind == CppTokenizer::TokenKind::Identifier)
                identifiers.insert(token.text);
        }
        bool foundPack = false;
        for (const QString &identifier : identifiers) {
            const QVector<TemplateArgument> *pack = findParameterPack(bindings, identifier);
            if (!pack)
                continue;
            foundPack = true;
            for (const TemplateArgument &packArgument : *pack) {
                QString expanded = substituteIdentifier(pattern, identifier, packArgument.toString());
                ConstraintSatisfaction expandedResult = evaluateOne(
                            TemplateArgument::fromConstant(expanded));
                if (combine(expandedResult))
                    return conjunction ? ConstraintSatisfaction::Unsatisfied
                                       : ConstraintSatisfaction::Satisfied;
            }
            break;
        }
        if (!foundPack)
            unknown = true;
    }
    if (unknown)
        return ConstraintSatisfaction::Unknown;
    return conjunction ? ConstraintSatisfaction::Satisfied : ConstraintSatisfaction::Unsatisfied;
}

ConstraintSatisfaction CppTypeResolver::evaluateInvocableR(
        const QVector<TemplateArgument> &arguments, const PStatement &context,
        const PTemplateBindingFrame &bindings, EvaluationContext &evaluation) const
{
    if (arguments.count() < 2 || arguments.first().kind != TemplateArgumentKind::Type
            || !arguments.first().type) {
        return ConstraintSatisfaction::Unknown;
    }
    EvalResult result = evaluateInvocation(arguments, 1, context, bindings, evaluation);
    if (!result.isValid())
        return ConstraintSatisfaction::Unknown;
    TypeInfo target = resolveType(*arguments.first().type, bindings, context, evaluation);
    if (target.qualifiedName.isEmpty())
        return ConstraintSatisfaction::Unknown;
    if (target.qualifiedName == "void" && target.derivations.isEmpty())
        return ConstraintSatisfaction::Satisfied;
    if (isBuiltinType(target) && !isBuiltinType(result.type))
        return ConstraintSatisfaction::Unsatisfied;
    TypeInfo resolvedTarget;
    return compatibleArgument(target, result.type, bindings, context, evaluation, resolvedTarget)
            ? ConstraintSatisfaction::Satisfied : ConstraintSatisfaction::Unsatisfied;
}

ConstraintSatisfaction CppTypeResolver::evaluateConstructibleExpression(
        const QString &expression, const PStatement &context, const PTemplateBindingFrame &bindings,
        EvaluationContext &evaluation) const
{
    const QString prefix = "__is_constructible(";
    if (!expression.startsWith(prefix) || !expression.endsWith(')'))
        return ConstraintSatisfaction::Unknown;
    QString argumentsText = expression.mid(prefix.size(), expression.size() - prefix.size() - 1);
    CppTokenizer tokenizer;
    tokenizer.tokenize({argumentsText});
    QStringList parts;
    int partStart = 0;
    int angleLevel = 0;
    const auto &tokens = tokenizer.tokens();
    for (int i = 0; i < tokens.count(); ++i) {
        if (tokens[i].matchIndex > i) {
            i = tokens[i].matchIndex;
            continue;
        }
        if (tokens[i].text == "<") {
            ++angleLevel;
            continue;
        }
        if (tokens[i].text == ">" || tokens[i].text == ">>") {
            angleLevel = qMax(0, angleLevel - tokens[i].text.size());
            continue;
        }
        if (tokens[i].text != "," || angleLevel != 0)
            continue;
        parts.append(argumentsText.mid(partStart, tokens[i].position.ch - partStart));
        partStart = tokens[i].position.ch + 1;
    }
    parts.append(argumentsText.mid(partStart));
    if (parts.isEmpty())
        return ConstraintSatisfaction::Unknown;
    TypeInfo target = resolveType(parseTypeText(parts.first().trimmed(), evaluation),
                                  bindings, context, evaluation);
    if (target.qualifiedName.isEmpty())
        return ConstraintSatisfaction::Unknown;
    QVector<TypeInfo> arguments;
    for (int i = 1; i < parts.count(); ++i) {
        TypeInfo argument = resolveType(parseTypeText(parts[i].trimmed(), evaluation),
                                        bindings, context, evaluation);
        if (argument.qualifiedName.isEmpty())
            return ConstraintSatisfaction::Unknown;
        arguments.append(argument);
    }
    return evaluateConstructible(target, arguments, context, bindings, evaluation);
}

ConstraintSatisfaction CppTypeResolver::evaluateConstructible(
        const TypeInfo &target, const QVector<TypeInfo> &arguments, const PStatement &context,
        const PTemplateBindingFrame &bindings, EvaluationContext &evaluation) const
{
    for (const TypeInfo &argument : arguments) {
        if (argument.qualifiedName.isEmpty()
                || (!isBuiltinType(argument) && !argument.declaration.lock()
                    && !findDeclaration(argument.qualifiedName, context, true))) {
            return ConstraintSatisfaction::Unknown;
        }
    }
    if (isBuiltinType(target)) {
        if (target.qualifiedName == "void")
            return ConstraintSatisfaction::Unsatisfied;
        if (arguments.isEmpty())
            return ConstraintSatisfaction::Satisfied;
        if (arguments.count() != 1)
            return ConstraintSatisfaction::Unknown;
        TypeInfo resolvedTarget;
        return compatibleArgument(target, arguments.first(), bindings, context, evaluation,
                                  resolvedTarget)
                ? ConstraintSatisfaction::Satisfied : ConstraintSatisfaction::Unsatisfied;
    }
    PStatement declaration = target.declaration.lock();
    if (!declaration)
        declaration = findDeclaration(target.qualifiedName, context, true);
    if (!declaration || declaration->kind != StatementKind::Class)
        return ConstraintSatisfaction::Unknown;

    QVector<PStatement> constructors;
    for (const std::weak_ptr<AbstractStatement> &weak
         : std::dynamic_pointer_cast<Statement>(declaration)->children) {
        PStatement candidate = weak.lock();
        if (candidate && candidate->kind == StatementKind::Constructor)
            constructors.append(candidate);
    }
    if (constructors.isEmpty()) {
        if (arguments.isEmpty())
            return ConstraintSatisfaction::Satisfied;
        if (arguments.count() == 1) {
            TypeInfo resolvedTarget;
            return compatibleArgument(target, arguments.first(), bindings, context, evaluation,
                                      resolvedTarget)
                    ? ConstraintSatisfaction::Satisfied : ConstraintSatisfaction::Unsatisfied;
        }
        return ConstraintSatisfaction::Unsatisfied;
    }
    for (const PStatement &constructor : constructors) {
        EvaluationContext candidateEvaluation = evaluation;
        candidateEvaluation.operationCount = 0;
        candidateEvaluation.resolutionFailed = false;
        candidateEvaluation.aliases.clear();
        candidateEvaluation.resolvingTypes.clear();
        candidateEvaluation.resolvingConstraints.clear();
        if (matchFunctionCandidate(constructor, arguments, {}, candidateEvaluation, {}, nullptr,
                                   false))
            return ConstraintSatisfaction::Satisfied;
    }
    return ConstraintSatisfaction::Unsatisfied;
}

ConstraintSatisfaction CppTypeResolver::evaluateNoexceptExpression(
        const QString &expression, const PStatement &context, const PTemplateBindingFrame &bindings,
        EvaluationContext *evaluationContext) const
{
    QString text = expression.trimmed();
    if (!text.startsWith("noexcept(") || !text.endsWith(')'))
        return ConstraintSatisfaction::Unknown;
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    CppTokenizer tokenizer;
    tokenizer.tokenize({text.mid(9, text.size() - 10)});
    TypeOperationRegistry operations(*this);
    CppExpressionEvaluator evaluator(const_cast<StatementModel &>(mModel), *this, operations,
                                     bindings);
    EvalResult evaluated = evaluator.evaluate(tokenizer, context, &evaluation);
    return evaluateNoexceptCall(evaluated, evaluation);
}

ConstraintSatisfaction CppTypeResolver::evaluateNoexceptCall(
        const EvalResult &evaluated, EvaluationContext &evaluation) const
{
    if (!evaluated.isValid())
        return ConstraintSatisfaction::Unknown;
    if (evaluated.invocation.statement) {
        const auto function = std::dynamic_pointer_cast<Statement>(evaluated.invocation.statement);
        switch (function->noexceptKind) {
        case NoexceptKind::None:
            return ConstraintSatisfaction::Unsatisfied;
        case NoexceptKind::Always:
            return ConstraintSatisfaction::Satisfied;
        case NoexceptKind::Expression: {
            QString value = evaluateConstantValue(function->noexceptExpression,
                    evaluated.invocation.bindings, function->parentScope.lock(), evaluation);
            if (value == "true")
                return ConstraintSatisfaction::Satisfied;
            if (value == "false")
                return ConstraintSatisfaction::Unsatisfied;
            return ConstraintSatisfaction::Unknown;
        }
        }
    }
    return evaluated.kind == EvalStatementKind::Literal
            ? ConstraintSatisfaction::Satisfied : ConstraintSatisfaction::Unknown;
}

TypeInfo CppTypeResolver::invokeFunction(
        const PStatement &function, const QVector<TypeInfo> &arguments,
        const QVector<TemplateArgument> &explicitArguments,
        EvaluationContext *evaluationContext, ClassInstance *resultInstance,
        const PTemplateBindingFrame &outerBindings, CallResolution *invocation) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    if (invocation)
        *invocation = {};
    struct MatchedCandidate {
        PStatement statement;
        PTemplateBindingFrame bindings;
        EvaluationContext evaluation;
        int score;
    };
    QVector<MatchedCandidate> matches;
    for (const PStatement &candidate : functionCandidates(function)) {
        EvaluationContext candidateEvaluation = evaluation;
        candidateEvaluation.operationCount = 0;
        candidateEvaluation.resolutionFailed = false;
        candidateEvaluation.aliases.clear();
        candidateEvaluation.resolvingTypes.clear();
        candidateEvaluation.resolvingConstraints.clear();
        int score = 0;
        PTemplateBindingFrame currentBindings = matchFunctionCandidate(
                    candidate, arguments, explicitArguments, candidateEvaluation, outerBindings, &score, false);
        if (currentBindings)
            matches.append({candidate, currentBindings, candidateEvaluation, score});
    }
    int previousScore = std::numeric_limits<int>::max();
    while (true) {
        int currentScore = std::numeric_limits<int>::min();
        for (const MatchedCandidate &match : matches) {
            if (match.score < previousScore && match.score > currentScore)
                currentScore = match.score;
        }
        if (currentScore == std::numeric_limits<int>::min())
            break;
        previousScore = currentScore;
        PStatement result;
        ClassInstance instance;
        PTemplateBindingFrame resultBindings;
        bool tied = false;
        for (MatchedCandidate &match : matches) {
            if (match.score != currentScore)
                continue;
            TypeInfo returnType = invocableReturnType(match.statement);
            ClassInstance current;
            if (match.statement->kind == StatementKind::Constructor) {
                current = {returnType, returnType.declaration.lock(), match.bindings};
            } else {
                current = resolveTypeInstance(
                            returnType, match.bindings, invocableReturnContext(match.statement),
                            match.evaluation);
            }
            if (current.type.qualifiedName.isEmpty())
                continue;
            if (result) {
                tied = true;
            } else {
                result = match.statement;
                resultBindings = match.bindings;
                instance = current;
            }
        }
        if (tied)
            return {};
        if (result) {
            if (resultInstance)
                *resultInstance = instance;
            if (invocation)
                *invocation = {result, resultBindings};
            return instance.type;
        }
    }
    if (resultInstance)
        *resultInstance = {};
    return {};
}

static bool functionAcceptsArgumentCount(
        const PStatement &function, int argumentCount, bool allowPartialArguments)
{
    auto normal = std::dynamic_pointer_cast<Statement>(function);
    if (!normal)
        return false;
    const int parameterCount = normal->parameters.count();
    bool parameterPack = false;
    for (const TemplateParameterInfo &parameter : normal->templateParameters)
        parameterPack |= parameter.parameterPack;
    const int parametersEnd = normal->normalizedArgs.lastIndexOf(')');
    const bool variadic = parametersEnd > 0
            && normal->normalizedArgs.left(parametersEnd).trimmed().endsWith("...");
    int requiredCount = parameterCount;
    while (requiredCount > 0
           && !normal->parameters[requiredCount - 1].defaultArgumentText.isEmpty()) {
        --requiredCount;
    }
    if (parameterPack)
        requiredCount = qMin(requiredCount, parameterCount - 1);
    if (allowPartialArguments)
        return argumentCount <= parameterCount || parameterPack || variadic;
    return argumentCount >= requiredCount
            && (argumentCount <= parameterCount || parameterPack || variadic);
}

QList<PStatement> CppTypeResolver::rankFunctionCandidates(
        const QList<PStatement> &candidates, const QVector<TypeInfo> &arguments,
        int minimumArgumentCount, EvaluationContext *evaluationContext) const
{
    EvaluationContext localEvaluation;
    EvaluationContext &evaluation = evaluationContext ? *evaluationContext : localEvaluation;
    struct MatchedCandidate {
        PStatement statement;
        int score;
    };
    QVector<MatchedCandidate> matches;
    for (const PStatement &candidate : candidates) {
        if (!functionAcceptsArgumentCount(candidate, minimumArgumentCount, true))
            continue;
        EvaluationContext candidateEvaluation = evaluation;
        candidateEvaluation.operationCount = 0;
        candidateEvaluation.resolutionFailed = false;
        candidateEvaluation.aliases.clear();
        candidateEvaluation.resolvingTypes.clear();
        candidateEvaluation.resolvingConstraints.clear();
        int score = 0;
        PTemplateBindingFrame bindings = matchFunctionCandidate(
                    candidate, arguments, {}, candidateEvaluation, {}, &score, true);
        if (bindings)
            matches.append({candidate, score});
    }
    std::stable_sort(matches.begin(), matches.end(), [](const auto &left, const auto &right) {
        return left.score > right.score;
    });
    QList<PStatement> result;
    for (const MatchedCandidate &match : matches)
        result.append(match.statement);
    return result;
}

QList<PStatement> CppTypeResolver::functionCandidates(const PStatement &function) const
{
    QList<PStatement> result;
    if (!function)
        return result;
    QSet<const AbstractStatement *> seen;
    auto appendScope = [&](const PStatement &scope) {
        const auto candidates = mModel.childrenStatements(scope).values(function->command);
        for (const std::weak_ptr<AbstractStatement> &weak : candidates) {
            PStatement candidate = weak.lock();
            if (candidate && candidate->kind == StatementKind::InlineAlias) {
                auto alias = std::dynamic_pointer_cast<InlineAliasStatement>(candidate);
                candidate = alias->target.lock();
            }
            if (isInvocable(candidate) && !seen.contains(candidate.get())) {
                seen.insert(candidate.get());
                result.append(candidate);
            }
        }
    };
    PStatement parent = function->parentScope.lock();
    appendScope(parent);
    if (function->kind == StatementKind::Alias) {
        const Statement &alias = *std::dynamic_pointer_cast<Statement>(function);
        if (!alias.value.isEmpty()) {
            PStatement target = findStatement(alias.value, parent);
            if (target)
                appendScope(target->parentScope.lock());
        }
    }
    return result;
}

PTemplateBindingFrame CppTypeResolver::matchFunctionCandidate(
        const PStatement &function, const QVector<TypeInfo> &arguments,
        const QVector<TemplateArgument> &explicitArguments,
        EvaluationContext &evaluation, const PTemplateBindingFrame &outerBindings,
        int *matchScore, bool allowPartialArguments) const
{
    if (!function)
        return {};
    traceEvent(resolverProfile.functionTrace, function->fullName + std::dynamic_pointer_cast<Statement>(function)->normalizedArgs);
    ResolveOriginScope originScope("matchFunctionCandidate");
    if (!functionAcceptsArgumentCount(function, arguments.count(), allowPartialArguments))
        return {};
    auto frame = std::make_shared<TemplateBindingFrame>();
    frame->owner = function.get();
    frame->parent = outerBindings;
    QSet<QString> templateParameters;
    for (const TemplateParameterInfo &parameter : std::dynamic_pointer_cast<Statement>(function)->templateParameters)
        templateParameters.insert(parameter.name);

    int explicitCount = qMin(explicitArguments.count(), std::dynamic_pointer_cast<Statement>(function)->templateParameters.count());
    for (int i = 0; i < explicitCount; ++i) {
        const TemplateParameterInfo &parameter = std::dynamic_pointer_cast<Statement>(function)->templateParameters[i];
        const TemplateArgument &argument = explicitArguments[i];
        if (parameter.kind == TemplateParameterKind::Type) {
            if (argument.kind != TemplateArgumentKind::Type || !argument.type)
                return {};
            TypeInfo actual = resolveType(*argument.type, frame, function->parentScope.lock(),
                                          evaluation);
            if (actual.qualifiedName.isEmpty())
                return {};
            ++frame->revision;
            frame->values.insert(parameter.name, TemplateArgument::fromType(actual));
        } else {
            QString constant = argument.constantExpression;
            if (constant.isEmpty() && argument.type)
                constant = argument.type->qualifiedNameText;
            QString value = evaluateConstantValue(constant, frame, function->parentScope.lock(),
                                                  evaluation);
            ++frame->revision;
            frame->values.insert(parameter.name, TemplateArgument::fromConstant(
                    value.isEmpty() ? constant : value));
        }
    }

    int count = qMin(std::dynamic_pointer_cast<Statement>(function)->parameters.count(), arguments.count());
    for (int i = 0; i < count; ++i) {
        if (std::dynamic_pointer_cast<Statement>(function)->parameters[i].type) {
            bindParameterType(*std::dynamic_pointer_cast<Statement>(function)->parameters[i].type, arguments[i], templateParameters,
                              frame->values, evaluation);
            ++frame->revision;
        }
    }
    for (int i = explicitCount; i < std::dynamic_pointer_cast<Statement>(function)->templateParameters.count(); ++i) {
        const TemplateParameterInfo &parameter = std::dynamic_pointer_cast<Statement>(function)->templateParameters[i];
        if (frame->values.contains(parameter.name))
            continue;
        QString defaultText = parameter.defaultArgumentText;
        if (defaultText.isEmpty() && i < std::dynamic_pointer_cast<Statement>(function)->inheritedDefaultArgumentTexts.count())
            defaultText = std::dynamic_pointer_cast<Statement>(function)->inheritedDefaultArgumentTexts[i];
        if (defaultText.isEmpty())
            continue;
        TemplateArgument argument = resolveArgument(defaultText, parameter.kind,
                                                    frame, function->parentScope.lock(), evaluation);
        ++frame->revision;
        frame->values.insert(parameter.name, argument);
    }
    QVector<ArgumentConversion> conversions;
    conversions.reserve(count);
    for (int i = 0; i < count; ++i) {
        TypeInfo resolvedParameter;
        ArgumentConversion conversion = ArgumentConversion::Exact;
        if (std::dynamic_pointer_cast<Statement>(function)->parameters[i].type) {
            conversion = classifyArgumentConversion(
                    *std::dynamic_pointer_cast<Statement>(function)->parameters[i].type,
                    arguments[i], frame, function->parentScope.lock(), evaluation, resolvedParameter);
            if (conversion == ArgumentConversion::NoMatch)
                return {};
        }
        conversions.append(conversion);
    }
    ConstraintSatisfaction satisfaction = constraintsSatisfied(
            function, frame, function->parentScope.lock(), evaluation);
    if (satisfaction == ConstraintSatisfaction::Unsatisfied)
        return {};
    if (matchScore) {
        int score = 0;
        for (int i = 0; i < count; ++i) {
            const TypeInfo &formal = *std::dynamic_pointer_cast<Statement>(function)->parameters[i].type;
            const TypeInfo &actual = arguments[i];
            if (!templateParameters.contains(formal.qualifiedName))
                ++score;
            switch (conversions[i]) {
            case ArgumentConversion::Exact:
                score += 16;
                break;
            case ArgumentConversion::Promotion:
                score += 12;
                break;
            case ArgumentConversion::Conversion:
                score += 8;
                break;
            case ArgumentConversion::UserDefined:
                score += 2;
                break;
            case ArgumentConversion::NoMatch:
                break;
            }
            TypeDerivationKind referenceKind = TypeDerivationKind::Pointer;
            if (!formal.derivations.isEmpty()
                    && (formal.derivations.first().kind == TypeDerivationKind::LvalueReference
                        || formal.derivations.first().kind == TypeDerivationKind::RvalueReference)) {
                referenceKind = formal.derivations.first().kind;
            }
            if (referenceKind == TypeDerivationKind::RvalueReference
                    && actual.valueCategory == ReferenceQualifier::Rvalue) {
                score += 4;
            } else if (referenceKind == TypeDerivationKind::LvalueReference
                       && actual.valueCategory == ReferenceQualifier::Lvalue) {
                score += 4;
            } else if (referenceKind == TypeDerivationKind::Pointer) {
                score += 2;
            } else if (referenceKind == TypeDerivationKind::LvalueReference
                       && actual.valueCategory == ReferenceQualifier::Rvalue) {
                score += 2;
            } else {
                score -= 4;
            }
        }
        *matchScore = score;
    }
    return frame;
}

bool CppTypeResolver::compatibleArgument(
        const TypeInfo &parameter, const TypeInfo &argument,
        const PTemplateBindingFrame &bindings, const PStatement &context,
        EvaluationContext &evaluation, TypeInfo &resolvedParameter) const
{
    return classifyArgumentConversion(parameter, argument, bindings, context, evaluation,
                                      resolvedParameter) != ArgumentConversion::NoMatch;
}

CppTypeResolver::ArgumentConversion CppTypeResolver::classifyArgumentConversion(
        const TypeInfo &parameter, const TypeInfo &argument,
        const PTemplateBindingFrame &bindings, const PStatement &context,
        EvaluationContext &evaluation, TypeInfo &resolvedParameter) const
{
    ResolveOriginScope originScope("compatibleArgument");
    ClassInstance formalInstance = resolveTypeInstance(parameter, bindings, context, evaluation);
    const TypeInfo &formal = formalInstance.type;
    resolvedParameter = formal;
    auto withoutReferences = [](TypeInfo type) {
        while (!type.derivations.isEmpty()
               && (type.derivations.first().kind == TypeDerivationKind::LvalueReference
                   || type.derivations.first().kind == TypeDerivationKind::RvalueReference)) {
            type.derivations.removeFirst();
        }
        return type;
    };
    auto pointerLike = [](const TypeInfo &type) {
        for (const TypeDerivation &derivation : type.derivations) {
            if (derivation.kind == TypeDerivationKind::Pointer
                    || derivation.kind == TypeDerivationKind::MemberPointer) {
                return true;
            }
        }
        return false;
    };
    if (formal.toString() == argument.toString())
        return ArgumentConversion::Exact;
    TypeInfo formalValue = withoutReferences(formal);
    TypeInfo argumentValue = withoutReferences(argument);
    if (formalValue.qualifiedName.isEmpty() || argumentValue.qualifiedName.isEmpty())
        return ArgumentConversion::Conversion;

    TypeInfo formalArithmetic = canonicalArithmeticType(formalValue);
    TypeInfo argumentArithmetic = canonicalArithmeticType(argumentValue);
    if (!formalArithmetic.qualifiedName.isEmpty()
            && !argumentArithmetic.qualifiedName.isEmpty()) {
        if (formalArithmetic.qualifiedName == argumentArithmetic.qualifiedName)
            return ArgumentConversion::Exact;
        TypeInfo promoted = promotedArithmeticType(argumentValue);
        if (promoted.qualifiedName == formalArithmetic.qualifiedName)
            return ArgumentConversion::Promotion;
        return ArgumentConversion::Conversion;
    }

    PStatement argumentDeclaration = argumentValue.declaration.lock();
    if (!argumentDeclaration)
        argumentDeclaration = findDeclaration(argumentValue.qualifiedName, context, true);
    const bool formalPointer = pointerLike(formalValue);
    const bool argumentPointer = pointerLike(argumentValue);
    if (!formalPointer && !argumentPointer && formalInstance.declaration
            && formalInstance.declaration->kind == StatementKind::Class) {
        if (argumentDeclaration == formalInstance.declaration
                && formalValue.toString() == argumentValue.toString()) {
            return ArgumentConversion::Exact;
        }
        if (argumentDeclaration && argumentDeclaration->kind == StatementKind::Class) {
            QSet<const AbstractStatement *> visited;
            QStack<PStatement> pending;
            pending.push(argumentDeclaration);
            while (!pending.isEmpty()) {
                PStatement current = pending.pop();
                if (!current || visited.contains(current.get()))
                    continue;
                if (current == formalInstance.declaration)
                    return ArgumentConversion::Conversion;
                visited.insert(current.get());
                for (const QString &baseSpecifier : std::dynamic_pointer_cast<Statement>(current)->baseClasses) {
                    StatementAccessibility inheritance;
                    if (PStatement base = baseClass(current, baseSpecifier, inheritance))
                        pending.push(base);
                }
            }
        }
    }
    if (formalPointer && argumentPointer)
        return formalValue.toString() == argumentValue.toString()
                ? ArgumentConversion::Exact : ArgumentConversion::Conversion;
    if (formalPointer && argumentValue.qualifiedName == "std::nullptr_t")
        return ArgumentConversion::Conversion;
    if (formalValue.qualifiedName == argumentValue.qualifiedName
            && formalValue.derivations.count() == argumentValue.derivations.count()) {
        return ArgumentConversion::Exact;
    }
    if (!formalInstance.declaration && !argumentDeclaration
            && formalValue.derivations.isEmpty() && argumentValue.derivations.isEmpty()) {
        return ArgumentConversion::Conversion;
    }
    if (!evaluation.allowUserDefinedConversions)
        return ArgumentConversion::NoMatch;

    if (!formalPointer && formalInstance.declaration
            && formalInstance.declaration->kind == StatementKind::Class) {
        for (const std::weak_ptr<AbstractStatement> &weak
             : std::dynamic_pointer_cast<Statement>(formalInstance.declaration)->children.values(
                 formalInstance.declaration->command)) {
            PStatement constructor = weak.lock();
            if (!constructor || constructor->kind != StatementKind::Constructor
                    || std::dynamic_pointer_cast<Statement>(constructor)->declSpecifierProperties.testFlag(
                        DeclSpecifierProperty::Explicit)) {
                continue;
            }
            EvaluationContext candidateEvaluation = evaluation;
            candidateEvaluation.allowUserDefinedConversions = false;
            if (matchFunctionCandidate(constructor, {argument}, {}, candidateEvaluation,
                                       formalInstance.bindings, nullptr, false)) {
                return ArgumentConversion::UserDefined;
            }
        }
    }

    if (argumentDeclaration && argumentDeclaration->kind == StatementKind::Class) {
        for (const PStatement &member : membersOf(argumentDeclaration)) {
            auto conversion = std::dynamic_pointer_cast<Statement>(member);
            if (!conversion || member->kind != StatementKind::OverloadedOperator
                    || !member->command.startsWith("operator ")
                    || conversion->declSpecifierProperties.testFlag(DeclSpecifierProperty::Explicit)) {
                continue;
            }
            EvaluationContext candidateEvaluation = evaluation;
            candidateEvaluation.allowUserDefinedConversions = false;
            if (!matchFunctionCandidate(member, {}, {}, candidateEvaluation, {}, nullptr, false))
                continue;
            TypeInfo converted;
            if (classifyArgumentConversion(formal, conversion->typeInfo, bindings, context,
                                           candidateEvaluation, converted)
                    != ArgumentConversion::NoMatch) {
                return ArgumentConversion::UserDefined;
            }
        }
    }
    return ArgumentConversion::NoMatch;
}

QList<PStatement> CppTypeResolver::membersOf(const PStatement &scope) const
{
    QList<PStatement> result;
    if (!scope) {
        for (const std::weak_ptr<AbstractStatement> &weak : mModel.childrenStatements()) {
            if (PStatement statement = weak.lock())
                result.append(statement);
        }
        return result;
    }
    if (scope->kind == StatementKind::InlineAlias) {
        PStatement target = std::dynamic_pointer_cast<InlineAliasStatement>(scope)->target.lock();
        return target ? membersOf(target) : result;
    }
    QSet<const AbstractStatement *> visited;
    MemberEntryList entries;
    appendMembers(scope, false, visited, entries);
    for (const MemberEntry &entry : entries)
        result.append(entry.statement);
    return result;
}

QList<PStatement> CppTypeResolver::resolveMembersOfClass(
        const PStatement &classStatement, const PStatement &accessScope) const
{
    QList<PStatement> result;
    if (!classStatement || classStatement->kind != StatementKind::Class)
        return result;

    auto enclosingClass = [](PStatement scope) {
        while (scope && scope->kind != StatementKind::Class)
            scope = scope->parentScope.lock();
        return scope;
    };
    PStatement accessClass = enclosingClass(accessScope);
    QSet<const AbstractStatement *> visited;
    MemberEntryList entries;
    appendMembers(classStatement, false, visited, entries);
    for (const MemberEntry &entry : entries) {
        PStatement declaringClass = enclosingClass(entry.statement->parentScope.lock());
        bool accessible = entry.accessibility == StatementAccessibility::Public;
        if (accessClass && accessClass == declaringClass) {
            accessible = true;
        } else if (accessClass == classStatement) {
            accessible = entry.statement->accessibility != StatementAccessibility::Private;
        } else if (accessClass && entry.accessibility == StatementAccessibility::Protected) {
            QSet<const AbstractStatement *> basesVisited;
            QStack<PStatement> pending;
            pending.push(accessClass);
            while (!pending.isEmpty() && !accessible) {
                PStatement current = pending.pop();
                if (!current || basesVisited.contains(current.get()))
                    continue;
                basesVisited.insert(current.get());
                for (const QString &baseSpecifier : std::dynamic_pointer_cast<Statement>(current)->baseClasses) {
                    StatementAccessibility inheritance;
                    PStatement base = baseClass(current, baseSpecifier, inheritance);
                    if (base == classStatement) {
                        accessible = true;
                        break;
                    }
                    if (base)
                        pending.push(base);
                }
            }
        }
        if (accessible)
            result.append(entry.statement);
    }
    return result;
}

PStatement CppTypeResolver::baseClass(const PStatement &scope, const QString &baseSpecifier,
                                      StatementAccessibility &inheritance) const
{
    QString baseName = baseSpecifier.trimmed();
    inheritance = std::dynamic_pointer_cast<Statement>(scope)->classKey == ClassKey::Struct
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
                                    QSet<const AbstractStatement *> &visited,
                                    MemberEntryList &result) const
{
    if (!scope || visited.contains(scope.get()))
        return;
    visited.insert(scope.get());
    auto signature = [](const PStatement &statement) {
        QString result = statement->command;
        if (statement->kind != StatementKind::Namespace)
            result += std::dynamic_pointer_cast<Statement>(statement)->normalizedArgs;
        return result;
    };
    QSet<QString> signatures;
    for (const MemberEntry &entry : result)
        signatures.insert(signature(entry.statement));

    for (const std::weak_ptr<AbstractStatement> &weak : mModel.childrenStatements(scope)) {
        PStatement statement = weak.lock();
        if (statement && statement->kind == StatementKind::InlineAlias)
            statement = std::dynamic_pointer_cast<InlineAliasStatement>(statement)->target.lock();
        if (!statement || (inherited && (statement->kind == StatementKind::Constructor
                                        || statement->kind == StatementKind::Destructor))) {
            continue;
        }
        if (signatures.contains(signature(statement)))
            continue;
        signatures.insert(signature(statement));
        result.append({statement, statement->accessibility});
    }

    if (scope->kind == StatementKind::Namespace)
        return;
    for (const QString &baseSpecifier : std::dynamic_pointer_cast<Statement>(scope)->baseClasses) {
        StatementAccessibility inheritance;
        PStatement base = baseClass(scope, baseSpecifier, inheritance);
        if (!base)
            continue;
        MemberEntryList baseMembers;
        appendMembers(base, true, visited, baseMembers);
        for (MemberEntry entry : baseMembers) {
            if (signatures.contains(signature(entry.statement)))
                continue;
            entry.accessibility = applyInheritance(entry.accessibility, inheritance);
            signatures.insert(signature(entry.statement));
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

    if (!formal.derivations.isEmpty() && !actual.derivations.isEmpty()
            && formal.derivations.first().kind == actual.derivations.first().kind
            && (formal.derivations.first().kind == TypeDerivationKind::Pointer
                || formal.derivations.first().kind == TypeDerivationKind::Array)) {
        formal.derivations.removeFirst();
        actual.derivations.removeFirst();
        bindParameterType(formal, actual, templateParameters, bindings, evaluation);
        return;
    }
    if (templateParameters.contains(formal.qualifiedName)) {
        if (!bindings.contains(formal.qualifiedName))
            bindings.insert(formal.qualifiedName, TemplateArgument::fromType(actual));
        return;
    }
    if (formal.qualifiedName != actual.qualifiedName) {
        PStatement formalDeclaration = formal.declaration.lock();
        PStatement actualDeclaration = actual.declaration.lock();
        if (!formalDeclaration && actualDeclaration) {
            formalDeclaration = findDeclaration(formal.qualifiedName,
                                                actualDeclaration->parentScope.lock(), true);
        }
        if (!formalDeclaration || formalDeclaration != actualDeclaration)
            return;
    }
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
    QString typeString = profiledTypeString(type);
    bool cacheEnabled = !evaluation.allowModelChanges
            && evaluation.caches.testFlag(CacheLevel::ResolvedType);
    ResolvedTypeKey cacheKey;
    if (cacheEnabled) {
        cacheKey = {typeString, type.declaration.lock().get(), context.get(), bindings};
        auto cached = evaluation.resolvedTypes->constFind(cacheKey);
        traceSyntaxAttempt("resolveType", typeString, cached != evaluation.resolvedTypes->constEnd());
        if (cached != evaluation.resolvedTypes->constEnd())
            return cached.value();
    }
    QString resolutionKey = (context ? context->fullName : QString()) + "|" + typeString
            + "|" + (bindings && bindings->owner ? bindings->owner->fullName : QString());
    if (evaluation.resolvingTypes.contains(resolutionKey)) {
        evaluation.resolutionFailed = true;
        return {};
    }
    ResolutionGuard guard(evaluation.resolvingTypes, resolutionKey);
    auto cacheResult = [&](ClassInstance result) {
        if (cacheEnabled && !result.type.qualifiedName.isEmpty()
                && evaluation.operationCount <= 4096 && !evaluation.resolutionFailed) {
            evaluation.resolvedTypes->insert(cacheKey, result);
        }
        return result;
    };

    const QString decltypePrefix = "decltype(";
    if (text.startsWith(decltypePrefix) && text.endsWith(")")) {
        CppTokenizer tokenizer;
        tokenizer.tokenize({text.mid(decltypePrefix.size(),
                                      text.size() - decltypePrefix.size() - 1)});
        TypeOperationRegistry operations(*this);
        CppExpressionEvaluator evaluator(const_cast<StatementModel &>(mModel), *this, operations,
                                         bindings);
        EvalResult evaluated = evaluator.evaluate(tokenizer, context, &evaluation);
        if (!evaluated.isValid())
            return {};
        evaluated.instance.type = evaluated.type;
        evaluated.instance.type.qualifiers |= type.qualifiers;
        evaluated.instance.type.derivations += type.derivations;
        if (!evaluated.instance.declaration)
            evaluated.instance.declaration = evaluated.type.declaration.lock();
        return cacheResult(evaluated.instance);
    }

    for (const QString &name : typeBuiltinsForCompiler(mOptions.compilerType)) {
        QString prefix = name + "(";
        if (!text.startsWith(prefix) || !text.endsWith(")"))
            continue;
        QString argumentText = text.mid(prefix.size(), text.size() - prefix.size() - 1);
        TypeInfo argument = parseTypeText(argumentText, evaluation);
        if (argument.qualifiedName.isEmpty())
            return {};
        TypeInfo result = applyTypeBuiltin(
                    name, resolveType(argument, bindings, context, evaluation));
        if (result.qualifiedName.isEmpty())
            return {};
        result.qualifiers |= type.qualifiers;
        result.derivations += type.derivations;
        ClassInstance instance = classInstanceFromResolvedType(result, bindings, context, evaluation);
        if (instance.type.qualifiedName.isEmpty()) {
            instance.type = result;
            instance.declaration = result.declaration.lock();
        }
        return cacheResult(instance);
    }

    QVector<TypeNameSegment> segments;
    bool globallyQualified = false;
    PStatement referenceDeclaration = type.declaration.lock();
    if (referenceDeclaration && referenceDeclaration->fullName == type.qualifiedName
            && (!type.templateArguments.isEmpty() || !type.qualifiedNameText.contains('<'))) {
        TypeNameSegment segment;
        segment.name = type.qualifiedName;
        segment.arguments = type.templateArguments;
        segment.hasTemplateArguments = !type.templateArguments.isEmpty();
        segments.append(segment);
        globallyQualified = type.qualifiedName.startsWith("::");
    } else if (!parseTypeNameSegments(text, segments, globallyQualified, evaluation)
               || segments.isEmpty()) {
        return {};
    }
    TypeInfo reference = type;
    if (segments.count() == 1 && segments.first().arguments.isEmpty()) {
        QString name = segments.first().name;
        PTemplateBindingFrame bindingFrame = bindings;
        const TemplateArgument *binding = nullptr;
        ClassInstance result;
        bool hasResolvedValue = false;
        bool resolvedValueCacheEnabled = evaluation.caches.testFlag(CacheLevel::ResolvedValue);
        for (; bindingFrame; bindingFrame = bindingFrame->parent) {
            auto argument = bindingFrame->values.constFind(name);
            if (argument == bindingFrame->values.constEnd())
                continue;
            binding = &argument.value();
            auto resolved = resolvedValueCacheEnabled
                    ? bindingFrame->resolvedValues.constFind(name)
                    : bindingFrame->resolvedValues.constEnd();
            if (resolved != bindingFrame->resolvedValues.constEnd()) {
                result = resolved.value();
                hasResolvedValue = true;
            }
            break;
        }
        if (binding) {
            if (binding->kind != TemplateArgumentKind::Type || !binding->type)
                return {};
            if (binding->type->toString() == typeString)
                return {};
            if (!hasResolvedValue) {
                PStatement bindingDeclaration = binding->type->declaration.lock();
                if (bindingDeclaration && (bindingDeclaration->kind == StatementKind::Class
                                           || bindingDeclaration->kind == StatementKind::Lambda)) {
                    result = classInstanceFromResolvedType(*binding->type, bindings,
                                                           context, evaluation);
                } else {
                    result = resolveTypeInstance(*binding->type, bindings, context, evaluation);
                }
                if (resolvedValueCacheEnabled && !result.type.qualifiedName.isEmpty())
                    bindingFrame->resolvedValues.insert(name, result);
            }
            applyReference(result.type, reference);
            return cacheResult(result);
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
    return cacheResult(resolveSegments(type, segments, globallyQualified, bindings, context, evaluation));
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
            PStatement bindingDeclaration = binding->type->declaration.lock();
            ClassInstance owner;
            if (bindingDeclaration && (bindingDeclaration->kind == StatementKind::Class
                                       || bindingDeclaration->kind == StatementKind::Lambda)) {
                owner = classInstanceFromResolvedType(*binding->type, bindings, context, evaluation);
            } else {
                owner = resolveTypeInstance(*binding->type, bindings, context, evaluation);
            }
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
        QVector<TemplateArgument> expandedArguments;
        for (const TemplateArgument &argument : instance.templateArguments) {
            const QVector<TemplateArgument> *pack = nullptr;
            if (argument.kind == TemplateArgumentKind::Type && argument.type)
                pack = findParameterPack(bindings, argument.type->qualifiedName);
            if (pack) {
                expandedArguments += *pack;
                continue;
            }
            if (argument.kind == TemplateArgumentKind::NonType) {
                QString pattern = argument.constantExpression.trimmed();
                if (pattern.endsWith("...")) {
                    pattern.chop(3);
                    CppTokenizer patternTokenizer;
                    patternTokenizer.tokenize({pattern});
                    QSet<QString> identifiers;
                    for (const auto &token : patternTokenizer.tokens()) {
                        if (token.kind == CppTokenizer::TokenKind::Identifier)
                            identifiers.insert(token.text);
                    }
                    bool foundPack = false;
                    for (const QString &identifier : identifiers) {
                        const QVector<TemplateArgument> *constantPack = findParameterPack(
                                    bindings, identifier);
                        if (!constantPack)
                            continue;
                        foundPack = true;
                        for (const TemplateArgument &packArgument : *constantPack) {
                            QString expanded = substituteIdentifier(
                                        pattern, identifier, packArgument.toString());
                            QString value = evaluateConstantValue(
                                        expanded, bindings, context, evaluation);
                            expandedArguments.append(TemplateArgument::fromConstant(
                                    value.isEmpty() ? expanded : value));
                        }
                        break;
                    }
                    if (foundPack)
                        continue;
                }
            }
            expandedArguments.append(argument);
        }
        instance.templateArguments = expandedArguments;
        for (int argumentIndex = 0; argumentIndex < instance.templateArguments.count();
             ++argumentIndex) {
            TemplateArgument &argument = instance.templateArguments[argumentIndex];
            if (argumentIndex < std::dynamic_pointer_cast<Statement>(declaration)->templateParameters.count()
                    && std::dynamic_pointer_cast<Statement>(declaration)->templateParameters[argumentIndex].kind
                    == TemplateParameterKind::NonType) {
                if (argument.constantExpression.isEmpty() && argument.type) {
                    argument.constantExpression = argument.type->qualifiedNameText.isEmpty()
                            ? argument.type->qualifiedName : argument.type->qualifiedNameText;
                }
                argument.kind = TemplateArgumentKind::NonType;
                argument.type.reset();
            }
            if (argument.kind == TemplateArgumentKind::NonType) {
                QString value = evaluateConstantValue(
                        argument.constantExpression, bindings, context, evaluation);
                if (!value.isEmpty())
                    argument.constantExpression = value;
                continue;
            }
            if (argumentIndex < std::dynamic_pointer_cast<Statement>(declaration)->templateParameters.count()
                    && std::dynamic_pointer_cast<Statement>(declaration)->templateParameters[argumentIndex].kind
                    == TemplateParameterKind::TemplateTemplate) {
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
            TypeInfo input = *argument.type;
            argument.type = std::make_shared<TypeInfo>(resolveType(
                    input, bindings, context, evaluation));
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
            if (!localBindings->complete) {
                evaluation.aliases.remove(aliasKey);
                return {};
            }
            ConstraintSatisfaction satisfaction = constraintsSatisfied(
                    declaration, localBindings, declaration->parentScope.lock(), evaluation);
            if (satisfaction == ConstraintSatisfaction::Unsatisfied) {
                evaluation.aliases.remove(aliasKey);
                return {};
            }
            TypeInfo targetType = std::dynamic_pointer_cast<Statement>(declaration)->typeInfo;
            PStatement targetDeclaration = targetType.declaration.lock();
            if (!instance.templateArguments.isEmpty() && targetType.templateArguments.isEmpty()
                    && targetDeclaration && targetDeclaration->kind == StatementKind::Class
                    && !std::dynamic_pointer_cast<Statement>(targetDeclaration)->templateParameters.isEmpty()) {
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
            if (!segment.hasTemplateArguments && !std::dynamic_pointer_cast<Statement>(declaration)->templateParameters.isEmpty()) {
                PStatement scope = context;
                while (scope && scope != declaration)
                    scope = scope->parentScope.lock();
                if (scope == declaration) {
                    PTemplateBindingFrame current = environment;
                    while (current && current->owner != declaration.get())
                        current = current->parent;
                    if (current) {
                        QVector<TemplateArgument> arguments;
                        const TypeInfo &pattern = std::dynamic_pointer_cast<Statement>(declaration)->templatePattern;
                        if (!pattern.templateArguments.isEmpty()) {
                            for (const TemplateArgument &argument : pattern.templateArguments) {
                                TemplateArgument resolved = argument;
                                if (argument.kind == TemplateArgumentKind::Type && argument.type) {
                                    resolved = TemplateArgument::fromType(resolveType(
                                            *argument.type, current, declaration, evaluation));
                                } else if (argument.kind == TemplateArgumentKind::NonType) {
                                    QString value = evaluateConstantValue(
                                            argument.constantExpression, current, declaration, evaluation);
                                    resolved = TemplateArgument::fromConstant(
                                            value.isEmpty() ? argument.constantExpression : value);
                                }
                                arguments.append(resolved);
                            }
                        } else {
                            for (const TemplateParameterInfo &parameter : std::dynamic_pointer_cast<Statement>(declaration)->templateParameters) {
                                auto argument = current->values.constFind(parameter.name);
                                if (argument == current->values.constEnd())
                                    break;
                                arguments.append(argument.value());
                            }
                            if (arguments.count() != std::dynamic_pointer_cast<Statement>(declaration)->templateParameters.count())
                                arguments.clear();
                        }
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
            QSet<QString> visited;
            QList<MemberResolution> candidates = memberCandidates(
                    {instance, declaration, environment}, segments[index + 1].name,
                    visited, evaluation);
            if (!candidates.isEmpty())
                member = candidates.first();
        } else if (declaration->kind == StatementKind::Namespace) {
            QStringList qualifiedParts;
            for (int i = 0; i <= index + 1; ++i)
                qualifiedParts.append(segments[i].name);
            member.statement = findDeclaration(qualifiedParts.join("::"), context, true);
            member.owner = {instance, declaration, environment};
        } else if (declaration->kind == StatementKind::EnumType
                   || declaration->kind == StatementKind::EnumClassType) {
            member.statement = findMemberDeclaration(declaration, segments[index + 1].name);
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
    bool cacheEnabled = evaluation.caches.testFlag(materializeArguments
                                                   ? CacheLevel::ParsedTypeName
                                                   : CacheLevel::ParsedTypePattern);
    auto cached = cacheEnabled ? cache->constFind(text) : cache->constEnd();
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
        QString trimmed = argumentText.trimmed();
        if (trimmed.endsWith("...")
                && (trimmed.startsWith('(') || trimmed.contains("::value"))) {
            return TemplateArgument::fromConstant(trimmed);
        }
        if (!materializeArguments) {
            TemplateArgument result;
            result.kind = TemplateArgumentKind::Type;
            result.constantExpression = trimmed;
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
        while (index < tokens.count()
               && tokens[index].kind == CppTokenizer::TokenKind::Identifier) {
            const QString word = tokens[index++].text;
            if (word == "const")
                segment.qualifiers.setFlag(TypeQualifier::Const);
            else if (word == "volatile")
                segment.qualifiers.setFlag(TypeQualifier::Volatile);
            else
                appendText(segment.name, word);
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
                if (!argumentText.trimmed().isEmpty())
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
    if (cacheEnabled)
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
    bool cacheEnabled = evaluation.caches.testFlag(CacheLevel::ParsedTypeText);
    auto cached = cacheEnabled ? evaluation.parsedTypeTexts->constFind(text)
                               : evaluation.parsedTypeTexts->constEnd();
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
            for (const TypeNameSegment &segment : segments) {
                qualifiers |= segment.qualifiers;
                simpleType &= !segment.name.isEmpty() && !segment.name.startsWith("decltype")
                        && (!segment.name.contains(' ') || isBuiltinTypeName(segment.name));
                names.append(segment.name);
            }
            if (simpleType) {
                TypeInfo result;
                result.qualifiedName = (globallyQualified ? "::" : "") + names.join("::");
                result.qualifiedNameText = text;
                result.qualifiers = qualifiers;
                if (!segments.isEmpty())
                    result.templateArguments = segments.last().arguments;
                if (cacheEnabled)
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
    if (cacheEnabled && !result.qualifiedName.isEmpty())
        evaluation.parsedTypeTexts->insert(text, result);
    return result;
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
            && std::dynamic_pointer_cast<Statement>(result.declaration)->templateKind != TemplateDeclarationKind::Primary) {
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
    if (!result.bindings->complete)
        return {};
    if (result.declaration->kind == StatementKind::Class)
        return selectSpecialization(result, evaluation);
    return result;
}

CppTypeResolver::ClassInstance CppTypeResolver::selectSpecialization(
        const ClassInstance &primary, EvaluationContext &evaluation) const
{
    ResolverProfileScope profileScope(resolverProfile.specializationNs,
                                      resolverProfile.specializationCalls, "selectSpecialization");
    if (!primary.bindings || !primary.bindings->complete)
        return {};
    if (!primary.declaration || primary.declaration->kind != StatementKind::Class
            || std::dynamic_pointer_cast<Statement>(primary.declaration)->templateKind != TemplateDeclarationKind::Primary) {
        return primary;
    }
    QString cacheKey = QString::number(reinterpret_cast<quintptr>(primary.declaration.get()), 16) + "|"
            + QString::number(reinterpret_cast<quintptr>(primary.bindings.get()), 16);
    bool cacheEnabled = evaluation.caches.testFlag(CacheLevel::SelectedSpecialization);
    auto cached = cacheEnabled ? evaluation.selectedSpecializations->constFind(cacheKey)
                               : evaluation.selectedSpecializations->constEnd();
    traceSyntaxAttempt("selectSpecialization", cacheKey,
                       cached != evaluation.selectedSpecializations->constEnd());
    if (cached != evaluation.selectedSpecializations->constEnd())
        return cached.value();
    auto cacheResult = [&](const ClassInstance &selected) {
        if (cacheEnabled && selected.declaration
                && evaluation.operationCount <= 4096 && !evaluation.resolutionFailed) {
            evaluation.selectedSpecializations->insert(cacheKey, selected);
        }
        return selected;
    };
    if (resolverProfile.traceEnabled) {
        traceSpecialization(primary.declaration->fullName + "|"
                            + primary.type.toString());
    }
    ClassInstance result = primary;
    result.type.templateArguments.clear();
    for (const TemplateParameterInfo &parameter : std::dynamic_pointer_cast<Statement>(primary.declaration)->templateParameters) {
        if (parameter.parameterPack) {
            auto pack = primary.bindings->parameterPacks.constFind(parameter.name);
            if (pack == primary.bindings->parameterPacks.constEnd())
                return {};
            result.type.templateArguments += pack.value();
            continue;
        }
        auto argument = primary.bindings->values.constFind(parameter.name);
        if (argument == primary.bindings->values.constEnd())
            return {};
        result.type.templateArguments.append(argument.value());
    }

    PStatement parent = primary.declaration->parentScope.lock();
    QList<PStatement> scopes;
    scopes.append(parent);
    QString prefix = primary.declaration->command + "<";
    QList<PStatement> partialMatches;
    bool unknown = false;
    for (const PStatement &scope : scopes) {
        const StatementMap &children = mModel.childrenStatements(scope);
        for (auto it = children.lowerBound(prefix);
             it != children.end() && it.key().startsWith(prefix); ++it) {
            PStatement candidate = it.value().lock();
            if (candidate && candidate->kind == StatementKind::InlineAlias) {
                auto alias = std::dynamic_pointer_cast<InlineAliasStatement>(candidate);
                candidate = alias->target.lock();
            }
            if (!candidate || candidate->kind != StatementKind::Class
                    || (std::dynamic_pointer_cast<Statement>(candidate)->templateKind != TemplateDeclarationKind::ExplicitSpecialization
                        && std::dynamic_pointer_cast<Statement>(candidate)->templateKind != TemplateDeclarationKind::PartialSpecialization)) {
                continue;
            }
            TemplateBindings bindings;
            EvaluationContext candidateEvaluation = evaluation;
            SpecializationMatch match = matchSpecialization(candidate, result.type.templateArguments,
                                                            parent, bindings, candidateEvaluation);
            evaluation.operationCount = candidateEvaluation.operationCount;
            if (candidateEvaluation.resolutionFailed && match != SpecializationMatch::NoMatch)
                match = SpecializationMatch::Unknown;
            if (match == SpecializationMatch::Unknown) {
                unknown = true;
                continue;
            }
            if (match != SpecializationMatch::Match)
                continue;
            if (std::dynamic_pointer_cast<Statement>(candidate)->templateKind == TemplateDeclarationKind::ExplicitSpecialization) {
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
    TypeInfo pattern = std::dynamic_pointer_cast<Statement>(specialization)->templatePattern;
    Q_ASSERT(!pattern.qualifiedName.isEmpty());
    if (pattern.qualifiedName.isEmpty())
        return SpecializationMatch::Unknown;
    if (pattern.templateArguments.count() != arguments.count())
        return SpecializationMatch::NoMatch;
    QSet<QString> parameters;
    for (const TemplateParameterInfo &parameter : std::dynamic_pointer_cast<Statement>(specialization)->templateParameters)
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

TypeInfo CppTypeResolver::substituteTypePattern(
        const TypeInfo &pattern, const TemplateBindings &bindings,
        const QHash<QString, QVector<TemplateArgument>> &packs,
        EvaluationContext &evaluation) const
{
    TypeInfo result = pattern;
    materializeTemplateArgumentPatterns(result, evaluation);
    auto bound = bindings.constFind(result.qualifiedName);
    if (bound != bindings.constEnd()) {
        if (bound->kind != TemplateArgumentKind::Type || !bound->type)
            return {};
        TypeInfo replacement = *bound->type;
        applyReference(replacement, result);
        return replacement;
    }
    QVector<TemplateArgument> substituted;
    for (TemplateArgument argument : result.templateArguments) {
        if (argument.kind == TemplateArgumentKind::Type && !argument.type
                && !argument.constantExpression.trimmed().isEmpty()) {
            TypeInfo parsed = parseTypeText(argument.constantExpression, evaluation, false);
            if (!parsed.qualifiedName.isEmpty())
                argument.type = std::make_shared<TypeInfo>(parsed);
        }
        QString packName;
        bool packExpansion = false;
        if (argument.kind == TemplateArgumentKind::Type && argument.type) {
            packName = argument.type->qualifiedName;
            packExpansion = argument.constantExpression.trimmed().endsWith("...")
                    || argument.type->qualifiedNameText.trimmed().endsWith("...");
        } else if (argument.kind == TemplateArgumentKind::NonType) {
            packName = argument.constantExpression.trimmed();
            packExpansion = packName.endsWith("...");
            if (packExpansion)
                packName.chop(3);
        }
        auto pack = packs.constFind(packName);
        if (pack != packs.constEnd()) {
            if (!packExpansion)
                return {};
            substituted += pack.value();
            continue;
        }
        if (argument.kind == TemplateArgumentKind::Type && argument.type) {
            auto nonType = bindings.constFind(argument.type->qualifiedName);
            if (nonType != bindings.constEnd()
                    && nonType->kind == TemplateArgumentKind::NonType) {
                argument.kind = TemplateArgumentKind::NonType;
                argument.constantExpression = nonType->constantExpression;
                argument.type.reset();
                substituted.append(argument);
                continue;
            }
            TypeInfo nested = substituteTypePattern(*argument.type, bindings, packs, evaluation);
            if (nested.qualifiedName.isEmpty())
                return {};
            argument.type = std::make_shared<TypeInfo>(nested);
        }
        substituted.append(argument);
    }
    result.templateArguments = substituted;
    for (TypeDerivation &derivation : result.derivations) {
        auto member = bindings.constFind(derivation.memberClass);
        if (member != bindings.constEnd() && member->kind == TemplateArgumentKind::Type
                && member->type) {
            derivation.memberClass = member->type->qualifiedName;
        }
        for (ParameterInfo &parameter : derivation.parameters) {
            if (!parameter.type)
                continue;
            TypeInfo nested = substituteTypePattern(*parameter.type, bindings, packs, evaluation);
            if (nested.qualifiedName.isEmpty())
                return {};
            parameter.type = std::make_shared<TypeInfo>(nested);
        }
    }
    return result;
}

TypeInfo CppTypeResolver::expandTypePattern(
        const TypeInfo &type, const PStatement &context,
        QSet<const AbstractStatement *> &visited, EvaluationContext &evaluation) const
{
    TypeInfo instance = type;
    materializeTemplateArgumentPatterns(instance, evaluation);
    for (TemplateArgument &argument : instance.templateArguments) {
        if (argument.kind != TemplateArgumentKind::Type || argument.type)
            continue;
        TypeInfo parsed = parseTypeText(argument.constantExpression, evaluation, false);
        if (parsed.qualifiedName.isEmpty())
            return {};
        argument.type = std::make_shared<TypeInfo>(parsed);
    }
    PStatement declaration = instance.declaration.lock();
    if (!declaration || declaration->fullName != instance.qualifiedName)
        declaration = findDeclaration(instance.qualifiedName, context, true);
    if (!declaration)
        return instance;
    if (instance.declaration.lock() != declaration)
        instance.declaration = declaration;
    if (!isAlias(declaration)) {
        if (declaration->kind != StatementKind::Class)
            return instance;
        const auto &parameters = std::dynamic_pointer_cast<Statement>(declaration)->templateParameters;
        if (instance.templateArguments.count() >= parameters.count())
            return instance;
        TemplateBindings localBindings;
        QHash<QString, QVector<TemplateArgument>> packs;
        int argumentIndex = 0;
        for (const TemplateParameterInfo &parameter : parameters) {
            if (parameter.parameterPack) {
                QVector<TemplateArgument> pack;
                while (argumentIndex < instance.templateArguments.count())
                    pack.append(instance.templateArguments[argumentIndex++]);
                packs.insert(parameter.name, pack);
                continue;
            }
            if (argumentIndex < instance.templateArguments.count()) {
                localBindings.insert(parameter.name, instance.templateArguments[argumentIndex++]);
                continue;
            }
            QString defaultText = parameter.defaultArgumentText;
            if (defaultText.isEmpty()
                    && argumentIndex < std::dynamic_pointer_cast<Statement>(declaration)->inheritedDefaultArgumentTexts.count()) {
                defaultText = std::dynamic_pointer_cast<Statement>(declaration)
                        ->inheritedDefaultArgumentTexts[argumentIndex];
            }
            if (defaultText.isEmpty())
                return instance;
            TemplateArgument argument;
            if (parameter.kind == TemplateParameterKind::Type) {
                TypeInfo defaultType = parseTypeText(defaultText, evaluation, false);
                defaultType = substituteTypePattern(defaultType, localBindings, packs, evaluation);
                if (defaultType.qualifiedName.isEmpty())
                    return {};
                argument = TemplateArgument::fromType(defaultType);
            } else {
                QString expression = defaultText;
                for (auto it = localBindings.constBegin(); it != localBindings.constEnd(); ++it) {
                    if (it.value().kind == TemplateArgumentKind::NonType)
                        expression = substituteIdentifier(
                                    expression, it.key(), it.value().constantExpression);
                }
                argument = TemplateArgument::fromConstant(expression);
            }
            localBindings.insert(parameter.name, argument);
            instance.templateArguments.append(argument);
            ++argumentIndex;
        }
        return instance;
    }
    if (visited.contains(declaration.get()))
        return {};
    visited.insert(declaration.get());
    struct VisitGuard {
        QSet<const AbstractStatement *> &visited;
        const AbstractStatement *declaration;
        ~VisitGuard() { visited.remove(declaration); }
    } guard{visited, declaration.get()};

    TemplateBindings localBindings;
    QHash<QString, QVector<TemplateArgument>> packs;
    int argumentIndex = 0;
    const auto &parameters = std::dynamic_pointer_cast<Statement>(declaration)->templateParameters;
    for (const TemplateParameterInfo &parameter : parameters) {
        if (parameter.parameterPack) {
            QVector<TemplateArgument> pack;
            while (argumentIndex < instance.templateArguments.count())
                pack.append(instance.templateArguments[argumentIndex++]);
            packs.insert(parameter.name, pack);
            continue;
        }
        if (argumentIndex < instance.templateArguments.count()) {
            localBindings.insert(parameter.name, instance.templateArguments[argumentIndex++]);
            continue;
        }
        if (parameter.defaultArgumentText.isEmpty())
            return {};
        if (parameter.kind == TemplateParameterKind::Type) {
            TypeInfo defaultType = parseTypeText(
                        parameter.defaultArgumentText, evaluation, false);
            if (defaultType.qualifiedName.isEmpty())
                return {};
            localBindings.insert(parameter.name, TemplateArgument::fromType(defaultType));
        } else {
            localBindings.insert(parameter.name, TemplateArgument::fromConstant(
                    parameter.defaultArgumentText));
        }
    }
    if (argumentIndex != instance.templateArguments.count())
        return {};

    TypeInfo target = std::dynamic_pointer_cast<Statement>(declaration)->typeInfo;
    materializeTemplateArgumentPatterns(target, evaluation);
    TypeInfo substituted = substituteTypePattern(target, localBindings, packs, evaluation);
    if (substituted.qualifiedName.isEmpty())
        return {};
    TypeInfo expanded = expandTypePattern(
                substituted, declaration->parentScope.lock(), visited, evaluation);
    if (expanded.qualifiedName.isEmpty())
        return {};
    applyReference(expanded, instance);
    return expanded;
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

    materializeTemplateArgumentPatterns(formal, evaluation);
    materializeTemplateArguments(real, evaluation);
    auto isVoidT = [](const TypeInfo &type) {
        return type.qualifiedName == "__void_t" || type.qualifiedName == "std::void_t"
                || type.qualifiedName == "void_t";
    };
    if ((isVoidT(formal) && (isVoidT(real) || real.qualifiedName == "void"))
            || (isVoidT(real) && formal.qualifiedName == "void")) {
        for (const TypeInfo *type : {&formal, &real}) {
            if (!isVoidT(*type))
                continue;
            for (const TemplateArgument &argument : type->templateArguments) {
                if (argument.kind != TemplateArgumentKind::Type)
                    return SpecializationMatch::NoMatch;
                TypeInfo argumentType = argument.type
                        ? *argument.type : parseTypeText(argument.constantExpression, evaluation);
                if (argumentType.qualifiedName.isEmpty())
                    return SpecializationMatch::NoMatch;
                TypeInfo resolved = resolve(argumentType, bindings, context, &evaluation);
                if (resolved.qualifiedName.isEmpty()
                        || (!isBuiltinType(resolved) && !resolved.declaration.lock())) {
                    return SpecializationMatch::NoMatch;
                }
            }
        }
        TypeInfo voidType;
        voidType.qualifiedName = "void";
        formal = voidType;
        real = voidType;
    }
    if (formal.qualifiedName != real.qualifiedName) {
        PStatement formalDeclaration = findDeclaration(formal.qualifiedName, context, true);
        if (isAlias(formalDeclaration)) {
            QSet<const AbstractStatement *> visited;
            TypeInfo expanded = expandTypePattern(formal, context, visited, evaluation);
            if (expanded.qualifiedName.isEmpty())
                return SpecializationMatch::Unknown;
            if (expanded.qualifiedNameText.trimmed().startsWith("typename")) {
                auto formalBindings = std::make_shared<TemplateBindingFrame>();
                formalBindings->values = bindings;
                ClassInstance resolved = resolveTypeInstance(
                            formal, formalBindings, context, evaluation);
                if (resolved.type.qualifiedName.isEmpty())
                    return SpecializationMatch::Unknown;
                formal = resolved.type;
            } else {
                formal = expanded;
            }
            formalDeclaration = formal.declaration.lock();
            if (!formalDeclaration)
                formalDeclaration = findDeclaration(formal.qualifiedName, context, true);
        }
        PStatement realDeclaration = real.declaration.lock();
        if (isAlias(realDeclaration)) {
            auto actualBindings = std::make_shared<TemplateBindingFrame>();
            actualBindings->values = bindings;
            ClassInstance resolved = resolveTypeInstance(real, actualBindings, context, evaluation);
            real = resolved.type;
            realDeclaration = real.declaration.lock();
        }
        if (isBuiltinType(formal) || isBuiltinType(real)) {
            return isBuiltinType(formal) && isBuiltinType(real)
                    && formal.qualifiedName == real.qualifiedName
                    ? SpecializationMatch::Match : SpecializationMatch::NoMatch;
        }
        if (realDeclaration && realDeclaration->kind == StatementKind::Class) {
            TemplateDeclarationKind kind = std::dynamic_pointer_cast<Statement>(realDeclaration)->templateKind;
            if (kind == TemplateDeclarationKind::PartialSpecialization
                    || kind == TemplateDeclarationKind::ExplicitSpecialization) {
                realDeclaration = findDeclaration(realDeclaration->command.section('<', 0, 0),
                                                  realDeclaration->parentScope.lock(), true);
            }
        }
        if (!formalDeclaration || !realDeclaration || formalDeclaration != realDeclaration)
            return SpecializationMatch::NoMatch;
    }
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
                                                                evaluation);
    return candidates.isEmpty() ? MemberResolution{} : candidates.first();
}

QList<CppTypeResolver::MemberResolution> CppTypeResolver::memberCandidates(
        const ClassInstance &instance, const QString &name, QSet<QString> &visited,
        EvaluationContext &evaluation) const
{
    ResolverProfileScope profileScope(resolverProfile.memberLookupNs,
                                      resolverProfile.memberLookupCalls, "memberCandidates");
    QList<MemberResolution> result;
    if (!instance.declaration
            || (instance.declaration->kind != StatementKind::Class
                && instance.declaration->kind != StatementKind::Lambda
                && instance.declaration->kind != StatementKind::EnumType
                && instance.declaration->kind != StatementKind::EnumClassType)) {
        return result;
    }
    traceEvent(resolverProfile.memberTrace, instance.declaration->fullName + "::" + name);
    QString instanceKey = instance.bindings
            ? QString::number(reinterpret_cast<quintptr>(instance.declaration.get()), 16) + "|"
                + QString::number(reinterpret_cast<quintptr>(instance.bindings.get()), 16)
            : instance.declaration->fullName + "<" + profiledTypeString(instance.type) + ">";
    if (visited.contains(instanceKey))
        return result;
    visited.insert(instanceKey);
    const auto candidates = std::dynamic_pointer_cast<Statement>(instance.declaration)->children.values(name);
    QList<MemberResolution> constFallback;
    for (const std::weak_ptr<AbstractStatement> &weak : candidates) {
        PStatement candidate = weak.lock();
        if (!candidate)
            continue;
        bool qualifiedMember = (candidate->kind == StatementKind::Function
                                || candidate->kind == StatementKind::OverloadedOperator)
                && !statementIsStatic(candidate);
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
    if (!result.isEmpty())
        return result;

    QSet<QString> signatures;
    for (const MemberResolution &member : result)
        signatures.insert(member.statement->command + std::dynamic_pointer_cast<Statement>(member.statement)->normalizedArgs);
    for (QString baseText : std::dynamic_pointer_cast<Statement>(instance.declaration)->baseClasses) {
        baseText = baseText.trimmed();
        StatementAccessibility inheritance = std::dynamic_pointer_cast<Statement>(instance.declaration)->classKey == ClassKey::Struct
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
        ClassInstance base = resolveTypeInstance(baseType, instance.bindings,
                                           instance.declaration->parentScope.lock(), evaluation);
        const QList<MemberResolution> baseMembers = memberCandidates(base, name, visited, evaluation);
        for (MemberResolution member : baseMembers) {
            QString signature = member.statement->command + std::dynamic_pointer_cast<Statement>(member.statement)->normalizedArgs;
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
    bool cacheEnabled = mEvaluation && !mEvaluation->allowModelChanges
            && mEvaluation->caches.testFlag(CacheLevel::Declaration);
    LookupKey cacheKey{initialContext.get(), name, typesOnly};
    if (cacheEnabled) {
        auto cached = mEvaluation->declarationResults->constFind(cacheKey);
        if (resolverProfile.traceEnabled)
            traceSyntaxAttempt("findDeclaration", lookupKeyText(cacheKey),
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
        } else if (PStatement current = findMemberDeclaration(searchScope, parts.first(), typesOnly)) {
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
    if (parts.isEmpty())
        return nullptr;
    const auto candidates = mModel.childrenStatements(scope).values(parts.first());
    PStatement aliasResult;
    for (const std::weak_ptr<AbstractStatement> &weak : candidates) {
        PStatement candidate = weak.lock();
        if (!candidate || (typesOnly && !isTypeDeclaration(candidate)))
            continue;
        firstPartFound = true;
        if (candidate && candidate->kind == StatementKind::InlineAlias) {
                auto alias = std::dynamic_pointer_cast<InlineAliasStatement>(candidate);
            candidate = alias->target.lock();
            }
        PStatement result = findDeclarationRemainder(candidate, parts, typesOnly);
        if (!result)
            continue;
        if (result->kind != StatementKind::Alias)
            return result;
        if (!aliasResult)
            aliasResult = result;
    }
    return aliasResult;
}

PStatement CppTypeResolver::findDeclarationRemainder(const PStatement &statement,
                                                     const QStringList &parts,
                                                     bool typesOnly) const
{
    ResolverProfileScope profileScope(resolverProfile.declarationRemainderNs,
                                      resolverProfile.declarationRemainderCalls,
                                      "findDeclarationRemainder");
    if (statement && statement->kind == StatementKind::InlineAlias) {
        auto alias = std::dynamic_pointer_cast<InlineAliasStatement>(statement);
        return findDeclarationRemainder(alias->target.lock(), parts, typesOnly);
    }
    if (typesOnly && !isTypeDeclaration(statement))
        return nullptr;
    if (parts.count() > 1 && statement
            && statement->kind != StatementKind::Namespace
            && statement->kind != StatementKind::Class
            && statement->kind != StatementKind::EnumType
            && statement->kind != StatementKind::EnumClassType) {
        return nullptr;
    }
    QList<PStatement> current{statement};
    for (int i = 1; !current.isEmpty() && i < parts.count(); ++i) {
        QList<PStatement> next;
        QSet<const AbstractStatement *> seen;
        for (const PStatement &scope : current) {
            const auto candidates = mModel.childrenStatements(scope).values(parts[i]);
            for (const std::weak_ptr<AbstractStatement> &weak : candidates) {
                PStatement candidate = weak.lock();
                if (candidate && candidate->kind == StatementKind::InlineAlias)
                    candidate = std::dynamic_pointer_cast<InlineAliasStatement>(candidate)->target.lock();
                if (!candidate || seen.contains(candidate.get())
                        || (typesOnly && !isTypeDeclaration(candidate)))
                    continue;
                seen.insert(candidate.get());
                next.append(candidate);
            }
            PStatement inherited = findMemberDeclaration(scope, parts[i], typesOnly);
            if (inherited && !seen.contains(inherited.get())) {
                seen.insert(inherited.get());
                next.append(inherited);
            }
        }
        current = next;
    }
    return current.isEmpty() ? PStatement() : current.first();
}

PStatement CppTypeResolver::findMemberDeclaration(const PStatement &scope, const QString &name,
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
    bool cacheEnabled = mEvaluation && !mEvaluation->allowModelChanges
            && mEvaluation->caches.testFlag(CacheLevel::Member);
    LookupKey cacheKey{scope.get(), name, typesOnly};
    if (cacheEnabled) {
        ResolverProfileScope profileScope(resolverProfile.memberCacheNs,
                                          resolverProfile.memberCacheCalls, "memberCache");
        auto cached = mEvaluation->memberResults->constFind(cacheKey);
        if (resolverProfile.traceEnabled)
            traceSyntaxAttempt("findMember", lookupKeyText(cacheKey),
                               cached != mEvaluation->memberResults->constEnd());
        if (cached != mEvaluation->memberResults->constEnd())
            return cached.value();
    }
    auto finish = [&](PStatement result) {
        if (cacheEnabled)
            mEvaluation->memberResults->insert(cacheKey, result);
        return result;
    };
    QSet<const AbstractStatement *> visited;
    QStack<PStatement> pending;
    pending.push(scope);
    while (!pending.isEmpty()) {
        PStatement current = pending.pop();
        if (!current || visited.contains(current.get()))
            continue;
        visited.insert(current.get());
        ResolverProfileScope profileScope(resolverProfile.memberDfsNs,
                                          resolverProfile.memberDfsCalls, "memberDfs");
        const auto directCandidates = mModel.childrenStatements(current).values(name);
        for (int i = directCandidates.count() - 1; i >= 0; --i) {
            PStatement candidate = directCandidates[i].lock();
            if (candidate && candidate->kind == StatementKind::InlineAlias) {
                auto alias = std::dynamic_pointer_cast<InlineAliasStatement>(candidate);
                candidate = alias->target.lock();
            }
            if (!candidate || (typesOnly && !isTypeDeclaration(candidate)))
                continue;
            return finish(candidate);
        }
        QList<PStatement> bases;
        if (current->kind != StatementKind::Namespace) {
            for (QString baseName : std::dynamic_pointer_cast<Statement>(current)->baseClasses) {
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

const QVector<TemplateArgument> *CppTypeResolver::findParameterPack(
        const PTemplateBindingFrame &bindings, const QString &name) const
{
    for (PTemplateBindingFrame frame = bindings; frame; frame = frame->parent) {
        auto pack = frame->parameterPacks.constFind(name);
        if (pack != frame->parameterPacks.constEnd())
            return &pack.value();
    }
    return nullptr;
}

ConstraintSatisfaction CppTypeResolver::constraintsSatisfied(
        const PStatement &statement, const PTemplateBindingFrame &bindings,
        const PStatement &context, EvaluationContext &evaluation) const
{
    if (mOptions.cppVersion < CppVersion::Cpp20)
        return ConstraintSatisfaction::Satisfied;
    if (!statement || statement->kind == StatementKind::Namespace
            || statement->kind == StatementKind::InlineAlias) {
        return ConstraintSatisfaction::Satisfied;
    }
    auto normal = std::dynamic_pointer_cast<Statement>(statement);
    ConstraintSatisfaction result = ConstraintSatisfaction::Satisfied;
    if (!normal->templateRequiresClause.isEmpty()) {
        result = evaluateConstraint(normal->templateRequiresClause, bindings, context, evaluation);
        if (result == ConstraintSatisfaction::Unsatisfied)
            return result;
    }
    if (!normal->requiresClause.isEmpty())
        result = constraintAnd(result, evaluateConstraint(normal->requiresClause, bindings,
                                                          context, evaluation));
    return result;
}

ConstraintSatisfaction CppTypeResolver::evaluateConstraint(
        const QString &text, const PTemplateBindingFrame &bindings,
        const PStatement &context, EvaluationContext &evaluation) const
{
    if (++evaluation.operationCount > 4096)
        return ConstraintSatisfaction::Unknown;
    ConstraintParser parser(text, [&](const QString &atom) {
        return evaluateConstraintAtom(atom, bindings, context, evaluation);
    });
    return parser.evaluate();
}

ConstraintSatisfaction CppTypeResolver::evaluateConstraintAtom(
        const QString &text, const PTemplateBindingFrame &bindings,
        const PStatement &context, EvaluationContext &evaluation) const
{
    QString constant = evaluateConstantValue(text, bindings, context, evaluation);
    if (constant == "true")
        return ConstraintSatisfaction::Satisfied;
    if (constant == "false")
        return ConstraintSatisfaction::Unsatisfied;
    bool ok = false;
    qlonglong value = constant.toLongLong(&ok, 0);
    if (ok)
        return value != 0 ? ConstraintSatisfaction::Satisfied : ConstraintSatisfaction::Unsatisfied;
    return ConstraintSatisfaction::Unknown;
}

QString CppTypeResolver::evaluateConstantValue(
        const QString &text, const PTemplateBindingFrame &bindings,
        const PStatement &context, EvaluationContext &evaluation) const
{
    QString expression = text.trimmed();
    if (expression.startsWith("noexcept(") && expression.endsWith(')')) {
        ConstraintSatisfaction result = evaluateNoexceptExpression(
                    expression, context, bindings, &evaluation);
        if (result == ConstraintSatisfaction::Satisfied)
            return "true";
        if (result == ConstraintSatisfaction::Unsatisfied)
            return "false";
        return {};
    }
    if (expression == "true" || expression == "false")
        return expression;
    QString literal = expression;
    while (literal.endsWith('U') || literal.endsWith('u')
           || literal.endsWith('L') || literal.endsWith('l')) {
        literal.chop(1);
    }
    bool literalOk = false;
    qlonglong literalValue = literal.toLongLong(&literalOk, 0);
    if (literalOk)
        return QString::number(literalValue);

    CppTokenizer tokenizer;
    tokenizer.tokenize({expression});
    if (tokenizer.tokenCount() >= 2 && tokenizer[0].text == "("
            && tokenizer[0].matchIndex == tokenizer.tokenCount() - 1) {
        expression = expression.mid(1, expression.size() - 2).trimmed();
        tokenizer.tokenize({expression});
    }
    int angleLevel = 0;
    for (int i = 0; i < tokenizer.tokenCount(); ++i) {
        const CppTokenizer::Token &token = tokenizer[i];
        if (token.matchIndex > i) {
            i = token.matchIndex;
            continue;
        }
        if (token.text == "<") {
            ++angleLevel;
        } else if (token.text == ">") {
            angleLevel = qMax(0, angleLevel - 1);
        } else if (token.text == ">>") {
            angleLevel = qMax(0, angleLevel - 2);
        } else if (angleLevel == 0 && (token.text == "&&" || token.text == "||")) {
            QString left;
            QString right;
            for (int j = 0; j < i; ++j)
                appendConstraintToken(left, tokenizer[j].text);
            for (int j = i + 1; j < tokenizer.tokenCount(); ++j)
                appendConstraintToken(right, tokenizer[j].text);
            QString leftValue = evaluateConstantValue(left, bindings, context, evaluation);
            QString rightValue = evaluateConstantValue(right, bindings, context, evaluation);
            if (token.text == "&&") {
                if (leftValue == "false" || rightValue == "false")
                    return "false";
                if (leftValue == "true" && rightValue == "true")
                    return "true";
            } else {
                if (leftValue == "true" || rightValue == "true")
                    return "true";
                if (leftValue == "false" && rightValue == "false")
                    return "false";
            }
            return {};
        } else if (angleLevel == 0 && (token.text == "==" || token.text == "!=")) {
            QString left;
            QString right;
            for (int j = 0; j < i; ++j)
                appendConstraintToken(left, tokenizer[j].text);
            for (int j = i + 1; j < tokenizer.tokenCount(); ++j)
                appendConstraintToken(right, tokenizer[j].text);
            QString leftValue = evaluateConstantValue(left, bindings, context, evaluation);
            QString rightValue = evaluateConstantValue(right, bindings, context, evaluation);
            auto integerValue = [](QString value, qlonglong &result) {
                value = value.trimmed();
                if (value == "true" || value == "false") {
                    result = value == "true" ? 1 : 0;
                    return true;
                }
                while (value.endsWith('U') || value.endsWith('u')
                       || value.endsWith('L') || value.endsWith('l')) {
                    value.chop(1);
                }
                bool ok = false;
                result = value.toLongLong(&ok, 0);
                return ok;
            };
            qlonglong leftNumber = 0;
            qlonglong rightNumber = 0;
            if (!integerValue(leftValue, leftNumber) || !integerValue(rightValue, rightNumber))
                return QString();
            bool equal = leftNumber == rightNumber;
            return (token.text == "==") == equal ? QString("true") : QString("false");
        }
    }

    QString constant = resolveConstantExpression(expression, bindings, context, evaluation);
    if (constant == "true" || constant == "false")
        return constant;
    QString numeric = constant;
    while (numeric.endsWith('U') || numeric.endsWith('u')
           || numeric.endsWith('L') || numeric.endsWith('l')) {
        numeric.chop(1);
    }
    bool numericOk = false;
    qlonglong numericValue = numeric.toLongLong(&numericOk, 0);
    if (numericOk)
        return QString::number(numericValue);

    const QStringList &constantBuiltins = constantBuiltinsForCompiler(mOptions.compilerType);
    const QString constructibleBuiltin = "__is_constructible(";
    if (constantBuiltins.contains(constructibleBuiltin)
            && expression.startsWith(constructibleBuiltin) && expression.endsWith(')')) {
        ConstraintSatisfaction result = evaluateConstructibleExpression(
                    expression, context, bindings, evaluation);
        if (result == ConstraintSatisfaction::Satisfied)
            return "true";
        if (result == ConstraintSatisfaction::Unsatisfied)
            return "false";
        return {};
    }
    const QString arrayBuiltin = "__is_array(";
    if (constantBuiltins.contains(arrayBuiltin)
            && expression.startsWith(arrayBuiltin) && expression.endsWith(')')) {
        int argumentStart = arrayBuiltin.length();
        TypeInfo argument = parseTypeText(
                expression.mid(argumentStart, expression.length() - argumentStart - 1), evaluation);
        argument = resolveType(argument, bindings, context, evaluation);
        if (argument.qualifiedName.isEmpty())
            return QString();
        for (const TypeDerivation &derivation : argument.derivations) {
            if (derivation.kind == TypeDerivationKind::Array)
                return "true";
        }
        return "false";
    }

    for (const QString &builtin : constantBuiltins) {
        if (!expression.startsWith(builtin) || !expression.endsWith(")"))
            continue;
        QString argumentsText = expression.mid(builtin.size(),
                                               expression.size() - builtin.size() - 1);
        CppTokenizer argumentsTokenizer;
        argumentsTokenizer.tokenize({argumentsText});
        QStringList argumentParts;
        int partStart = 0;
        int angleLevel = 0;
        const auto &tokens = argumentsTokenizer.tokens();
        for (int i = 0; i < tokens.count(); ++i) {
            if (tokens[i].matchIndex > i) {
                i = tokens[i].matchIndex;
                continue;
            }
            if (tokens[i].text == "<") {
                ++angleLevel;
                continue;
            }
            if (tokens[i].text == ">" || tokens[i].text == ">>") {
                angleLevel = qMax(0, angleLevel - tokens[i].text.size());
                continue;
            }
            if (tokens[i].text != "," || angleLevel != 0)
                continue;
            argumentParts.append(argumentsText.mid(partStart, tokens[i].position.ch - partStart));
            partStart = tokens[i].position.ch + 1;
        }
        argumentParts.append(argumentsText.mid(partStart));
        TypeInfo firstArgument;
        QString firstValue;
        for (const QString &part : argumentParts) {
            TypeInfo argument = parseTypeText(part.trimmed(), evaluation);
            argument = resolveType(argument, bindings, context, evaluation);
            if (argument.qualifiedName.isEmpty())
                return {};
            while (!argument.derivations.isEmpty()
                   && (argument.derivations.first().kind == TypeDerivationKind::LvalueReference
                       || argument.derivations.first().kind == TypeDerivationKind::RvalueReference)) {
                argument.derivations.removeFirst();
            }
            argument.qualifiers = TypeQualifier::None;
            if (firstValue.isEmpty())
                firstValue = argument.toString();
            else if (argument.toString() != firstValue)
                return {};
            if (firstArgument.qualifiedName.isEmpty())
                firstArgument = argument;
        }
        PStatement declaration = firstArgument.declaration.lock();
        if (!declaration)
            declaration = findDeclaration(firstArgument.qualifiedName, context, true);
        if (declaration && declaration->kind == StatementKind::Class) {
            for (const std::weak_ptr<AbstractStatement> &weak
                 : std::dynamic_pointer_cast<Statement>(declaration)->children) {
                PStatement member = weak.lock();
                if (!member || member->kind != StatementKind::Destructor)
                    continue;
                if (std::dynamic_pointer_cast<Statement>(member)->definitionKind
                            == DeclaratorDefinitionKind::Defaulted
                        && builtin == "__is_trivially_destructible(") {
                    return "true";
                }
                return "false";
            }
            return {};
        }
        if (isBuiltinType(firstArgument)
                || (!firstArgument.derivations.isEmpty()
                    && firstArgument.derivations.first().kind == TypeDerivationKind::Pointer)) {
            return "true";
        }
        return {};
    }

    QString variableExpression = expression;
    bool classValue = variableExpression.endsWith("::value");
    if (classValue)
        variableExpression.truncate(variableExpression.size() - QString("::value").size());
    TypeInfo variableType = parseTypeText(variableExpression, evaluation);
    if (variableType.qualifiedName.isEmpty())
        return QString();
    materializeTemplateArguments(variableType, evaluation);
    for (TemplateArgument &argument : variableType.templateArguments) {
        if (argument.kind != TemplateArgumentKind::Type || !argument.type)
            continue;
        TypeInfo resolved = resolveType(*argument.type, bindings, context, evaluation);
        if (resolved.qualifiedName.isEmpty())
            return QString();
        argument.type = std::make_shared<TypeInfo>(resolved);
    }
    PStatement variable;
    PTemplateBindingFrame variableBindings = bindings;
    auto isVariable = [](const PStatement &statement) {
        return statement && (statement->kind == StatementKind::Variable
                             || statement->kind == StatementKind::GlobalVariable
                             || statement->kind == StatementKind::LocalVariable);
    };
    if (classValue) {
        ClassInstance owner = resolveTypeInstance(variableType, bindings, context, evaluation);
        if (owner.declaration && owner.declaration->kind == StatementKind::Class) {
            MemberResolution member = findMember(owner, "value", evaluation);
            if (isVariable(member.statement)) {
                variable = member.statement;
                variableBindings = member.owner.bindings;
            }
        }
    }
    const QString &variableName = variableType.qualifiedNameText.isEmpty()
            ? variableType.qualifiedName : variableType.qualifiedNameText;
    int scopeSeparator = variableName.lastIndexOf("::");
    if (!variable && scopeSeparator >= 0) {
        TypeInfo ownerType = parseTypeText(variableName.left(scopeSeparator), evaluation);
        if (!ownerType.qualifiedName.isEmpty()) {
            ClassInstance owner = resolveTypeInstance(ownerType, bindings, context, evaluation);
            if (owner.declaration) {
                MemberResolution member = findMember(
                        owner, variableName.mid(scopeSeparator + 2), evaluation);
                variable = member.statement;
                variableBindings = member.owner.bindings;
            }
        }
    }
    if (!variable)
        variable = findDeclaration(variableType.qualifiedName, context);
    if (!isVariable(variable))
        return QString();
    if (variable->command == "__is_invocable_r_v" || variable->command == "is_invocable_r_v") {
        ConstraintSatisfaction result = evaluateInvocableR(
                    variableType.templateArguments, context, bindings, evaluation);
        if (result == ConstraintSatisfaction::Satisfied)
            return "true";
        if (result == ConstraintSatisfaction::Unsatisfied)
            return "false";
        return {};
    }
    const auto normalVariable = std::dynamic_pointer_cast<Statement>(variable);
    bool variableTemplate = normalVariable->templateKind != TemplateDeclarationKind::None;
    bool staticConstMember = statementIsStatic(variable)
            && normalVariable->typeInfo.qualifiers.testFlag(TypeQualifier::Const);
    if (!variableTemplate && !staticConstMember)
        return QString();

    PStatement selected = variable;
    PStatement partialMatch;
    bool unknownMatch = false;
    PStatement parent = variable->parentScope.lock();
    const StatementMap &siblings = mModel.childrenStatements(parent);
    for (auto it = siblings.lowerBound(variable->command);
         it != siblings.end() && it.key().startsWith(variable->command); ++it) {
        PStatement candidate = it.value().lock();
        if (!isVariable(candidate) || candidate == variable)
            continue;
        TemplateDeclarationKind candidateKind =
                std::dynamic_pointer_cast<Statement>(candidate)->templateKind;
        if (candidateKind != TemplateDeclarationKind::ExplicitSpecialization
                && candidateKind != TemplateDeclarationKind::PartialSpecialization) {
            continue;
        }
        TemplateBindings captured;
        SpecializationMatch match = matchSpecialization(candidate, variableType.templateArguments,
                                                        context, captured, evaluation);
        if (match == SpecializationMatch::NoMatch)
            continue;
        if (match == SpecializationMatch::Unknown) {
            unknownMatch = true;
            continue;
        }
        if (candidateKind == TemplateDeclarationKind::ExplicitSpecialization) {
            selected = candidate;
            partialMatch.reset();
            unknownMatch = false;
            break;
        }
        if (partialMatch)
            unknownMatch = true;
        else
            partialMatch = candidate;
    }
    if (unknownMatch)
        return QString();
    if (partialMatch)
        selected = partialMatch;

    auto normal = std::dynamic_pointer_cast<Statement>(selected);
    if (normal->value.isEmpty())
        return QString();
    QString key = QString::number(reinterpret_cast<quintptr>(selected.get()), 16) + "|"
            + profiledTypeString(variableType);
    if (evaluation.resolvingConstraints.contains(key))
        return QString();
    ResolutionGuard guard(evaluation.resolvingConstraints, key);
    PTemplateBindingFrame localBindings = declarationBindings(selected, variableType, variableBindings,
                                                               evaluation);
    if (!localBindings->complete)
        return QString();
    return evaluateConstantValue(normal->value, localBindings, selected->parentScope.lock(), evaluation);
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
    QString logicalName = traitType.qualifiedName.section("::", -1);
    QVector<TemplateArgument> logicalArguments = traitType.templateArguments;
    PTemplateBindingFrame logicalBindings = bindings;
    PStatement logicalContext = context;
    QSet<const AbstractStatement *> visitedAliases;
    PStatement logicalDeclaration = findDeclaration(traitType.qualifiedName, context, true);
    while (isAlias(logicalDeclaration) && !visitedAliases.contains(logicalDeclaration.get())) {
        visitedAliases.insert(logicalDeclaration.get());
        const auto alias = std::dynamic_pointer_cast<Statement>(logicalDeclaration);
        QString target = alias->value.isEmpty() ? alias->typeInfo.qualifiedName : alias->value;
        if (target.isEmpty())
            break;
        logicalName = target.section('<', 0, 0).section("::", -1);
        TypeInfo targetType = parseTypeText(target, evaluation);
        logicalArguments = targetType.templateArguments;
        for (TemplateArgument &argument : traitType.templateArguments) {
            if (argument.kind == TemplateArgumentKind::Type && argument.type) {
                TypeInfo resolved = resolveType(*argument.type, logicalBindings,
                                                logicalContext, evaluation);
                if (resolved.qualifiedName.isEmpty())
                    return text.trimmed();
                argument.type = std::make_shared<TypeInfo>(resolved);
            } else if (argument.kind == TemplateArgumentKind::NonType) {
                QString value = evaluateConstantValue(
                            argument.constantExpression, logicalBindings, logicalContext, evaluation);
                if (!value.isEmpty())
                    argument.constantExpression = value;
            }
        }
        logicalBindings = declarationBindings(logicalDeclaration, traitType,
                                              logicalBindings, evaluation);
        logicalContext = logicalDeclaration->parentScope.lock();
        traitType = targetType;
        logicalDeclaration = findDeclaration(target.section('<', 0, 0),
                                             logicalContext, true);
    }
    if (logicalName == "__not_") {
        if (logicalArguments.count() != 1)
            return text.trimmed();
        QString value = evaluateConstantValue(
                    logicalArguments.first().toString() + "::value",
                    logicalBindings, logicalContext, evaluation);
        if (value == "true")
            return "false";
        if (value == "false")
            return "true";
        return text.trimmed();
    }
    if (logicalName == "__and_" || logicalName == "__or_" || logicalName == "__all") {
        if (!supportsConstantTrait(logicalName))
            return text.trimmed();
        ConstraintSatisfaction result = evaluateLogicalTrait(
                    logicalArguments, logicalName != "__or_", logicalContext,
                    logicalBindings, evaluation);
        if (result == ConstraintSatisfaction::Satisfied)
            return "true";
        if (result == ConstraintSatisfaction::Unsatisfied)
            return "false";
        return text.trimmed();
    }
    EvaluationContext traitEvaluation = evaluation;
    ClassInstance trait = resolveTypeInstance(
                traitType, logicalBindings, logicalContext, traitEvaluation);
    if (!trait.declaration || trait.type.templateArguments.isEmpty()
            || trait.type.templateArguments.first().kind != TemplateArgumentKind::Type
            || !trait.type.templateArguments.first().type) {
        return text.trimmed();
    }
    const TypeInfo &argument = *trait.type.templateArguments.first().type;
    const QString traitName = trait.declaration->command.section('<', 0, 0);
    if (!supportsConstantTrait(traitName))
        return text.trimmed();
    if (traitName == "__has_iterator_typedefs") {
        static const QStringList members{
            "value_type", "difference_type", "pointer", "reference", "iterator_category"
        };
        for (const QString &member : members) {
            if (!memberStatement(argument, member, &traitEvaluation))
                return "false";
        }
        return "true";
    }
    if (traitName == "__is_fast_hash") {
        MemberResolution value = findMember(trait, "value", evaluation);
        if (value.statement && (value.statement->kind == StatementKind::Variable
                                || value.statement->kind == StatementKind::GlobalVariable
                                || value.statement->kind == StatementKind::LocalVariable)) {
            const auto normal = std::dynamic_pointer_cast<Statement>(value.statement);
            if (!normal->value.isEmpty())
                return evaluateConstantValue(normal->value, value.owner.bindings,
                                             value.statement->parentScope.lock(), evaluation);
        }
        return text.trimmed();
    }
    if (traitName == "__is_nothrow_invocable" || traitName == "is_nothrow_invocable") {
        EvalResult evaluated = evaluateInvocation(
                    trait.type.templateArguments, 0, logicalContext, logicalBindings, evaluation);
        ConstraintSatisfaction result = evaluateNoexceptCall(evaluated, evaluation);
        if (result == ConstraintSatisfaction::Satisfied)
            return "true";
        if (result == ConstraintSatisfaction::Unsatisfied)
            return "false";
        return text.trimmed();
    }
    if (traitName == "integral_constant"
            && trait.type.templateArguments.count() == 2
            && trait.type.templateArguments[1].kind == TemplateArgumentKind::NonType) {
        return trait.type.templateArguments[1].constantExpression;
    }
    if (traitName == "is_array") {
        for (const TypeDerivation &derivation : argument.derivations) {
            if (derivation.kind == TypeDerivationKind::Array)
                return "true";
        }
        return "false";
    }
    if (traitName == "is_void")
        return argument.qualifiedName == "void" && argument.derivations.isEmpty()
                ? "true" : "false";
    if (traitName == "is_reference" || traitName == "is_lvalue_reference"
            || traitName == "is_rvalue_reference") {
        bool lvalue = !argument.derivations.isEmpty()
                && argument.derivations.first().kind == TypeDerivationKind::LvalueReference;
        bool rvalue = !argument.derivations.isEmpty()
                && argument.derivations.first().kind == TypeDerivationKind::RvalueReference;
        bool result = traitName == "is_lvalue_reference" ? lvalue
                : traitName == "is_rvalue_reference" ? rvalue : lvalue || rvalue;
        return result ? "true" : "false";
    }
    if (traitName == "is_copy_constructible" || traitName == "is_move_constructible"
            || traitName == "is_default_constructible") {
        QVector<TypeInfo> arguments;
        if (traitName != "is_default_constructible") {
            TypeInfo source = argument;
            if (traitName == "is_copy_constructible") {
                source.qualifiers |= TypeQualifier::Const;
                source = applyTypeBuiltin("__add_lvalue_reference", source);
            } else {
                source = applyTypeBuiltin("__add_rvalue_reference", source);
            }
            arguments.append(source);
        }
        ConstraintSatisfaction result = evaluateConstructible(
                    argument, arguments, logicalContext, logicalBindings, evaluation);
        if (result == ConstraintSatisfaction::Satisfied)
            return "true";
        if (result == ConstraintSatisfaction::Unsatisfied)
            return "false";
        return text.trimmed();
    }
    if (traitName == "is_convertible" && trait.type.templateArguments.count() == 2
            && trait.type.templateArguments[0].kind == TemplateArgumentKind::Type
            && trait.type.templateArguments[1].kind == TemplateArgumentKind::Type
            && trait.type.templateArguments[0].type && trait.type.templateArguments[1].type) {
        TypeInfo target = resolveType(*trait.type.templateArguments[1].type,
                                      logicalBindings, logicalContext, evaluation);
        TypeInfo source = resolveType(*trait.type.templateArguments[0].type,
                                      logicalBindings, logicalContext, evaluation);
        TypeInfo resolvedTarget;
        bool convertible = compatibleArgument(target, source, {}, logicalContext,
                                              evaluation, resolvedTarget);
        return convertible ? "true" : "false";
    }
    static const QHash<QString, QString> trivialTraits{
        {"is_trivially_destructible", "__is_trivially_destructible(%1)"},
        {"is_trivially_constructible", "__is_trivially_constructible(%1)"},
        {"is_trivially_default_constructible", "__is_trivially_constructible(%1)"},
        {"is_trivially_copy_constructible", "__is_trivially_constructible(%1,const %1&)"},
        {"is_trivially_move_constructible", "__is_trivially_constructible(%1,%1&&)"},
        {"is_trivially_assignable", "__is_trivially_assignable(%1)"},
        {"is_trivially_copy_assignable", "__is_trivially_assignable(%1&,const %1&)"},
        {"is_trivially_move_assignable", "__is_trivially_assignable(%1&,%1&&)"},
        {"is_trivially_copyable", "__is_trivially_copyable(%1)"}
    };
    auto trivial = trivialTraits.constFind(traitName);
    if (trivial != trivialTraits.constEnd()) {
        QStringList arguments;
        for (const TemplateArgument &templateArgument : trait.type.templateArguments)
            arguments.append(templateArgument.toString());
        return evaluateConstantValue(trivial.value().arg(arguments.join(",")),
                                     logicalBindings, logicalContext, evaluation);
    }
    return text.trimmed();
}

TypeInfo CppTypeResolver::traitMemberType(const ClassInstance &instance,
                                          const QString &memberName,
                                          EvaluationContext &evaluation) const
{
    Q_UNUSED(evaluation);
    if (!instance.declaration || memberName != "type"
            || instance.type.templateArguments.count() != 1) {
        return {};
    }
    const QString &traitName = instance.declaration->command;
    if (!supportsTypeTrait(traitName))
        return {};
    const TemplateArgument &argument = instance.type.templateArguments.first();
    if (argument.kind != TemplateArgumentKind::Type || !argument.type)
        return {};
    return applyTypeBuiltin(traitName, *argument.type);
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
        QString value = evaluateConstantValue(text, bindings, context, evaluation);
        return TemplateArgument::fromConstant(value.isEmpty() ? text.trimmed() : value);
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
    bool cacheEnabled = evaluation.caches.testFlag(CacheLevel::BindingFrame);
    auto cached = cacheEnabled ? evaluation.bindingFrames->constFind(cacheKey)
                               : evaluation.bindingFrames->constEnd();
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
    int parameterCount = std::dynamic_pointer_cast<Statement>(declaration)->templateParameters.count();
    for (int i = 0; i < parameterCount; ++i) {
        const TemplateParameterInfo &parameter = std::dynamic_pointer_cast<Statement>(declaration)->templateParameters[i];
        if (parameter.parameterPack) {
            QVector<TemplateArgument> pack;
            for (int argumentIndex = i; argumentIndex < materialized.templateArguments.count();
                 ++argumentIndex) {
                pack.append(materialized.templateArguments[argumentIndex]);
            }
            ++result->revision;
            result->parameterPacks.insert(parameter.name, pack);
            continue;
        }
        TemplateArgument argument;
        if (i < materialized.templateArguments.count()) {
            argument = materialized.templateArguments[i];
        } else {
            QString defaultText = parameter.defaultArgumentText;
            if (defaultText.isEmpty() && i < std::dynamic_pointer_cast<Statement>(declaration)->inheritedDefaultArgumentTexts.count())
                defaultText = std::dynamic_pointer_cast<Statement>(declaration)->inheritedDefaultArgumentTexts[i];
            if (!defaultText.isEmpty()) {
                argument = resolveArgument(defaultText, parameter.kind, result,
                                           declaration->parentScope.lock(), evaluation);
            } else {
                result->complete = false;
                continue;
            }
        }
        if ((parameter.kind == TemplateParameterKind::Type
             && (argument.kind != TemplateArgumentKind::Type || !argument.type
                 || argument.type->qualifiedName.isEmpty()))
                || (parameter.kind != TemplateParameterKind::Type
                    && argument.constantExpression.isEmpty())) {
            result->complete = false;
            continue;
        }
        ++result->revision;
        result->values.insert(parameter.name, argument);
    }
    if (cacheEnabled && result->complete
            && evaluation.operationCount <= 4096 && !evaluation.resolutionFailed) {
        evaluation.bindingFrames->insert(cacheKey, result);
    }
    return result;
}
