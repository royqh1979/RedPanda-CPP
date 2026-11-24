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
#include "cppdeclarationparser.h"

#include <algorithm>
#include <QDebug>
#include <QElapsedTimer>
#include <QVarLengthArray>
#include "cppsyntaxparser.h"

#ifdef REDPANDA_PARSER_DECLARATION_PROFILE
#include <QDebug>
#include <QElapsedTimer>
#endif

namespace {
#ifdef REDPANDA_PARSER_DECLARATION_PROFILE
struct DeclarationProfile {
    bool enabled = qEnvironmentVariableIsSet("REDPANDA_PARSER_PROFILE");
    qint64 simpleMs = 0;
    qint64 specifiersMs = 0;
    qint64 declaratorMs = 0;
    qint64 parameterMs = 0;
    qint64 constructorCheckMs = 0;
    qint64 typeNameMs = 0;
    qint64 templateArgumentsMs = 0;
    qint64 templateTextMs = 0;
    qint64 simpleSelfNs = 0;
    qint64 specifiersSelfNs = 0;
    qint64 declaratorSelfNs = 0;
    qint64 parameterSelfNs = 0;
    qint64 constructorCheckSelfNs = 0;
    qint64 typeNameSelfNs = 0;
    qint64 templateArgumentsSelfNs = 0;
    qint64 templateTextSelfNs = 0;
    qint64 constructorCheckCalls = 0;
    qint64 templateTextCalls = 0;
    qint64 templateTextTokens = 0;
    qint64 sourceTextCalls = 0;
    qint64 normalizedTextCalls = 0;
    qint64 simpleCalls = 0;
    qint64 simpleFailures = 0;
    qint64 failedTemplateTextTokens = 0;
    ~DeclarationProfile()
    {
        if (!enabled)
            return;
        qWarning() << "DECLARATION_PROFILE" << simpleMs / 1000000.0
                   << specifiersMs / 1000000.0 << declaratorMs / 1000000.0
                   << parameterMs / 1000000.0
                   << "constructorCheck" << constructorCheckMs / 1000000.0
                   << "typeName" << typeNameMs / 1000000.0
                   << "templateArguments" << templateArgumentsMs / 1000000.0
                   << "templateText" << templateTextMs / 1000000.0
                   << "calls" << templateTextCalls << "tokens" << templateTextTokens
                   << "simpleCalls" << simpleCalls << "simpleFailures" << simpleFailures
                   << "failedTemplateTokens" << failedTemplateTextTokens
                   << "sourceCalls" << sourceTextCalls << "normalizedCalls" << normalizedTextCalls;
        qWarning() << "DECLARATION_SELF"
                   << "simple" << simpleSelfNs / 1000000.0
                   << "specifiers" << specifiersSelfNs / 1000000.0
                   << "declarator" << declaratorSelfNs / 1000000.0
                   << "parameter" << parameterSelfNs / 1000000.0
                   << "constructorCheck" << constructorCheckSelfNs / 1000000.0
                   << "constructorCheckCalls" << constructorCheckCalls
                   << "typeName" << typeNameSelfNs / 1000000.0
                   << "templateArguments" << templateArgumentsSelfNs / 1000000.0
                   << "templateText" << templateTextSelfNs / 1000000.0;
    };
};

DeclarationProfile declarationProfile;

struct DeclarationProfileScope;
QVector<DeclarationProfileScope *> declarationProfileScopeStack;

struct DeclarationProfileScope {
    DeclarationProfileScope(qint64 &target, qint64 &selfTarget)
        : target(target), selfTarget(selfTarget),
          parent(declarationProfileScopeStack.isEmpty()
                         ? nullptr : declarationProfileScopeStack.last()) {
        if (declarationProfile.enabled) {
            declarationProfileScopeStack.append(this);
            timer.start();
        }
    }
    ~DeclarationProfileScope() {
        if (declarationProfile.enabled) {
            qint64 elapsed = timer.nsecsElapsed();
            target += elapsed;
            selfTarget += elapsed - childNs;
            if (parent)
                parent->childNs += elapsed;
            if (declarationProfileScopeStack.last() != this) {
                qWarning() << "DECLARATION_SCOPE_MISMATCH" << &target;
            } else {
                declarationProfileScopeStack.removeLast();
            }
        }
    }
    qint64 &target;
    qint64 &selfTarget;
    DeclarationProfileScope *parent;
    qint64 childNs = 0;
    QElapsedTimer timer;
};
#else
struct DeclarationProfileDisabled {
    qint64 simpleMs = 0;
    qint64 specifiersMs = 0;
    qint64 declaratorMs = 0;
    qint64 parameterMs = 0;
    qint64 constructorCheckMs = 0;
    qint64 typeNameMs = 0;
    qint64 templateArgumentsMs = 0;
    qint64 templateTextMs = 0;
    qint64 simpleSelfNs = 0;
    qint64 specifiersSelfNs = 0;
    qint64 declaratorSelfNs = 0;
    qint64 parameterSelfNs = 0;
    qint64 constructorCheckSelfNs = 0;
    qint64 typeNameSelfNs = 0;
    qint64 templateArgumentsSelfNs = 0;
    qint64 templateTextSelfNs = 0;
};

DeclarationProfileDisabled declarationProfile;

struct DeclarationProfileScope {
    DeclarationProfileScope(qint64 &, qint64 &) {}
};
#endif

void appendToken(QString &target, const QString &text)
{
    if (!target.isEmpty() && !text.isEmpty()
            && (target.back().isLetterOrNumber() || target.back() == '_')
            && (text.front().isLetterOrNumber() || text.front() == '_'))
        target += ' ';
    target += text;
}

void appendTypeDerivation(TypeInfo &type, const DeclaratorInfo &declarator)
{
    type.derivations += declarator.derivations;
}

bool isClangTypeBuiltin(const QString &text)
{
    static const QSet<QString> names{
        "__add_lvalue_reference", "__add_rvalue_reference", "__add_pointer",
        "__remove_extent", "__remove_reference_t", "__remove_cv", "__remove_cv_t"
    };
    return names.contains(text);
}

bool isNonDeclarationStartKeyword(const QString &text)
{
    static const QSet<QString> keywords{
        "_Pragma", "__attribute", "__attribute__", "__asm",
        "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel",
        "atomic_commit", "atomic_noexcept", "bitand", "bitor", "break",
        "case", "catch", "compl", "concept", "const_cast", "continue",
        "co_await", "co_return", "co_yield", "default", "delete", "do",
        "dynamic_cast", "else", "false", "for", "goto", "if",
        "new", "noexcept", "not", "not_eq", "nullptr", "or", "or_eq",
        "private", "protected", "public", "reflexpr", "reinterpret_cast",
        "requires", "return", "sizeof", "static_assert", "static_cast",
        "switch", "this", "throw", "true", "try", "typeid", "while",
        "xor", "xor_eq"
    };
    return keywords.contains(text);
}

bool isNormalizedSourceText(const QString &text)
{
    auto isWord = [](QChar ch) { return ch.isLetterOrNumber() || ch == '_'; };
    for (int i = 0; i < text.length(); ++i) {
        if (text[i] == '/')
            return false;
        if (!text[i].isSpace())
            continue;
        if (text[i] != ' ' || i == 0 || i + 1 >= text.length()
                || !isWord(text[i - 1]) || !isWord(text[i + 1])) {
            return false;
        }
    }
    return true;
}
}

