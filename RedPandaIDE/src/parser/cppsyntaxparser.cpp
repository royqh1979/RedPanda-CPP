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
#include "cppsyntaxparser.h"

#include <QStringList>

namespace {
QString readBalanced(CppTokenCursor &cursor)
{
    QString result;
    int target = cursor.current().matchIndex;
    while (!cursor.atEnd() && cursor.index() < target) {
        result += cursor.currentText();
        cursor.advance();
    }
    if (!cursor.atEnd()
            && cursor.current().kind != CppTokenizer::TokenKind::FileBoundary) {
        result += cursor.currentText();
        cursor.advance();
    }
    return result;
}

void appendToken(QString &target, const QString &text)
{
    if (!target.isEmpty() && !text.isEmpty()
            && (target.back().isLetterOrNumber() || target.back() == '_')
            && (text.front().isLetterOrNumber() || text.front() == '_'))
        target += ' ';
    target += text;
}
}

CppSyntaxParser::CppSyntaxParser(CppTokenCursor &cursor)
    : mCursor(cursor), mDeclarationParser(cursor)
{
}

bool CppSyntaxParser::parseDeclaration(DeclarationInfo &result)
{
    SourcePosition start = mCursor.current().position;
    result = DeclarationInfo();
    bool success = false;
    if (!mDeclarationParser.parseTemplatePrefix(result))
        return false;
    if (mCursor.currentText() == "inline"
            && mCursor.peek().text == "namespace") {
        result.specifiers.properties.setFlag(DeclSpecifierProperty::Inline);
        mCursor.advance();
    }
    if (mCursor.currentText() == "namespace")
        success = parseNamespace(result);
    else if (mCursor.currentText() == "class" || mCursor.currentText() == "struct"
            || mCursor.currentText() == "union")
        success = parseClass(result);
    else if (mCursor.currentText() == "enum")
        success = parseEnum(result);
    else if (mCursor.currentText() == "using")
        success = parseUsing(result);
    else
        success = mDeclarationParser.parseSimpleDeclaration(result);
    if (success)
        finishRanges(result, start);
    return success;
}

bool CppSyntaxParser::parseClass(DeclarationInfo &result)
{
    QString keyword = mCursor.currentText();
    mCursor.advance();
    result.kind = DeclarationKind::Class;
    if (keyword == "class")
        result.classKey = ClassKey::Class;
    else if (keyword == "struct")
        result.classKey = ClassKey::Struct;
    else
        result.classKey = ClassKey::Union;

    DeclaratorInfo declarator;
    declarator.name = mDeclarationParser.parseQualifiedNameText();
    declarator.position = mCursor.current().position;
    result.declarators.append(declarator);
    mCursor.consume("final");
    if (mCursor.consume(":")) {
        QString bases = readUntil({"{" , ";"});
        result.baseClasses = splitTopLevel(bases);
    }
    if (mCursor.consume(";"))
        return true;
    SourcePosition bodyStart = mCursor.current().position;
    if (!mCursor.consume("{"))
        return false;

    DeclarationAccessibility accessibility = result.classKey == ClassKey::Class
            ? DeclarationAccessibility::Private : DeclarationAccessibility::Public;
    while (!mCursor.atEnd() && mCursor.currentText() != "}") {
        if (mCursor.consume(";"))
            continue;
        DeclarationAccessibility declaredAccessibility = DeclarationAccessibility::Default;
        if (mCursor.currentText() == "public" || mCursor.currentText() == "protected"
                || mCursor.currentText() == "private") {
            QString text = mCursor.currentText();
            if (mCursor.peek().text == ":") {
                if (text == "public")
                    accessibility = DeclarationAccessibility::Public;
                else if (text == "protected")
                    accessibility = DeclarationAccessibility::Protected;
                else
                    accessibility = DeclarationAccessibility::Private;
                mCursor.advance();
                mCursor.advance();
                continue;
            }
            declaredAccessibility = accessibility;
        }
        DeclarationInfo child;
        if (!parseDeclaration(child)) {
            skipDeclaration();
            continue;
        }
        if (child.accessibility == DeclarationAccessibility::Default)
            child.accessibility = declaredAccessibility == DeclarationAccessibility::Default
                    ? accessibility : declaredAccessibility;
        result.children.append(child);
        if (mCursor.currentText() == "{")
            skipBalanced();
        mCursor.consume(";");
    }
    SourcePosition bodyEnd = mCursor.currentText() == "}"
            ? mCursor.current().position : mCursor.peek(-1).position;
    result.bodyRange = SourceRange{bodyStart, bodyEnd};
    mCursor.consume("}");
    while (!mCursor.atEnd() && mCursor.currentText() != ";"
            && mCursor.currentText() != "}") {
        DeclaratorInfo trailing;
        if (!mDeclarationParser.parseDeclarator(trailing))
            return false;
        result.declarators.append(trailing);
        if (!mCursor.consume(","))
            break;
    }
    mCursor.consume(";");
    return true;
}

