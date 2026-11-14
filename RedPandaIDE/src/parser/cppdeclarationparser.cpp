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

namespace {
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
    };
};

DeclarationProfile declarationProfile;

struct DeclarationProfileScope {
    explicit DeclarationProfileScope(qint64 &target) : target(target) {
        if (declarationProfile.enabled)
            timer.start();
    }
    ~DeclarationProfileScope() {
        if (declarationProfile.enabled)
            target += timer.nsecsElapsed();
    }
    qint64 &target;
    QElapsedTimer timer;
};

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
    DeclarationProfileScope profileScope(declarationProfile.simpleMs);
    ++declarationProfile.simpleCalls;
    qint64 templateTextTokensAtStart = declarationProfile.templateTextTokens;
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    if (isNonDeclarationStartKeyword(mCursor.currentText())) {
        ++declarationProfile.simpleFailures;
        return false;
    }
    bool withoutSpecifiers = mCursor.currentText() == "~"
            || mCursor.currentText() == "operator"
            || (mCursor.current().kind == CppTokenizer::TokenKind::Identifier
                && !isBuiltinType(mCursor.currentText())
                && mCursor.peek().text == "(");
    if (!withoutSpecifiers && !parseDeclSpecifierSeq(result.specifiers)) {
        ++declarationProfile.simpleFailures;
        declarationProfile.failedTemplateTextTokens +=
                declarationProfile.templateTextTokens - templateTextTokensAtStart;
        mCursor.restore(start);
        return false;
    }
    if (mCursor.currentText() == "[")
        return parseStructuredBinding(result);

    while (!mCursor.atEnd()) {
        DeclaratorInfo declarator;
        if (!parseDeclarator(declarator)) {
            ++declarationProfile.simpleFailures;
            declarationProfile.failedTemplateTextTokens +=
                    declarationProfile.templateTextTokens - templateTextTokensAtStart;
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
                if (result.specifiers.type.qualifiedName == "auto") {
                    result.declarators.last().initializerPosition = mCursor.current().position;
                    result.declarators.last().initializerText = readUntilDelimiter({",", ";"});
                } else {
                    skipExpression({",", ";"});
                }
            }
        }
        if (!mCursor.atEnd() && mCursor.currentText() == "{") {
            bool function = !declarator.derivations.isEmpty()
                    && declarator.derivations.first().kind == TypeDerivationKind::Function;
            if (!function)
                skipBalancedToken();
        }
        if (!mCursor.atEnd() && mCursor.currentText() == "(")
            skipBalancedToken();
        if (mCursor.consume(":")) {
            bool function = !declarator.derivations.isEmpty()
                    && declarator.derivations.first().kind == TypeDerivationKind::Function;
            skipExpression(function ? QSet<QString>{"{"} : QSet<QString>{",", ";"});
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
        QString initializer = readUntilDelimiter({";"});
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
    if (!mCursor.consume("template"))
        return true;
    if (!mCursor.consume("<")) {
        mCursor.restore(start);
        return false;
    }
    result.templateKind = TemplateDeclarationKind::Primary;
    if (mCursor.consume(">")) {
        result.templateKind = TemplateDeclarationKind::ExplicitSpecialization;
        return true;
    }

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
            if (parameter.kind == TemplateParameterKind::NonType)
                skipExpression({",", ">", ">>"});
            else
                skipTemplateDefaultArgument();
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
    return true;
}

void CppDeclarationParser::setCurrentClassName(const QString &className)
{
    mClassName = className.section("::", -1);
    int templateStart = mClassName.indexOf('<');
    if (templateStart >= 0)
        mClassName.truncate(templateStart);
}

bool CppDeclarationParser::isConstructorDeclaration()
{
    DeclarationProfileScope profileScope(declarationProfile.constructorCheckMs);
    CppTokenCursor::Checkpoint checkpoint = mCursor.checkpoint();
    QStringList parts;
    mCursor.consume("::");
    while (mCursor.current().kind == CppTokenizer::TokenKind::Identifier) {
        parts.append(mCursor.currentText());
        mCursor.advance();
        if (mCursor.currentText() == "<") {
            mCursor.skipToMatch();
            if (mCursor.atEnd()) {
                mCursor.restore(checkpoint);
                return false;
            }
            mCursor.advance();
        }
        if (!mCursor.consume("::"))
            break;
    }
    if (mCursor.currentText() != "(") {
        mCursor.restore(checkpoint);
        return false;
    }
    mCursor.restore(checkpoint);
    if (parts.isEmpty())
        return false;
    if (!mClassName.isEmpty())
        return parts.last() == mClassName;
    return parts.count() >= 2 && parts.last() == parts[parts.count() - 2];
}