CppDeclarationParser::CppDeclarationParser(CppTokenCursor &cursor)
    : mCursor(cursor)
{
}

bool CppDeclarationParser::parseDeclaration(DeclarationInfo &result)
{
    result = DeclarationInfo();
    if (!parseTemplatePrefix(result))
        return false;
    return parseSimpleDeclaration(result);
}

bool CppDeclarationParser::parseSimpleDeclaration(DeclarationInfo &result)
{
    DeclarationProfileScope profileScope(declarationProfile.simpleMs,
                                         declarationProfile.simpleSelfNs);
#ifdef REDPANDA_PARSER_DECLARATION_PROFILE
    ++declarationProfile.simpleCalls;
    qint64 templateTextTokensAtStart = declarationProfile.templateTextTokens;
#endif
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    if (isNonDeclarationStartKeyword(mCursor.currentText())) {
#ifdef REDPANDA_PARSER_DECLARATION_PROFILE
        ++declarationProfile.simpleFailures;
#endif
        return false;
    }
    auto outOfClassConstructor = [this] {
        if (mCursor.peek(-1).text != "::")
            return false;
        if (mCursor.peek(-2).text != ">")
            return mCursor.peek(-2).text == mCursor.currentText();
        int open = mCursor.peek(-2).matchIndex;
        if (open < 0)
            return false;
        CppTokenCursor::Checkpoint current = mCursor.checkpoint();
        CppTokenCursor::Checkpoint opening = current;
        opening.index = open;
        mCursor.restore(opening);
        QString className = mCursor.peek(-1).text;
        mCursor.restore(current);
        return className == mCursor.currentText();
    };
    bool deductionGuide = mCursor.peek().text == "("
            && mCursor.peek().matchIndex > mCursor.index()
            && mCursor.peek(mCursor.peek().matchIndex - mCursor.index() + 1).text == "->";
    bool withoutSpecifiers = mCursor.currentText() == "~"
            || mCursor.currentText() == "operator"
            || (mCursor.current().kind == CppTokenizer::TokenKind::Identifier
                && mCursor.currentText() != "explicit"
                && mCursor.currentText() != "decltype"
                && !isBuiltinType(mCursor.currentText())
                && !isClangTypeBuiltin(mCursor.currentText())
                && mCursor.peek().text == "("
                && (deductionGuide || (!currentClassName().isEmpty()
                     && mCursor.currentText() == currentClassName()
                        .section('<', 0, 0).section("::", -1, -1))
                    || outOfClassConstructor()));
    if (!withoutSpecifiers && !parseDeclSpecifierSeq(result.specifiers)) {
#ifdef REDPANDA_PARSER_DECLARATION_PROFILE
        ++declarationProfile.simpleFailures;
        declarationProfile.failedTemplateTextTokens +=
                declarationProfile.templateTextTokens - templateTextTokensAtStart;
#endif
        mCursor.restore(start);
        return false;
    }
    if (mCursor.currentText() == "[")
        return parseStructuredBinding(result);

    while (!mCursor.atEnd()) {
        DeclaratorInfo declarator;
        if (!parseDeclarator(declarator)) {
#ifdef REDPANDA_PARSER_DECLARATION_PROFILE
            ++declarationProfile.simpleFailures;
            declarationProfile.failedTemplateTextTokens +=
                    declarationProfile.templateTextTokens - templateTextTokensAtStart;
#endif
            mCursor.restore(start);
            result = DeclarationInfo();
            return false;
        }
        result.declarators.append(declarator);

        if (mCursor.consume("=")) {
            bool function = !declarator.derivations.isEmpty()
                    && declarator.derivations.first().kind == TypeDerivationKind::Function;
            bool explicitDefinition = false;
            if (function && mCursor.consume("default")) {
                result.declarators.last().definitionKind = DeclaratorDefinitionKind::Defaulted;
                explicitDefinition = true;
            } else if (function && mCursor.consume("delete")) {
                result.declarators.last().definitionKind = DeclaratorDefinitionKind::Deleted;
                explicitDefinition = true;
            }
            if (!explicitDefinition) {
                result.declarators.last().initializerPosition = mCursor.current().position;
                bool constantVariable = (result.templateKind != TemplateDeclarationKind::None
                                         || result.specifiers.properties.testFlag(DeclSpecifierProperty::Static))
                        && (result.specifiers.properties.testFlag(DeclSpecifierProperty::Constexpr)
                            || result.specifiers.type.qualifiers.testFlag(TypeQualifier::Const));
                if (result.specifiers.type.qualifiedName == "auto" || constantVariable) {
                    result.declarators.last().initializerText = readUntilDelimiter(
                                {",", ";"}, &result.declarators.last());
                } else {
                    skipExpression({",", ";"}, &result.declarators.last());
                }
            }
        }
        if (!mCursor.atEnd() && mCursor.currentText() == "{") {
            bool function = !declarator.derivations.isEmpty()
                    && declarator.derivations.first().kind == TypeDerivationKind::Function;
            if (!function) {
                result.declarators.last().initializerPosition = mCursor.current().position;
                skipBalancedToken();
            }
        }
        if (!mCursor.atEnd() && mCursor.currentText() == "(")
            skipBalancedToken();
        if (mCursor.consume(":")) {
            bool function = !declarator.derivations.isEmpty()
                    && declarator.derivations.first().kind == TypeDerivationKind::Function;
            skipExpression(function ? QSet<QString>{"{"} : QSet<QString>{",", ";"}, nullptr);
        }
        if (mCursor.consume(","))
            continue;
        mCursor.consume(";");
        break;
    }
    if (result.templateKind == TemplateDeclarationKind::Primary) {
        for (const DeclaratorInfo &declarator : result.declarators) {
            if (declarator.name.contains('<') && declarator.name.endsWith('>')) {
                result.templateKind = TemplateDeclarationKind::PartialSpecialization;
                break;
            }
        }
    }
    return !result.declarators.isEmpty();
}