bool CppSyntaxParser::parseNamespace(DeclarationInfo &result)
{
    result.kind = DeclarationKind::Namespace;
    mCursor.advance();
    DeclaratorInfo declarator;
    declarator.name = mDeclarationParser.parseQualifiedNameText();
    declarator.position = mCursor.current().position;
    result.declarators.append(declarator);

    if (mCursor.consume("=")) {
        result.kind = DeclarationKind::NamespaceAlias;
        result.specifiers.type.qualifiedName = mDeclarationParser.parseQualifiedNameText();
        mCursor.consume(";");
        return true;
    }
    SourcePosition bodyStart = mCursor.current().position;
    if (!mCursor.consume("{"))
        return false;
    while (!mCursor.atEnd() && mCursor.currentText() != "}") {
        if (mCursor.consume(";"))
            continue;
        DeclarationInfo child;
        if (!parseDeclaration(child)) {
            skipDeclaration();
            continue;
        }
        result.children.append(child);
        if (mCursor.currentText() == "{")
            skipBalanced();
        mCursor.consume(";");
    }
    SourcePosition bodyEnd = mCursor.currentText() == "}"
            ? mCursor.current().position : mCursor.peek(-1).position;
    result.bodyRange = SourceRange{bodyStart, bodyEnd};
    mCursor.consume("}");
    mCursor.consume(";");
    return true;
}

bool CppSyntaxParser::parseEnum(DeclarationInfo &result)
{
    result.kind = DeclarationKind::Enum;
    mCursor.advance();
    if (mCursor.currentText() == "class" || mCursor.currentText() == "struct") {
        result.enumScoped = true;
        mCursor.advance();
    }
    DeclaratorInfo declarator;
    declarator.name = mDeclarationParser.parseQualifiedNameText();
    declarator.position = mCursor.current().position;
    result.declarators.append(declarator);
    if (mCursor.consume(":"))
        mDeclarationParser.parseDeclSpecifierSeq(result.specifiers);
    if (mCursor.consume(";"))
        return true;
    SourcePosition bodyStart = mCursor.current().position;
    if (!mCursor.consume("{"))
        return false;

    while (!mCursor.atEnd() && mCursor.currentText() != "}") {
        EnumeratorInfo enumerator;
        enumerator.name = mCursor.currentText();
        enumerator.range.start = mCursor.current().position;
        mCursor.advance();
        if (mCursor.consume("="))
            enumerator.value = readUntil({",", "}"});
        enumerator.range.end = mCursor.peek(-1).position;
        result.enumerators.append(enumerator);
        if (mCursor.consume(","))
            continue;
        if (mCursor.currentText() != "}")
            skipDeclaration();
    }
    SourcePosition bodyEnd = mCursor.currentText() == "}"
            ? mCursor.current().position : mCursor.peek(-1).position;
    result.bodyRange = SourceRange{bodyStart, bodyEnd};
    mCursor.consume("}");
    mCursor.consume(";");
    return true;
}