bool CppDeclarationParser::parseDeclSpecifierSeq(DeclSpecifierInfo &result)
{
    DeclarationProfileScope profileScope(declarationProfile.specifiersMs);
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    result = DeclSpecifierInfo();
    bool hasType = false;
    while (!mCursor.atEnd()) {
        if (isConstructorDeclaration())
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
                || mCursor.current().kind == CppTokenizer::TokenKind::Identifier;
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
    if (!hasType && !isConstructorDeclaration()) {
        mCursor.restore(start);
        return false;
    }
    return true;
}

bool CppDeclarationParser::parseDeclarator(DeclaratorInfo &result)
{
    DeclarationProfileScope profileScope(declarationProfile.declaratorMs);
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
            QString qualified = parseQualifiedNameText();
            int last = qualified.lastIndexOf("::");
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
        }
    }

    while (!mCursor.atEnd()) {
        if (mCursor.currentText() == "[") {
            result.derivations.append(TypeDerivation::array(readBalancedToken("[", "]")));
            continue;
        }
        if (mCursor.currentText() == "(") {
            CppTokenCursor::Checkpoint suffixStart = mCursor.checkpoint();
            TypeDerivation function = TypeDerivation::function();
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
                    if (mCursor.consume("="))
                        skipExpression({",", ")"});
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
                break;
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
                    if (mCursor.currentText() == "(")
                        skipBalancedToken();
                } else if (mCursor.consume("override")) {
                    function.functionProperties.setFlag(FunctionProperty::Override);
                } else if (mCursor.consume("final")) {
                } else {
                    break;
                }
            }
            if (mCursor.consume("->"))
                applyTrailingReturn(function);
            if (mCursor.consume("requires"))
                skipRequiresClause();
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

bool CppDeclarationParser::parseParameter(ParameterInfo &result)
{
    DeclarationProfileScope profileScope(declarationProfile.parameterMs);
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
    result.position = position;
    return true;
}

bool CppDeclarationParser::parseTypeName(TypeInfo &result)
{
    DeclarationProfileScope profileScope(declarationProfile.typeNameMs);
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
    DeclarationProfileScope profileScope(declarationProfile.templateArgumentsMs);
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
            argument += readBalancedToken(text, text == "(" ? ")" : text == "[" ? "]" : "}");
            continue;
        }
        if (mCursor.current().kind == CppTokenizer::TokenKind::Operator) {
            if (text == "<") {
                ++level;
                argument += text;
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
                argument += ">";
                continue;
            }
        }
        if (text == "," && level == 1) {
            appendArgument(argument);
            argument.clear();
            mCursor.advance();
            continue;
        }
        argument += text;
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
    }
    return result;
}

QString CppDeclarationParser::parseQualifiedNameText()
{
    QString result;
    if (mCursor.consume("::"))
        result = "::";
    while (!mCursor.atEnd()) {
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
        appendToken(result, text);
        if (text == ">" || text == ">>")
            mCursor.consumeGreater();
        else
            mCursor.advance();
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
        DeclarationProfileScope profileScope(declarationProfile.templateTextMs);
        ++declarationProfile.templateTextCalls;
        if (useSourceText)
            ++declarationProfile.sourceTextCalls;
        else
            ++declarationProfile.normalizedTextCalls;
        CppTokenCursor::Checkpoint start = mCursor.checkpoint();
        QString result;
        int level = 0;
        while (!mCursor.atEnd()) {
            ++declarationProfile.templateTextTokens;
            const QString &text = mCursor.currentText();
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

QString CppDeclarationParser::readUntilDelimiter(const QSet<QString> &delimiters)
{
    QString result;
    while (!mCursor.atEnd() && !delimiters.contains(mCursor.currentText())) {
        const QString &text = mCursor.currentText();
        if (text == "(")
            result += readBalancedToken("(", ")");
        else if (text == "[")
            result += readBalancedToken("[", "]");
        else if (text == "{")
            result += readBalancedToken("{", "}");
        else {
            appendToken(result, text);
            mCursor.advance();
        }
    }
    return result;
}

void CppDeclarationParser::skipBalancedToken()
{
    mCursor.skipToMatch();
    if (!mCursor.atEnd()
            && mCursor.current().kind != CppTokenizer::TokenKind::FileBoundary) {
        mCursor.advance();
    }
}

void CppDeclarationParser::skipExpression(const QSet<QString> &delimiters)
{
    while (!mCursor.atEnd() && !delimiters.contains(mCursor.currentText())) {
        if (mCursor.current().matchIndex > mCursor.index())
            skipBalancedToken();
        else
            mCursor.advance();
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

void CppDeclarationParser::skipTemplateDefaultArgument()
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

void CppDeclarationParser::applyTrailingReturn(TypeDerivation &function)
{
    CppTokenCursor::Checkpoint start = mCursor.checkpoint();
    TypeInfo trailing;
    if (!parseTypeName(trailing))
        return;
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
        "double"
    };
    return types.contains(text);
}

bool CppDeclarationParser::isDeclarationDelimiter(const QString &text) const
{
    static const QSet<QString> delimiters{",", ";", ")", "]", "}", "=", ":"};
    return delimiters.contains(text);
}