bool CppDeclarationParser::parseStructuredBinding(DeclarationInfo &result)
{
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    result.structuredBinding = true;
    mCursor.advance();
    while (!mCursor.atEnd() && mCursor.currentText() != "]") {
        if (mCursor.current().kind != CppTokenizer::TokenKind::Identifier) {
            mCursor.restore(start);
            return false;
        }
        DeclaratorInfo declarator;
        declarator.name = mCursor.currentText();
        declarator.position = mCursor.current().position;
        declarator.structuredBindingIndex = result.declarators.count();
        result.declarators.append(declarator);
        mCursor.advance();
        if (mCursor.consume(","))
            continue;
        if (mCursor.currentText() != "]") {
            mCursor.restore(start);
            return false;
        }
    }
    if (!mCursor.consume("]")) {
        mCursor.restore(start);
        return false;
    }
    if (mCursor.consume("=")) {
        SourcePosition initializerPosition = mCursor.current().position;
        QString initializer = readUntilDelimiter({";"}, nullptr);
        for (DeclaratorInfo &declarator : result.declarators) {
            declarator.initializerText = initializer;
            declarator.initializerPosition = initializerPosition;
        }
    }
    mCursor.consume(";");
    return !result.declarators.isEmpty();
}

bool CppDeclarationParser::parseTemplatePrefix(DeclarationInfo &result)
{
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    mCursor.consume("export");
    while (mCursor.consume("template")) {
        if (!mCursor.consume("<")) {
            mCursor.restore(start);
            return false;
        }
        result.templateKind = TemplateDeclarationKind::Primary;
        if (mCursor.consume(">")) {
            result.templateKind = TemplateDeclarationKind::ExplicitSpecialization;
        } else {
            while (!mCursor.atEnd()) {
                TemplateParameterInfo parameter;
                if (mCursor.consume("template")) {
                    parameter.kind = TemplateParameterKind::TemplateTemplate;
                    if (mCursor.currentText() == "<")
                        readBalancedToken("<", ">");
                    if (!mCursor.consume("class"))
                        mCursor.consume("typename");
                } else if (mCursor.consume("class") || mCursor.consume("typename")) {
                    parameter.kind = TemplateParameterKind::Type;
                } else {
                    parameter.kind = TemplateParameterKind::NonType;
                    DeclSpecifierInfo specifiers;
                    if (!parseDeclSpecifierSeq(specifiers)) {
                        mCursor.restore(start);
                        return false;
                    }
                    if (mCursor.consume("..."))
                        parameter.parameterPack = true;
                    DeclaratorInfo declarator;
                    if (!parseDeclarator(declarator)) {
                        mCursor.restore(start);
                        return false;
                    }
                    parameter.name = declarator.name;
                    parameter.type = std::make_shared<TypeInfo>(specifiers.type);
                    appendTypeDerivation(*parameter.type, declarator);
                }
                if (mCursor.consume("..."))
                    parameter.parameterPack = true;
                if (parameter.name.isEmpty() && !mCursor.atEnd()
                        && mCursor.current().kind == CppTokenizer::TokenKind::Identifier) {
                    parameter.name = mCursor.currentText();
                    mCursor.advance();
                }
                if (mCursor.consume("=")) {
                    CppTokenCursor::Checkpoint defaultStart = mCursor.checkpoint();
                    skipTemplateDefaultArgument(parameter.kind == TemplateParameterKind::NonType);
                    parameter.defaultArgumentText = readConsumedText(defaultStart);
                }
                result.templateParameters.append(parameter);

                if (mCursor.consume(","))
                    continue;
                if (mCursor.currentText() == ">" || mCursor.currentText() == ">>") {
                    mCursor.consumeGreater();
                    break;
                }
                mCursor.restore(start);
                return false;
            }
        }
        if (mCursor.consume("requires")) {
            CppTokenCursor::Checkpoint requiresStart = mCursor.checkpoint();
            skipTemplateRequiresClause();
            if (mCppVersion >= CppVersion::Cpp20)
                result.templateRequiresClause = readConsumedText(requiresStart);
        }
    }
    return true;
}

void CppDeclarationParser::setCurrentClassName(const QString &className)
{
    mClassName = className.section("::", -1);
    int templateStart = mClassName.indexOf('<');
    if (templateStart >= 0)
        mClassName.truncate(templateStart);
}

bool CppDeclarationParser::skipTemplateArguments()
{
    if (mCursor.atEnd() || mCursor.currentText() != "<")
        return false;
    int level = 0;
    while (!mCursor.atEnd()) {
        const QString &text = mCursor.currentText();
        if (mCursor.current().kind == CppTokenizer::TokenKind::FileBoundary || text == ";")
            return false;
        // A non-negative matchIndex before the current index marks a closing
        // token such as ')', ']' or '}'. Opening tokens such as '(' use the branch below.
        if (mCursor.current().matchIndex >= 0
                && mCursor.current().matchIndex < mCursor.index()) {
            return false;
        }
        if (mCursor.current().matchIndex > mCursor.index()) {
            skipBalancedToken();
            continue;
        }
        if (mCursor.current().kind == CppTokenizer::TokenKind::Operator) {
            if (text == "<") {
                ++level;
                mCursor.advance();
                continue;
            }
            if (text == ">" || text == ">>") {
                mCursor.consumeGreater();
                if (--level == 0)
                    return true;
                continue;
            }
        }
        mCursor.advance();
    }
    return false;
}

bool CppDeclarationParser::isConstructorDeclaration()
{
    DeclarationProfileScope profileScope(declarationProfile.constructorCheckMs,
                                         declarationProfile.constructorCheckSelfNs);
#ifdef REDPANDA_PARSER_DECLARATION_PROFILE
    ++declarationProfile.constructorCheckCalls;
#endif
    CppTokenCursor::Checkpoint checkpoint = mCursor.checkpoint();
    QString previousPart;
    QString lastPart;
    int partCount = 0;
    bool hasOperator = false;
    mCursor.consume("::");
    while (mCursor.currentText() == "~"
           || mCursor.current().kind == CppTokenizer::TokenKind::Identifier) {
        mCursor.consume("~");
        if (mCursor.current().kind != CppTokenizer::TokenKind::Identifier) {
            mCursor.restore(checkpoint);
            return false;
        }
        if (mCursor.currentText() == "operator")
            hasOperator = true;
        previousPart = lastPart;
        lastPart = mCursor.currentText();
        ++partCount;
        mCursor.advance();
        if (mCursor.currentText() == "<" && !skipTemplateArguments()) {
            mCursor.restore(checkpoint);
            return false;
        }
        if (!mCursor.consume("::"))
            break;
    }
    if (hasOperator) {
        mCursor.restore(checkpoint);
        return true;
    }
    if (mCursor.currentText() != "(") {
        mCursor.restore(checkpoint);
        return false;
    }
    mCursor.restore(checkpoint);
    if (partCount == 0)
        return false;
    if (!mClassName.isEmpty())
        return lastPart == mClassName;
    return partCount >= 2 && lastPart == previousPart;
}