bool CppSyntaxParser::parseUsing(DeclarationInfo &result)
{
    mCursor.advance();
    if (mCursor.consume("namespace")) {
        result.kind = DeclarationKind::Using;
        result.usingKind = UsingDeclarationKind::Namespace;
        DeclaratorInfo declarator;
        declarator.name = mDeclarationParser.parseQualifiedNameText();
        result.declarators.append(declarator);
        mCursor.consume(";");
        return true;
    }
    if (mCursor.consume("enum")) {
        result.kind = DeclarationKind::Using;
        result.usingKind = UsingDeclarationKind::Enum;
        DeclaratorInfo declarator;
        declarator.name = mDeclarationParser.parseQualifiedNameText();
        result.declarators.append(declarator);
        mCursor.consume(";");
        return true;
    }

    DeclaratorInfo declarator;
    declarator.name = mDeclarationParser.parseQualifiedNameText();
    declarator.position = mCursor.current().position;
    result.declarators.append(declarator);
    if (!mCursor.consume("=")) {
        result.kind = DeclarationKind::Using;
        mCursor.consume(";");
        return true;
    }

    result.kind = DeclarationKind::Alias;
    if (!mDeclarationParser.parseDeclSpecifierSeq(result.specifiers))
        return false;
    DeclaratorInfo targetDeclarator;
    if (!mDeclarationParser.parseDeclarator(targetDeclarator))
        return false;
    result.specifiers.type.derivations += targetDeclarator.derivations;
    mCursor.consume(";");
    return true;
}

void CppSyntaxParser::finishRanges(DeclarationInfo &result, const SourcePosition &start)
{
    result.range.start = start;
    if (mCursor.currentText() == "{") {
        parseBlock(result);
        mCursor.consume(";");
    }
    result.range.end = mCursor.peek(-1).position;
}

QString CppSyntaxParser::readUntil(const QSet<QString> &delimiters)
{
    QString result;
    while (!mCursor.atEnd() && !delimiters.contains(mCursor.currentText())) {
        QString text = mCursor.currentText();
        if (text == "(" || text == "[" || text == "{")
            result += readBalanced(mCursor);
        else {
            appendToken(result, text);
            mCursor.advance();
        }
    }
    return result;
}

QStringList CppSyntaxParser::splitTopLevel(const QString &text)
{
    CppTokenizer tokenizer;
    tokenizer.tokenize({text});
    QStringList result;
    QString current;
    int angleLevel = 0;
    const auto &tokens = tokenizer.tokens();
    for (int i = 0; i < tokens.count(); ++i) {
        const QString &token = tokens[i].text;
        if (token == "," && angleLevel == 0) {
            result.append(current.trimmed());
            current.clear();
            continue;
        }
        if (token == "<")
            ++angleLevel;
        else if (token == ">")
            --angleLevel;
        else if (token == ">>")
            angleLevel = qMax(0, angleLevel - 2);
        appendToken(current, token);
    }
    if (!current.trimmed().isEmpty())
        result.append(current.trimmed());
    return result;
}

void CppSyntaxParser::skipBalanced()
{
    mCursor.skipToMatch();
    if (!mCursor.atEnd()
            && mCursor.current().kind != CppTokenizer::TokenKind::FileBoundary)
        mCursor.advance();
}

void CppSyntaxParser::skipDeclaration()
{
    while (!mCursor.atEnd()) {
        QString text = mCursor.currentText();
        if (text == ";" || text == "}")
            return;
        if (text == "(" || text == "[" || text == "{")
            skipBalanced();
        else
            mCursor.advance();
    }
}


