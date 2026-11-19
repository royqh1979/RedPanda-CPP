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
#include "cppexpressionextractor.h"

#include <qsynedit/document.h>
#include <qsynedit/qsynedit.h>
#include <qsynedit/syntaxer/cpp.h>

namespace {
enum class LastSymbolType {
    Identifier,
    ScopeResolutionOperator,
    ObjectMemberOperator,
    PointerMemberOperator,
    PointerToMemberOfObjectOperator,
    PointerToMemberOfPointerOperator,
    MatchingBracket,
    BracketMatched,
    MatchingParenthesis,
    ParenthesisMatched,
    TildeSign,
    AsteriskSign,
    AmpersandSign,
    MatchingAngleQuotation,
    AngleQuotationMatched,
    None
};
}

QStringList CppExpressionExtractor::extract(
        const QSynedit::QSynEdit *editor, const QSynedit::CharPos &pos)
{
    QStringList result;
    int line = pos.line;
    int ch = pos.ch;
    int symbolMatchingLevel = 0;
    LastSymbolType lastSymbolType=LastSymbolType::None;
    QSynedit::CppSyntaxer syntaxer;
    while (true) {
        if (line>=editor->lineCount() || line<0)
            break;
        QStringList tokens;
        editor->startParseLine(&syntaxer, line);
        while (!syntaxer.eol()) {
            int start = syntaxer.getTokenPos();
            QString token = syntaxer.getToken();
            int endPos = start + token.length()-1;
            if (start>ch) {
                break;
            }
            QSynedit::PTokenAttribute attr = syntaxer.getTokenAttribute();
            if ( (line == pos.line)
                 && (start<=ch) && (ch<=endPos)) {
                if (syntaxer.getTokenType() == QSynedit::TokenType::Comment
                        || syntaxer.getTokenType() == QSynedit::TokenType::String) {
                    return result;
                }
            }
            if (syntaxer.getTokenType() != QSynedit::TokenType::Comment
                    && syntaxer.getTokenType() != QSynedit::TokenType::Space){
                tokens.append(token);
            }
            syntaxer.next();
        }
        for (int i=tokens.count()-1;i>=0;i--) {
            QString token = tokens[i];
            if (token=="using")
                return result;
            switch(lastSymbolType) {
            case LastSymbolType::ScopeResolutionOperator: //before '::'
                if (token==">") {
                    lastSymbolType=LastSymbolType::MatchingAngleQuotation;
                    symbolMatchingLevel=0;
                } else if (editor->isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::ObjectMemberOperator: //before '.'
            case LastSymbolType::PointerMemberOperator: //before '->'
            case LastSymbolType::PointerToMemberOfObjectOperator: //before '.*'
            case LastSymbolType::PointerToMemberOfPointerOperator: //before '->*'
                if (token == ")" ) {
                    lastSymbolType=LastSymbolType::MatchingParenthesis;
                    symbolMatchingLevel = 0;
                } else if (token == "]") {
                    lastSymbolType=LastSymbolType::MatchingBracket;
                    symbolMatchingLevel = 0;
                } else if (editor->isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::AsteriskSign: // before '*':
                if (token == '*') {
                } else {
                    QChar firstCh=token.front();
                    if (editor->isIdentChar(firstCh)
                            || firstCh.isDigit()
                            || firstCh == '.'
                            || firstCh == ')' ) {
                        result.pop_front();
                    }
                    return result;
                }
                break;
            case LastSymbolType::AmpersandSign: // before '&':
            {
                QChar firstCh=token.front();
                if (editor->isIdentChar(firstCh)
                        || firstCh.isDigit()
                        || firstCh == '.'
                        || firstCh == ')' ) {
                    result.pop_front();
                }
                return result;
            }
                break;
            case LastSymbolType::ParenthesisMatched: //before '()'
                if (token == ")" ) {
                    lastSymbolType=LastSymbolType::MatchingParenthesis;
                    symbolMatchingLevel = 0;
                } else if (token == "]") {
                    lastSymbolType=LastSymbolType::MatchingBracket;
                    symbolMatchingLevel = 0;
                } else if (token == "*") {
                    lastSymbolType=LastSymbolType::AsteriskSign;
                } else if (token == "&") {
                    lastSymbolType=LastSymbolType::AmpersandSign;
                } else if (editor->isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::BracketMatched: //before '[]'
                if (token == ")" ) {
                    lastSymbolType=LastSymbolType::MatchingParenthesis;
                    symbolMatchingLevel = 0;
                } else if (token == "]") {
                    lastSymbolType=LastSymbolType::MatchingBracket;
                    symbolMatchingLevel = 0;
                } else if (editor->isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::AngleQuotationMatched: //before '<>'
                if (editor->isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::None:
                if (token =="::") {
                    lastSymbolType=LastSymbolType::ScopeResolutionOperator;
                } else if (token == ".") {
                    lastSymbolType=LastSymbolType::ObjectMemberOperator;
                } else if (token=="->") {
                    lastSymbolType=LastSymbolType::PointerMemberOperator;
                } else if (token == ".*") {
                    lastSymbolType=LastSymbolType::PointerToMemberOfObjectOperator;
                } else if (token=="->*"){
                    lastSymbolType=LastSymbolType::PointerToMemberOfPointerOperator;
                } else if (token == ")" ) {
                    lastSymbolType=LastSymbolType::MatchingParenthesis;
                    symbolMatchingLevel = 0;
                } else if (token == "]") {
                    lastSymbolType=LastSymbolType::MatchingBracket;
                    symbolMatchingLevel = 0;
                } else if (editor->isIdentStartChar(token.front())) {
                    lastSymbolType=LastSymbolType::Identifier;
                } else
                    return result;
                break;
            case LastSymbolType::TildeSign:
                if (token =="::") {
                    lastSymbolType=LastSymbolType::ScopeResolutionOperator;
                } else {
                    // "~" must appear after "::"
                    result.pop_front();
                    return result;
                }
                break;;
            case LastSymbolType::Identifier:
                if (token =="::") {
                    lastSymbolType=LastSymbolType::ScopeResolutionOperator;
                } else if (token == ".") {
                    lastSymbolType=LastSymbolType::ObjectMemberOperator;
                } else if (token=="->") {
                    lastSymbolType=LastSymbolType::PointerMemberOperator;
                } else if (token == ".*") {
                    lastSymbolType=LastSymbolType::PointerToMemberOfObjectOperator;
                } else if (token=="->*"){
                    lastSymbolType=LastSymbolType::PointerToMemberOfPointerOperator;
                } else if (token == "~") {
                    lastSymbolType=LastSymbolType::TildeSign;
                } else if (token == "*") {
                    lastSymbolType=LastSymbolType::AsteriskSign;
                } else if (token == "&") {
                    lastSymbolType=LastSymbolType::AmpersandSign;
                } else
                    return result; // stop matching;
                break;
            case LastSymbolType::MatchingParenthesis:
                if (token=="(") {
                    if (symbolMatchingLevel==0) {
                        lastSymbolType=LastSymbolType::ParenthesisMatched;
                    } else {
                        symbolMatchingLevel--;
                    }
                } else if (token==")") {
                    symbolMatchingLevel++;
                }
                break;
            case LastSymbolType::MatchingBracket:
                if (token=="[") {
                    if (symbolMatchingLevel==0) {
                        lastSymbolType=LastSymbolType::BracketMatched;
                    } else {
                        symbolMatchingLevel--;
                    }
                } else if (token=="]") {
                    symbolMatchingLevel++;
                }
                break;
            case LastSymbolType::MatchingAngleQuotation:
                if (token=="<") {
                    if (symbolMatchingLevel==0) {
                        lastSymbolType=LastSymbolType::AngleQuotationMatched;
                    } else {
                        symbolMatchingLevel--;
                    }
                } else if (token==">") {
                    symbolMatchingLevel++;
                }
                break;
            }
            result.push_front(token);
        }

        line--;
        if (line>=0)
            ch = editor->document()->getLine(line).length()+1;
    }
    return result;
}