bool CppDeclarationParser::parseDeclSpecifierSeq(DeclSpecifierInfo &result)
{
    DeclarationProfileScope profileScope(declarationProfile.specifiersMs,
                                         declarationProfile.specifiersSelfNs);
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    result = DeclSpecifierInfo();
    bool hasType = false;
    bool constructor = false;
    while (!mCursor.atEnd()) {
        constructor = isConstructorDeclaration();
        if (constructor)
            break;
        const QString &text = mCursor.currentText();
        DeclSpecifierProperty property = DeclSpecifierProperty::None;
        if (text == "static") property = DeclSpecifierProperty::Static;
        else if (text == "extern") property = DeclSpecifierProperty::Extern;
        else if (text == "register") property = DeclSpecifierProperty::Register;
        else if (text == "thread_local") property = DeclSpecifierProperty::ThreadLocal;
        else if (text == "mutable") property = DeclSpecifierProperty::Mutable;
        else if (text == "inline") property = DeclSpecifierProperty::Inline;
        else if (text == "virtual") property = DeclSpecifierProperty::Virtual;
        else if (text == "friend") property = DeclSpecifierProperty::Friend;
        else if (text == "constexpr") property = DeclSpecifierProperty::Constexpr;
        else if (text == "consteval") property = DeclSpecifierProperty::Consteval;
        else if (text == "constinit") property = DeclSpecifierProperty::Constinit;
        else if (text == "explicit") property = DeclSpecifierProperty::Explicit;
        else if (text == "typedef") property = DeclSpecifierProperty::Typedef;
        if (property != DeclSpecifierProperty::None) {
            result.properties.setFlag(property);
            mCursor.advance();
            if (property == DeclSpecifierProperty::Explicit && mCursor.currentText() == "(")
                skipBalancedToken();
            continue;
        }
        if (text == "const" || text == "volatile") {
            result.type.qualifiers.setFlag(text == "const"
                    ? TypeQualifier::Const : TypeQualifier::Volatile);
            mCursor.advance();
            continue;
        }

        bool typeWord = isBuiltinType(text) || text == "struct" || text == "class"
                || text == "union" || text == "enum" || text == "decltype"
                || text == "typename"
                || (mCursor.current().kind == CppTokenizer::TokenKind::Identifier
                    && !isNonDeclarationStartKeyword(text));
        if (!typeWord || (hasType && !isBuiltinType(text)))
            break;

        TypeInfo part;
        if (!parseTypeName(part)) {
            mCursor.restore(start);
            return false;
        }
        if (hasType && isBuiltinType(part.qualifiedName)) {
            result.type.qualifiedName += " " + part.qualifiedName;
        } else {
            TypeQualifiers qualifiers = result.type.qualifiers;
            result.type = part;
            result.type.qualifiers |= qualifiers;
        }
        hasType = true;
    }
    if (!hasType && !constructor) {
        mCursor.restore(start);
        return false;
    }
    return true;
}

bool CppDeclarationParser::parseDeclarator(DeclaratorInfo &result)
{
    DeclarationProfileScope profileScope(declarationProfile.declaratorMs,
                                         declarationProfile.declaratorSelfNs);
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    result = DeclaratorInfo();
    result.position = mCursor.current().position;
    QVector<TypeDerivation> prefix;

    while (!mCursor.atEnd()) {
        TypeDerivation derivation;
        if (mCursor.consume("*")) {
            derivation = TypeDerivation::pointer();
        } else if (mCursor.consume("&&")) {
            derivation = TypeDerivation::rvalueReference();
        } else if (mCursor.consume("&")) {
            derivation = TypeDerivation::lvalueReference();
        } else {
            break;
        }
        while (!mCursor.atEnd()) {
            if (mCursor.consume("const"))
                derivation.qualifiers.setFlag(TypeQualifier::Const);
            else if (mCursor.consume("volatile"))
                derivation.qualifiers.setFlag(TypeQualifier::Volatile);
            else
                break;
        }
        prefix.append(derivation);
    }

    if (!mCursor.atEnd()
            && mCursor.current().kind == CppTokenizer::TokenKind::Identifier) {
        CppTokenCursor::Checkpoint memberStart = mCursor.checkpoint();
        QString memberClass = parseQualifiedNameText();
        if (memberClass.endsWith("::") && mCursor.consume("*")) {
            memberClass.chop(2);
            TypeDerivation derivation = TypeDerivation::memberPointer(memberClass);
            while (!mCursor.atEnd()) {
                if (mCursor.consume("const"))
                    derivation.qualifiers.setFlag(TypeQualifier::Const);
                else if (mCursor.consume("volatile"))
                    derivation.qualifiers.setFlag(TypeQualifier::Volatile);
                else
                    break;
            }
            prefix.append(derivation);
        } else {
            mCursor.restore(memberStart);
        }
    }

    result.parameterPack = mCursor.consume("...");
    if (mCursor.consume("(")) {
        if (!parseDeclarator(result)) {
            mCursor.restore(start);
            return false;
        }
        if (!mCursor.consume(")")) {
            mCursor.restore(start);
            return false;
        }
    } else if (!mCursor.atEnd()) {
        if (mCursor.currentText() == "operator") {
            result.name = parseOperatorName();
        } else if (mCursor.consume("~")) {
            result.name = "~" + parseQualifiedNameText();
        } else if (mCursor.current().kind == CppTokenizer::TokenKind::Identifier
                   || isBuiltinType(mCursor.currentText())) {
            CppTokenCursor::Checkpoint nameStart = mCursor.checkpoint();
            QString qualified = parseQualifiedNameText();
            CppTokenCursor::Checkpoint nameEnd = mCursor.checkpoint();
            int last = -1;
            int templateDepth = 0;
            for (int i = qualified.length() - 1; i > 0; --i) {
                if (qualified[i] == '>') {
                    ++templateDepth;
                } else if (qualified[i] == '<') {
                    templateDepth = qMax(0, templateDepth - 1);
                } else if (qualified[i] == ':' && qualified[i - 1] == ':'
                           && templateDepth == 0) {
                    last = i - 1;
                    break;
                }
            }
            if (last >= 0) {
                result.qualifier = qualified.left(last);
                result.name = qualified.mid(last + 2);
            } else {
                result.name = qualified;
            }
            result.globallyQualified = qualified.startsWith("::");
            if (result.globallyQualified) {
                result.qualifier.remove(0, 2);
                if (result.name.startsWith("::"))
                    result.name.remove(0, 2);
            }
            parseTemplatePattern(result, nameStart, nameEnd);
        }
    }
    if (result.name == "operator") {
        if (mCursor.currentText() == "(")
            result.name += readBalancedToken("(", ")");
        else if (mCursor.currentText() == "[")
            result.name += readBalancedToken("[", "]");
        else if (!mCursor.atEnd()) {
            appendToken(result.name, mCursor.currentText());
            mCursor.advance();
        }
    }

    while (!mCursor.atEnd()) {
        if (mCursor.currentText() == "[") {
            result.derivations.append(TypeDerivation::array(readBalancedToken("[", "]")));
            continue;
        }
        if (mCursor.currentText() == "(") {
            TypeDerivation function;
            if (!parseFunctionSuffix(function))
                break;
            result.derivations.append(function);
            continue;
        }
        break;
    }

    std::reverse(prefix.begin(), prefix.end());
    result.derivations += prefix;
    if (result.name.isEmpty() && result.qualifier.isEmpty()
            && result.derivations.isEmpty()
            && (mCursor.atEnd() || isDeclarationDelimiter(mCursor.currentText())))
        return true;
    if (result.name.isEmpty() && result.derivations.isEmpty()) {
        mCursor.restore(start);
        return false;
    }
    return true;
}