bool CppSyntaxParser::parseBlock(DeclarationInfo &result)
{
    if (mCursor.currentText() != "{")
        return false;
    if (!result.range.start.isValid())
        result.range.start = mCursor.current().position;
    result.bodyRange.start = mCursor.current().position;
    mCursor.advance();
    while (!mCursor.atEnd() && mCursor.currentText() != "}") {
        if (mCursor.consume(";"))
            continue;
        DeclarationInfo child;
        if (mCursor.currentText() == "for") {
            if (!parseFor(child))
                skipDeclaration();
            else
                result.children.append(child);
            continue;
        }
        if (mCursor.currentText() == "try") {
            mCursor.advance();
            child.kind = DeclarationKind::Block;
            if (mCursor.currentText() == "{")
                parseBlock(child);
            else
                skipDeclaration();
            result.children.append(child);
            continue;
        }
        if (mCursor.currentText() == "catch") {
            if (!parseCatch(child))
                skipDeclaration();
            else
                result.children.append(child);
            continue;
        }
        if (mCursor.currentText() == "while" || mCursor.currentText() == "if") {
            if (!parseControlBlock(child))
                skipDeclaration();
            else
                result.children.append(child);
            continue;
        }
        if (mCursor.currentText() == "{") {
            child.kind = DeclarationKind::Block;
            if (!parseBlock(child))
                skipBalanced();
            else
                result.children.append(child);
            continue;
        }
        CppTokenCursor::Checkpoint checkpoint = mCursor.checkpoint();
        if (parseDeclaration(child)) {
            result.children.append(child);
            continue;
        }
        mCursor.restore(checkpoint);
        skipDeclaration();
        mCursor.consume(";");
    }
    result.bodyRange.end = mCursor.currentText() == "}"
            ? mCursor.current().position : mCursor.peek(-1).position;
    mCursor.consume("}");
    result.range.end = mCursor.peek(-1).position;
    return true;
}

bool CppSyntaxParser::parseFor(DeclarationInfo &result)
{
    result.kind = DeclarationKind::Block;
    result.range.start = mCursor.current().position;
    mCursor.advance();
    if (!mCursor.consume("("))
        return false;

    CppTokenCursor::Checkpoint declarationStart = mCursor.checkpoint();
    DeclaratorInfo loopVariable;
    if (mDeclarationParser.parseDeclSpecifierSeq(result.specifiers)
            && mDeclarationParser.parseDeclarator(loopVariable)
            && mCursor.consume(":")) {
        result.declarators.append(loopVariable);
        result.rangeExpressionPosition = mCursor.current().position;
        result.rangeExpression = readUntil({")"});
        mCursor.consume(")");
        return parseStatementBody(result);
    }

    mCursor.restore(declarationStart);
    DeclarationInfo initializer;
    CppTokenCursor::Checkpoint initializerStart = mCursor.checkpoint();
    if (parseDeclaration(initializer)) {
        result.children.append(initializer);
    } else {
        mCursor.restore(initializerStart);
        readUntil({";"});
        mCursor.consume(";");
    }
    readUntil({";"});
    mCursor.consume(";");
    readUntil({")"});
    mCursor.consume(")");
    return parseStatementBody(result);
}

bool CppSyntaxParser::parseCatch(DeclarationInfo &result)
{
    result.kind = DeclarationKind::Block;
    result.range.start = mCursor.current().position;
    mCursor.advance();
    if (!mCursor.consume("("))
        return false;
    if (mCursor.currentText() == "...") {
        mCursor.advance();
    } else {
        DeclarationInfo exception;
        CppTokenCursor::Checkpoint exceptionStart = mCursor.checkpoint();
        if (parseDeclaration(exception))
            result.children.append(exception);
        else
            mCursor.restore(exceptionStart);
    }
    if (mCursor.currentText() != ")")
        readUntil({")"});
    mCursor.consume(")");
    return parseStatementBody(result);
}

bool CppSyntaxParser::parseControlBlock(DeclarationInfo &result)
{
    result.kind = DeclarationKind::Block;
    result.range.start = mCursor.current().position;
    mCursor.advance();
    if (mCursor.currentText() == "(")
        skipBalanced();
    return parseStatementBody(result);
}

bool CppSyntaxParser::parseStatementBody(DeclarationInfo &result)
{
    if (mCursor.currentText() == "{")
        return parseBlock(result);
    if (mCursor.atEnd() || mCursor.currentText() == "}")
        return false;
    result.bodyRange.start = mCursor.current().position;
    skipDeclaration();
    result.bodyRange.end = mCursor.peek(-1).position;
    mCursor.consume(";");
    result.range.end = mCursor.peek(-1).position;
    return true;
}