bool CppDeclarationParser::parseFunctionSuffix(TypeDerivation &function)
{
    CppTokenCursor::Checkpoint suffixStart = mCursor.checkpoint();
    function = TypeDerivation::function();
    mCursor.advance();
    bool valid = true;
    if (!mCursor.consume(")")) {
        while (!mCursor.atEnd()) {
            if (mCursor.consume("...")) {
                function.functionProperties.setFlag(FunctionProperty::Variadic);
            } else {
                ParameterInfo parameter;
                if (!parseParameter(parameter)) {
                    valid = false;
                    break;
                }
                function.parameters.append(parameter);
            }
            if (mCursor.consume("=")) {
                CppTokenCursor::Checkpoint defaultStart = mCursor.checkpoint();
                skipExpression({",", ")"}, nullptr);
                function.parameters.last().defaultArgumentText = readConsumedText(defaultStart);
            }
            if (mCursor.consume(")"))
                break;
            if (!mCursor.consume(",")) {
                valid = false;
                break;
            }
        }
    }
    if (!valid) {
        mCursor.restore(suffixStart);
        return false;
    }

    while (!mCursor.atEnd()) {
        if (mCursor.consume("const"))
            function.qualifiers.setFlag(TypeQualifier::Const);
        else if (mCursor.consume("volatile"))
            function.qualifiers.setFlag(TypeQualifier::Volatile);
        else if (mCursor.consume("&&"))
            function.refQualifier = ReferenceQualifier::Rvalue;
        else if (mCursor.consume("&"))
            function.refQualifier = ReferenceQualifier::Lvalue;
        else if (mCursor.consume("noexcept")) {
            readNoexcept(function.noexceptKind, function.noexceptExpression);
        } else if (mCursor.consume("override")) {
            function.functionProperties.setFlag(FunctionProperty::Override);
        } else if (mCursor.consume("final")) {
        } else {
            break;
        }
    }
    if (mCursor.consume("->"))
        applyTrailingReturn(function);
    if (mCursor.consume("requires")) {
        CppTokenCursor::Checkpoint requiresStart = mCursor.checkpoint();
        skipRequiresClause();
        if (mCppVersion >= CppVersion::Cpp20)
            function.requiresClause = readConsumedText(requiresStart);
    }
    return true;
}

bool CppDeclarationParser::parseParameter(ParameterInfo &result)
{
    DeclarationProfileScope profileScope(declarationProfile.parameterMs,
                                         declarationProfile.parameterSelfNs);
    SourcePosition position = mCursor.current().position;
    DeclSpecifierInfo specifiers;
    if (!parseDeclSpecifierSeq(specifiers))
        return false;
    DeclaratorInfo declarator;
    if (!parseDeclarator(declarator))
        return false;
    result = ParameterInfo();
    result.type = std::make_shared<TypeInfo>(specifiers.type);
    appendTypeDerivation(*result.type, declarator);
    result.name = declarator.name;
    result.parameterPack = declarator.parameterPack;
    result.position = position;
    return true;
}

bool CppDeclarationParser::parseTypeName(TypeInfo &result)
{
    DeclarationProfileScope profileScope(declarationProfile.typeNameMs,
                                         declarationProfile.typeNameSelfNs);
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    result = TypeInfo();
    QString prefix;
    if (mCursor.currentText() == "struct" || mCursor.currentText() == "class"
            || mCursor.currentText() == "union") {
        prefix = mCursor.currentText();
        mCursor.advance();
    } else if (mCursor.consume("enum")) {
        prefix = "enum";
        if (mCursor.currentText() == "class" || mCursor.currentText() == "struct") {
            prefix += " " + mCursor.currentText();
            mCursor.advance();
        }
    } else if (mCursor.consume("typename")) {
        prefix = "typename";
    }

    if (mCursor.consume("decltype")) {
        result.qualifiedName = "decltype";
        if (mCursor.currentText() == "(")
            result.qualifiedName += readBalancedToken("(", ")");
        while (mCursor.consume("::"))
            result.qualifiedName += "::" + parseQualifiedNameText();
        if (!prefix.isEmpty())
            result.qualifiedName = prefix + " " + result.qualifiedName;
        result.qualifiedNameText = readConsumedText(start);
        return true;
    }

    QString name;
    if (mCursor.consume("::"))
        name = "::";
    while (!mCursor.atEnd()) {
        const QString &text = mCursor.currentText();
        if (mCursor.current().kind != CppTokenizer::TokenKind::Identifier
                && !isBuiltinType(text))
            break;
        name += text;
        mCursor.advance();
        if (isClangTypeBuiltin(name) && mCursor.currentText() == "(")
            name += readBalancedToken("(", ")");
        if (mCursor.currentText() == "<") {
            if (readBalancedToken("<", ">", true).isEmpty()) {
                mCursor.restore(start);
                return false;
            }
        }
        if (mCursor.consume("::")) {
            name += "::";
            mCursor.consume("template");
        } else {
            break;
        }
    }
    if (name.isEmpty()) {
        mCursor.restore(start);
        return false;
    }
    result.qualifiedName = name;
    result.qualifiedNameText = readConsumedText(start);
    if (!prefix.isEmpty())
        result.qualifiedName = prefix + " " + result.qualifiedName;
    return !result.qualifiedName.isEmpty();
}

bool CppDeclarationParser::parseTemplateArguments(TypeInfo &result)
{
    DeclarationProfileScope profileScope(declarationProfile.templateArgumentsMs,
                                         declarationProfile.templateArgumentsSelfNs);
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    if (!mCursor.consume("<"))
        return false;

    auto appendArgument = [&result](QString text) {
        text = text.trimmed();
        if (text.isEmpty())
            return;
        CppTokenizer tokenizer;
        tokenizer.tokenize({text});
        CppTokenCursor cursor(tokenizer);
        CppDeclarationParser parser(cursor);
        DeclSpecifierInfo specifiers;
        if (parser.parseDeclSpecifierSeq(specifiers)) {
            if (cursor.currentText() == "(") {
                TypeDerivation function;
                if (parser.parseFunctionSuffix(function) && cursor.atEnd()) {
                    specifiers.type.derivations.append(function);
                    result.templateArguments.append(TemplateArgument::fromType(specifiers.type));
                    return;
                }
            }
            DeclaratorInfo declarator;
            if (parser.parseDeclarator(declarator) && cursor.atEnd()) {
                TypeInfo type = specifiers.type;
                appendTypeDerivation(type, declarator);
                result.templateArguments.append(TemplateArgument::fromType(type));
                return;
            }
        }
        result.templateArguments.append(TemplateArgument::fromConstant(text));
    };

    QString argument;
    int level = 1;
    while (!mCursor.atEnd()) {
        const QString &text = mCursor.currentText();
        if (mCursor.current().matchIndex > mCursor.index()) {
            appendToken(argument, readBalancedToken(
                    text, text == "(" ? ")" : text == "[" ? "]" : "}"));
            continue;
        }
        if (mCursor.current().kind == CppTokenizer::TokenKind::Operator) {
            if (text == "<") {
                ++level;
                appendToken(argument, text);
                mCursor.advance();
                continue;
            }
            if (text == ">" || text == ">>") {
                mCursor.consumeGreater();
                --level;
                if (level == 0) {
                    appendArgument(argument);
                    return true;
                }
                appendToken(argument, ">");
                continue;
            }
        }
        if (text == "," && level == 1) {
            appendArgument(argument);
            argument.clear();
            mCursor.advance();
            continue;
        }
        appendToken(argument, text);
        mCursor.advance();
    }
    mCursor.restore(start);
    return false;
}

QString CppDeclarationParser::parseOperatorName()
{
    QString result = "operator";
    mCursor.advance();
    if (mCursor.atEnd())
        return result;
    if (mCursor.currentText() == "(")
        result += readBalancedToken("(", ")");
    else if (mCursor.currentText() == "[")
        result += readBalancedToken("[", "]");
    else {
        appendToken(result, mCursor.currentText());
        mCursor.advance();
        if ((result == "operator new" || result == "operator delete")
                && mCursor.currentText() == "[") {
            result += readBalancedToken("[", "]");
        }
        if (result == "operator new" || result == "operator delete")
            return result;
        while (!mCursor.atEnd()) {
            if (mCursor.current().kind != CppTokenizer::TokenKind::Identifier
                    && mCursor.currentText() != "::" && mCursor.currentText() != "*"
                    && mCursor.currentText() != "&" && mCursor.currentText() != "&&") {
                break;
            }
            appendToken(result, mCursor.currentText());
            mCursor.advance();
            if (mCursor.currentText() == "<")
                result += readBalancedToken("<", ">", true, true);
        }
    }
    return result;
}

bool CppDeclarationParser::parseTemplatePattern(
        DeclaratorInfo &result, const CppTokenCursor::Checkpoint &nameStart,
        const CppTokenCursor::Checkpoint &nameEnd)
{
    if (!result.name.contains('<') || !result.name.endsWith('>'))
        return false;
    TypeInfo pattern;
    pattern.qualifiedName = result.name.section('<', 0, 0);
    pattern.qualifiedNameText = result.name;
    mCursor.restore(nameStart);
    while (!mCursor.atEnd() && mCursor.currentText() != "<")
        mCursor.advance();
    bool success = mCursor.currentText() == "<" && parseTemplateArguments(pattern);
    mCursor.restore(nameEnd);
    if (success)
        result.templatePattern = pattern;
    return success;
}

QString CppDeclarationParser::parseQualifiedNameText()
{
    QString result;
    if (mCursor.consume("::"))
        result = "::";
    while (!mCursor.atEnd()) {
        if (mCursor.consume("~"))
            result += "~";
        const QString &text = mCursor.currentText();
        if (mCursor.current().kind != CppTokenizer::TokenKind::Identifier
                && !isBuiltinType(text))
            break;
        result += text;
        mCursor.advance();
        if (mCursor.currentText() == "<")
            result += readBalancedToken("<", ">", true, true);
        if (mCursor.consume("::"))
            result += "::";
        else
            break;
    }
    return result;
}

QString CppDeclarationParser::readConsumedText(const CppTokenCursor::Checkpoint &start)
{
    CppTokenCursor::Checkpoint end = mCursor.checkpoint();
    CppTokenizer::Token endToken = mCursor.greaterConsumed() > 0
            ? mCursor.current() : mCursor.peek(-1);
    if (mCursor.greaterConsumed() > 0)
        endToken.text = ">";
    const CppTokenizer::Token &startToken = mCursor.peek(start.index - mCursor.index());
    QString result = readSourceText(startToken, endToken);
    if (!result.isEmpty())
        return result;

    mCursor.restore(start);
    while (mCursor.index() < end.index
            || (mCursor.index() == end.index
                && mCursor.greaterConsumed() < end.greaterConsumed)) {
        QString text = mCursor.currentText();
        if (text == ">" || text == ">>") {
            appendToken(result, ">");
            mCursor.consumeGreater();
        } else {
            appendToken(result, text);
            mCursor.advance();
        }
    }
    mCursor.restore(end);
    return result;
}

QString CppDeclarationParser::readSourceText(const CppTokenizer::Token &startToken,
                                             const CppTokenizer::Token &endToken) const
{
    if (startToken.fileName != endToken.fileName
            || !startToken.position.isValid() || !endToken.position.isValid()
            || endToken.position < startToken.position) {
        return QString();
    }
    const QStringList &lines = mCursor.sourceLines();
    if (startToken.position.line >= lines.count() || endToken.position.line >= lines.count())
        return QString();
    auto matchesSource = [&lines](const CppTokenizer::Token &token) {
        return lines[token.position.line].mid(
                    token.position.ch, token.text.length()) == token.text;
    };
    if (!matchesSource(startToken) || !matchesSource(endToken))
        return QString();
    if (startToken.position.line == endToken.position.line) {
        return lines[startToken.position.line].mid(
                    startToken.position.ch,
                    endToken.position.ch + endToken.text.length() - startToken.position.ch);
    }

    QStringList result{lines[startToken.position.line].mid(startToken.position.ch)};
    for (int line = startToken.position.line + 1; line < endToken.position.line; ++line)
        result.append(lines[line]);
    result.append(lines[endToken.position.line].left(
                endToken.position.ch + endToken.text.length()));
    return result.join(' ');
}

QString CppDeclarationParser::readBalancedToken(const QString &opening, const QString &closing,
                                                bool useSourceText, bool requireNormalized)
{
    if (mCursor.atEnd() || mCursor.currentText() != opening)
        return QString();

    if (opening == "<") {
        DeclarationProfileScope profileScope(declarationProfile.templateTextMs,
                                             declarationProfile.templateTextSelfNs);
#ifdef REDPANDA_PARSER_DECLARATION_PROFILE
        ++declarationProfile.templateTextCalls;
        if (useSourceText)
            ++declarationProfile.sourceTextCalls;
        else
            ++declarationProfile.normalizedTextCalls;
#endif
        CppTokenCursor::Checkpoint start = mCursor.checkpoint();
        QString result;
        int level = 0;
        while (!mCursor.atEnd()) {
#ifdef REDPANDA_PARSER_DECLARATION_PROFILE
            ++declarationProfile.templateTextTokens;
#endif
            const QString &text = mCursor.currentText();
            if (mCursor.current().kind == CppTokenizer::TokenKind::FileBoundary || text == ";")
                break;
            // A non-negative matchIndex before the current index marks a closing
            // token such as ')', ']' or '}'. Opening tokens such as '(' use the branch below.
            if (mCursor.current().matchIndex >= 0
                    && mCursor.current().matchIndex < mCursor.index()) {
                break;
            }
            if (mCursor.current().matchIndex > mCursor.index()) {
                if (useSourceText)
                    skipBalancedToken();
                else
                    result += readBalancedToken(text, text == "(" ? ")" : text == "[" ? "]" : "}");
                continue;
            }
            if (mCursor.current().kind == CppTokenizer::TokenKind::Operator) {
                if (text == "<") {
                    ++level;
                    if (!useSourceText)
                        appendToken(result, text);
                    mCursor.advance();
                    continue;
                }
                if (text == ">" || text == ">>") {
                    const CppTokenizer::Token &endToken = mCursor.current();
                    mCursor.consumeGreater();
                    --level;
                    if (!useSourceText)
                        appendToken(result, closing);
                    if (level == 0) {
                        if (useSourceText) {
                            const CppTokenizer::Token &startToken =
                                    mCursor.peek(start.index - mCursor.index());
                            QString sourceText = readSourceText(startToken, endToken);
                            if (!sourceText.isEmpty()
                                    && (!requireNormalized || isNormalizedSourceText(sourceText))) {
                                return sourceText;
                            }
                            mCursor.restore(start);
                            return readBalancedToken(opening, closing, false);
                        }
                        return result;
                    }
                    continue;
                }
            }
            if (!useSourceText)
                appendToken(result, text);
            mCursor.advance();
        }
        mCursor.restore(start);
        return QString();
    }

    CppTokenCursor::MatchStatus status = mCursor.matchStatus();
    int target = mCursor.current().matchIndex;
    if (useSourceText && status == CppTokenCursor::MatchStatus::SameFile) {
        QString sourceText = readSourceText(mCursor.current(), mCursor.peek(target - mCursor.index()));
        if (!sourceText.isEmpty()
                && (!requireNormalized || isNormalizedSourceText(sourceText))) {
            return sourceText;
        }
    }
    if (status == CppTokenCursor::MatchStatus::CrossFile) {
        QString result;
        while (!mCursor.atEnd()
               && mCursor.current().kind != CppTokenizer::TokenKind::FileBoundary) {
            appendToken(result, mCursor.currentText());
            mCursor.advance();
        }
        return result;
    }

    QString result;
    while (!mCursor.atEnd() && mCursor.index() < target) {
        appendToken(result, mCursor.currentText());
        mCursor.advance();
    }
    if (!mCursor.atEnd()
            && mCursor.current().kind != CppTokenizer::TokenKind::FileBoundary) {
        appendToken(result, mCursor.currentText());
        mCursor.advance();
    }
    return result;
}

QString CppDeclarationParser::readUntilDelimiter(const QSet<QString> &delimiters,
                                                  DeclaratorInfo *declarator)
{
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    skipExpression(delimiters, declarator);
    return readConsumedText(start);
}

void CppDeclarationParser::skipBalancedToken()
{
    mCursor.skipToMatch();
    if (!mCursor.atEnd()
            && mCursor.current().kind != CppTokenizer::TokenKind::FileBoundary) {
        mCursor.advance();
    }
}

namespace {
CppTokenizer::TokenList readLambdaReturnExpression(CppTokenCursor cursor)
{
    int bodyEnd = cursor.current().matchIndex;
    if (bodyEnd <= cursor.index())
        return {};
    cursor.advance();
    while (!cursor.atEnd() && cursor.index() < bodyEnd) {
        if (cursor.currentText() == "return") {
            cursor.advance();
            CppTokenizer::TokenList result;
            while (!cursor.atEnd() && cursor.index() < bodyEnd
                   && cursor.currentText() != ";") {
                int expressionEnd = cursor.current().matchIndex;
                if (expressionEnd > cursor.index()) {
                    while (!cursor.atEnd() && cursor.index() <= expressionEnd) {
                        result.append(cursor.current());
                        cursor.advance();
                    }
                } else {
                    result.append(cursor.current());
                    cursor.advance();
                }
            }
            return result;
        }
        if (cursor.current().matchIndex > cursor.index()) {
            cursor.skipToMatch();
            if (!cursor.atEnd())
                cursor.advance();
        } else {
            cursor.advance();
        }
    }
    return {};
}
}

bool CppDeclarationParser::parseLambda(LambdaInfo &result)
{
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    result = LambdaInfo();
    result.range.start = mCursor.current().position;
    if (mCursor.currentText() != "["
            || mCursor.current().matchIndex <= mCursor.index()) {
        return false;
    }
    skipBalancedToken();

    if (mCursor.currentText() == "(") {
        mCursor.advance();
        if (!mCursor.consume(")")) {
            while (!mCursor.atEnd()) {
                ParameterInfo parameter;
                if (!parseParameter(parameter)) {
                    mCursor.restore(start);
                    return false;
                }
                result.parameters.append(parameter);
                if (mCursor.consume("="))
                    skipExpression({",", ")"}, nullptr);
                if (mCursor.consume(")"))
                    break;
                if (!mCursor.consume(",")) {
                    mCursor.restore(start);
                    return false;
                }
            }
        }
    }

    while (!mCursor.atEnd()) {
        if (mCursor.consume("mutable")) {
            result.isMutable = true;
        } else if (mCursor.consume("constexpr") || mCursor.consume("consteval")) {
        } else if (mCursor.consume("noexcept")) {
            readNoexcept(result.noexceptKind, result.noexceptExpression);
        } else {
            break;
        }
    }
    if (mCursor.consume("->")) {
        DeclSpecifierInfo specifiers;
        if (!parseDeclSpecifierSeq(specifiers)) {
            mCursor.restore(start);
            return false;
        }
        result.returnType = specifiers.type;
        if (mCursor.currentText() != "{") {
            DeclaratorInfo abstractDeclarator;
            if (!parseDeclarator(abstractDeclarator)) {
                mCursor.restore(start);
                return false;
            }
            appendTypeDerivation(result.returnType, abstractDeclarator);
        }
        result.hasTrailingReturnType = true;
    }
    if (mCursor.currentText() != "{") {
        mCursor.restore(start);
        return false;
    }

    CppTokenCursor bodyCursor = mCursor;
    auto body = std::make_shared<DeclarationInfo>();
    body->fileName = mCursor.current().fileName;
    CppSyntaxParser bodyParser(mCursor);
    bodyParser.setCppVersion(mCppVersion);
    if (!bodyParser.parseBlock(*body)) {
        mCursor.restore(start);
        return false;
    }
    result.body = body;
    result.returnExpression = readLambdaReturnExpression(bodyCursor);
    result.range.end = mCursor.peek(-1).position;
    return true;
}

void CppDeclarationParser::skipExpression(const QSet<QString> &delimiters,
                                          DeclaratorInfo *declarator)
{
    while (!mCursor.atEnd() && !delimiters.contains(mCursor.currentText())) {
        if (declarator && mCursor.currentText() == "[") {
            auto lambda = std::make_shared<LambdaInfo>();
            if (parseLambda(*lambda)) {
                declarator->lambdas.append(lambda);
                continue;
            }
        }
        if (mCursor.current().matchIndex > mCursor.index()) {
            skipBalancedToken();
            continue;
        }
        if (mCursor.currentText() == "<"
                && mCursor.peek(-1).kind == CppTokenizer::TokenKind::Identifier) {
            CppTokenCursor::Checkpoint templateStart = mCursor.checkpoint();
            if (skipTemplateArguments())
                continue;
            mCursor.restore(templateStart);
        }
        mCursor.advance();
    }
}

void CppDeclarationParser::skipTemplateRequiresClause()
{
    static const QSet<QString> declarationStarts{
        "class", "struct", "union", "enum", "namespace", "using",
        "static", "extern", "register", "thread_local", "mutable",
        "inline", "virtual", "friend", "constexpr", "consteval",
        "constinit", "explicit", "typedef", "const", "volatile",
        "decltype", "typename", "operator"
    };
    static const QSet<QString> wordOperators{
        "and", "and_eq", "bitand", "bitor", "compl", "not", "not_eq",
        "or", "or_eq", "xor", "xor_eq"
    };
    bool expectOperand = true;
    while (!mCursor.atEnd()) {
        if (mCursor.current().kind == CppTokenizer::TokenKind::FileBoundary)
            return;
        const QString &text = mCursor.currentText();
        if (text == ";" || text == "}")
            return;
        if (expectOperand) {
            if (text == "requires") {
                mCursor.advance();
                if (mCursor.currentText() == "(")
                    skipBalancedToken();
                if (mCursor.currentText() == "{")
                    skipBalancedToken();
                expectOperand = false;
                continue;
            }
            if (mCursor.current().matchIndex > mCursor.index()) {
                skipBalancedToken();
                expectOperand = false;
                continue;
            }
            if (text == "::" || wordOperators.contains(text)
                    || (mCursor.current().kind == CppTokenizer::TokenKind::Operator
                        && (text == "!" || text == "~" || text == "+" || text == "-"
                            || text == "*" || text == "&" || text == "++" || text == "--"))) {
                mCursor.advance();
                continue;
            }
            mCursor.advance();
            expectOperand = false;
            continue;
        }
        if (declarationStarts.contains(text) || isBuiltinType(text)
                || mCursor.current().kind == CppTokenizer::TokenKind::Identifier) {
            if (!wordOperators.contains(text))
                return;
        }
        if (mCursor.current().matchIndex > mCursor.index()) {
            skipBalancedToken();
            continue;
        }
        if (text == "<" && mCursor.peek(-1).kind == CppTokenizer::TokenKind::Identifier) {
            CppTokenCursor::Checkpoint templateStart = mCursor.checkpoint();
            if (skipTemplateArguments())
                continue;
            mCursor.restore(templateStart);
        }
        if (mCursor.current().kind == CppTokenizer::TokenKind::Operator
                || wordOperators.contains(text)) {
            mCursor.advance();
            expectOperand = text != "..." && text != "++" && text != "--";
            continue;
        }
        return;
    }
}

void CppDeclarationParser::skipRequiresClause()
{
    bool requiresExpression = false;
    while (!mCursor.atEnd()) {
        const QString &text = mCursor.currentText();
        if (text == ";" || (text == "{" && !requiresExpression))
            return;
        if (text == "requires") {
            requiresExpression = true;
            mCursor.advance();
            continue;
        }
        if (text == "{") {
            skipBalancedToken();
            requiresExpression = false;
            continue;
        }
        if (mCursor.current().matchIndex > mCursor.index())
            skipBalancedToken();
        else
            mCursor.advance();
    }
}

void CppDeclarationParser::skipTemplateDefaultArgument(bool nonType)
{
    int level = 0;
    while (!mCursor.atEnd()) {
        const QString &text = mCursor.currentText();
        if (mCursor.current().matchIndex > mCursor.index()) {
            skipBalancedToken();
            continue;
        }
        if (mCursor.current().kind == CppTokenizer::TokenKind::Operator) {
            if (text == "<") {
                bool templateOpening = !nonType
                        || (mCursor.peek(-1).kind == CppTokenizer::TokenKind::Identifier
                            && mCursor.peek().kind == CppTokenizer::TokenKind::Identifier);
                if (templateOpening)
                    ++level;
                mCursor.advance();
                continue;
            }
            if (text == ">" || text == ">>") {
                if (level == 0)
                    return;
                mCursor.consumeGreater();
                --level;
                continue;
            }
        }
        if (text == "," && level == 0)
            return;
        mCursor.advance();
    }
}

void CppDeclarationParser::readNoexcept(NoexceptKind &kind, QString &expression)
{
    kind = NoexceptKind::Always;
    expression.clear();
    if (mCursor.currentText() != "(")
        return;
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    skipBalancedToken();
    kind = NoexceptKind::Expression;
    QString text = readConsumedText(start);
    expression = text.mid(1, text.size() - 2).trimmed();
}

void CppDeclarationParser::applyTrailingReturn(TypeDerivation &function)
{
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    TypeInfo trailing;
    if (!parseTypeName(trailing))
        return;
    if (mCursor.currentText() == "{") {
        function.trailingReturnType = trailing.toString();
        return;
    }
    DeclaratorInfo abstractDeclarator;
    if (!parseDeclarator(abstractDeclarator)) {
        mCursor.restore(start);
        return;
    }
    appendTypeDerivation(trailing, abstractDeclarator);
    function.trailingReturnType = trailing.toString();
}

bool CppDeclarationParser::isBuiltinType(const QString &text) const
{
    static const QSet<QString> types{
        "auto", "void", "bool", "char", "wchar_t", "char8_t", "char16_t",
        "char32_t", "short", "int", "long", "signed", "unsigned", "float",
        "double", "_Complex"
    };
    return types.contains(text);
}

bool CppDeclarationParser::isDeclarationDelimiter(const QString &text) const
{
    static const QSet<QString> delimiters{",", ";", ")", "]", "}", ">", ">>", "=", ":"};
    return delimiters.contains(text);
}
