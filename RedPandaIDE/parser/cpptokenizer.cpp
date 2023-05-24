/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
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
#include "cpptokenizer.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

CppTokenizer::CppTokenizer()
{

}

void CppTokenizer::clear()
{
    mTokenList.clear();
    mBuffer.clear();
    mBufferStr.clear();
    mLastToken.clear();
    mUnmatchedBraces.clear();
    mUnmatchedBrackets.clear();
    mUnmatchedParenthesis.clear();
    mLambdas.clear();
}

void CppTokenizer::tokenize(const QStringList &buffer)
{
    clear();

    mBuffer = buffer;
    if (mBuffer.isEmpty())
        return;
    mBufferStr = mBuffer[0];
    for (int i=1;i<mBuffer.size();i++) {
        mBufferStr+='\n';
        mBufferStr+=mBuffer[i];
    }
    mStart = mBufferStr.data();
    mCurrent = mStart;
    mLineCount = mStart;
    QString s = "";
    mCurrentLine = 1;

    TokenType tokenType;
    while (true) {
        mLastToken = s;
        s = getNextToken(&tokenType);
        simplify(s);
        if (s.isEmpty())
            break;
        else
            addToken(s,mCurrentLine,tokenType);
    }
    while (!mUnmatchedBraces.isEmpty()) {
        addToken("}",mCurrentLine,TokenType::RightBrace);
    }
    while (!mUnmatchedBrackets.isEmpty()) {
        addToken("]",mCurrentLine,TokenType::RightBracket);
    }
    while (!mUnmatchedParenthesis.isEmpty()) {
        addToken(")",mCurrentLine,TokenType::RightParenthesis);
    }
}

void CppTokenizer::dumpTokens(const QString &fileName)
{
    QFile file(fileName);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream stream(&file);
        foreach (const PToken& token,mTokenList) {
            stream<<QString("%1,%2,%3").arg(token->line).arg(token->text).arg(token->matchIndex)
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
                 <<Qt::endl;
#else
                 <<endl;
#endif
        }
    }
}

void CppTokenizer::addToken(const QString &sText, int iLine, TokenType tokenType)
{
    PToken token = std::make_shared<Token>();
    token->text = sText;
    token->line = iLine;
#ifdef Q_DEBUG
    token->matchIndex = 1000000000;
#endif
    switch(tokenType) {
    case TokenType::LeftBrace:
        token->matchIndex=-1;
        mUnmatchedBraces.push_back(mTokenList.count());
        break;
    case TokenType::RightBrace:
        if (mUnmatchedBraces.isEmpty()) {
            token->matchIndex=-1;
        } else {
            token->matchIndex = mUnmatchedBraces.last();
            mTokenList[token->matchIndex]->matchIndex=mTokenList.count();
            mUnmatchedBraces.pop_back();
        }
        break;
    case TokenType::LeftBracket:
        token->matchIndex=-1;
        mUnmatchedBrackets.push_back(mTokenList.count());
        break;
    case TokenType::RightBracket:
        if (mUnmatchedBrackets.isEmpty()) {
            token->matchIndex=-1;
        } else {
            token->matchIndex = mUnmatchedBrackets.last();
            mTokenList[token->matchIndex]->matchIndex=mTokenList.count();
            mUnmatchedBrackets.pop_back();
        }
        break;
    case TokenType::LeftParenthesis:
        token->matchIndex=-1;
        mUnmatchedParenthesis.push_back(mTokenList.count());
        break;
    case TokenType::RightParenthesis:
        if (mUnmatchedParenthesis.isEmpty()) {
            token->matchIndex=-1;
        } else {
            token->matchIndex = mUnmatchedParenthesis.last();
            mTokenList[token->matchIndex]->matchIndex=mTokenList.count();
            mUnmatchedParenthesis.pop_back();
        }
        break;
    case TokenType::LambdaCaptures:
        mLambdas.push_back(mTokenList.count());
    default:
        break;
    }
    mTokenList.append(token);
}

void CppTokenizer::countLines()
{
    while ((*mLineCount != 0) && (mLineCount < mCurrent)) {
        if (*mLineCount == '\n')
            mCurrentLine ++;
        mLineCount++;
    }
}

QString CppTokenizer::getForInit()
{
    QChar* startOffset = mCurrent;

    // Step into the init statement
    mCurrent++;

    TokenType tokenType;
    // Process until ; or end of file
    while (true) {
        QString s = getNextToken(&tokenType);
        simplify(s);
        if (!s.isEmpty())
            addToken(s,mCurrentLine,tokenType);
        if ( (s == "") || (s == ";") || (s==")") || (s=="("))
            break;
        // : is used in for-each loop
    }

    // Skip to end of for loop
    mCurrent = startOffset;
    skipPair('(', ')');
    return "";
}

QString CppTokenizer::getNextToken(TokenType *pTokenType)
{
    QString result;
    bool done = false;
    *pTokenType=TokenType::Normal;
    while (true) {
        skipToNextToken();
        if (*mCurrent == 0)
            break;
        if (isPreprocessor()) {
            countLines();
            result = getPreprocessor(); // don't count preprocessor lines
            if (result.startsWith("#include")) { // if we find
                int delimPos = result.lastIndexOf(':');
                if (delimPos >= 0) {
                    bool ok;
                    mCurrentLine = result.midRef(delimPos+1).toInt(&ok)-1; // fCurrLine is 0 based
                }
            }
            done = (result != "");
        } else if (isForInit()) {
            countLines();
            result = getForInit();
            done = (result != "");
//        } else if (isArguments()) {
//            countLines();
//            result = getArguments();
//            done = (result != "");
        } else if (isWord()) {
            countLines();
            result = getWord();
//            if (result=="noexcept" || result == "throw") {
//                result="";
//                if (*mCurrent=='(')
//                    skipPair('(',')');
//            }
            done = (result != "");
        } else if (isNumber()) {
            countLines();
            result = getNumber();
            done = (result != "");
        } else {
            switch((*mCurrent).unicode()) {
            case 0:
                done = true;
                break;
            case ':':
                if (*(mCurrent + 1) == ':') {
                    countLines();
                    mCurrent+=2;
                    result = "::";
                    skipToNextToken();
                    // Append next token to this one
//                    if (isIdentChar(*mCurrent))
//                        result+=getWord(true);
                    done = true;
                } else {
                    countLines();
                    result = *mCurrent;
                    mCurrent++;
                    done = true;
                }
                break;
            case '{':
                *pTokenType=TokenType::LeftBrace;
                countLines();
                result = *mCurrent;
                mCurrent++;
                done = true;
                break;
            case '}':
                *pTokenType=TokenType::RightBrace;
                countLines();
                result = *mCurrent;
                mCurrent++;
                done = true;
                break;
            case '(':
                *pTokenType=TokenType::LeftParenthesis;
                countLines();
                result = *mCurrent;
                mCurrent++;
                done = true;
                break;
            case '[':
                if (*(mCurrent+1)!='[') {
                    *pTokenType=TokenType::LambdaCaptures;
                    countLines();
                    QChar* backup=mCurrent;
                    skipPair('[',']');
                    result = QString(backup,mCurrent-backup);
                    done = true;
                } else {
                    skipPair('[',']'); // attribute, skipit
                }
                break;
            case ')':
                *pTokenType=TokenType::RightParenthesis;
                countLines();
                result = *mCurrent;
                mCurrent++;
                done = true;
                break;
            case '.':
            case ';':
            case ',':   //just return the brace or the ';'
                countLines();
                result = *mCurrent;
                mCurrent++;
                done = true;
                break;
            case '>':  // keep stream operators
                if (*(mCurrent + 1) == '>') {
                    countLines();
                    result = ">>";
                    mCurrent+=2;
                    done = true;
                } else {
                    countLines();
                    result = *mCurrent;
                    mCurrent++;
                    done = true;
                } break;
            case '<':
                if (*(mCurrent + 1) == '<') {
                    countLines();
                    result = "<<";
                    mCurrent+=2;
                    done = true;
                } else {
                    countLines();
                    result = *mCurrent;
                    mCurrent++;
                    done = true;
                }
                break;
            case '=': {
                if (*(mCurrent+1)=='=') {
                    // skip '=='
                    countLines();
                    result = "==";
                    mCurrent+=2;
                    done = true;
                } else {
                    countLines();
                    mCurrent++;
                    result = "=";
                    done = true;
                }
                break;
            }
                break;
            case '!':
                if (*(mCurrent+1)=='=') {
                    countLines();
                    result = "!=";
                    mCurrent+=2;
                    done = true;
                } else {
                    countLines();
                    result = *mCurrent;
                    mCurrent++;
                    done = true;
                }
                break;
            case '-':
                if (*(mCurrent + 1) == '=') {
                    countLines();
                    result = "-=";
                    mCurrent+=2;
                    done = true;
                } else if (*(mCurrent + 1) == '>') {
                    countLines();
                    mCurrent+=2;
                    result = "->";
                    done = true;
                } else {
                    countLines();
                    result = *mCurrent;
                    mCurrent++;
                    done = true;
                }
                break;
            case '/':
            case '%':
            case '&':
            case '*':
            case '|':
            case '+':
            case '~':
            case '^':
                if (*(mCurrent + 1) == '=') {
                    countLines();
                    result = *mCurrent;
                    result += "=";
                    mCurrent+=2;
                    done = true;
                } else {
                    countLines();
                    result = *mCurrent;
                    mCurrent++;
                    done = true;
                }
                break;
            default:
                advance();
            }
        }
        if (done)
            break;
    }
    return result;
}

QString CppTokenizer::getNumber()
{
    QChar* offset = mCurrent;

    if (isDigitChar(*mCurrent)) {
        while (isDigitChar(*mCurrent) || isHexChar(*mCurrent)) {
            mCurrent++;
            //advance();
        }
    }

    QString result;
    if (offset != mCurrent) {
        result = QString(offset,mCurrent-offset);
        if (*mCurrent=='.') // keep '.' for decimal
            result += *mCurrent;
    }
    return result;
}

QString CppTokenizer::getPreprocessor()
{
    QChar *offset = mCurrent;
    skipToEOL();
    return QString(offset, mCurrent-offset);
}

QString CppTokenizer::getWord()
{
    bool bFoundTemplate = false;
    //  bIsSmartPointer:=False;

    // Skip spaces
    skipToNextToken();

    // Get next word...
    QChar* offset = mCurrent;

    mCurrent++;
    // Copy the word ahead of us
    while (isIdentChar(*mCurrent) || isDigitChar(*mCurrent))
        mCurrent++;

    QString currentWord;
    if (offset != mCurrent) {
        currentWord = QString(offset,mCurrent-offset);
    }
//    // Append the operator characters and argument list to the operator word
//    if ((currentWord == "operator") ||
//            (currentWord == "&operator") ||
//            (currentWord == "operator*") ||
//            (currentWord == "operator&")) {
//        // Spaces between 'operator' and the operator itself are allowed
//        while (isSpaceChar(*mCurrent))
//            mCurrent++;
//        // Find end of operator
//        while (isOperatorChar(*mCurrent))
//            mCurrent++;
//        currentWord = QString(offset,mCurrent-offset);
//    } else if (currentWord == "template") {
    if (currentWord == "template") {
        bFoundTemplate = true;
    }


    QString result;
    // We found a word...
    if (!currentWord.isEmpty() ) {
        result = currentWord;
        // Skip whitespace
        skipToNextToken();
        if (currentWord!="operator") {
            // Skip template contents, but keep template variable types
            if (*mCurrent == '<') {
                offset = mCurrent;

                if (bFoundTemplate) {
                    skipTemplateArgs();
                } else if (skipAngleBracketPair()){
                    result += QString(offset, mCurrent-offset);
                    skipToNextToken();
                }
            } else if (*mCurrent == '[') {
                // Append array stuff
                while(true) {
                    offset = mCurrent;
                    skipPair('[', ']');
                    result += QString(offset,mCurrent-offset);
                    simplifyArgs(result);
                    skipToNextToken();
                    if (*mCurrent!='[') //maybe multi-dimension array
                        break;
                }
            }

        // Keep parent/child operators
//        if (*mCurrent == '.') {
//            result+=*mCurrent;
//            mCurrent++;
//        } else if ((*mCurrent == '-') && (*(mCurrent + 1) == '>')) {
//            result+=QString(mCurrent,2);
//            mCurrent+=2;
//        } else if ((*mCurrent == ':') && (*(mCurrent + 1) == ':') ) {
//            if (result != "using") {
//                result+=QString(mCurrent,2);
//                mCurrent+=2;
//                skipToNextToken();
//                if (isIdentChar(*mCurrent)) {
//                    // Append next token to this one
//                    QString s = getWord(bSkipParenthesis);
//                    result += s;
//                }
//            }
//        }
        }
    }
    return result;
}

bool CppTokenizer::isArguments()
{
    return *mCurrent == '(';
}

bool CppTokenizer::isForInit()
{
    return (*mCurrent == '(') && (mLastToken == "for");
}

bool CppTokenizer::isNumber()
{
    return isDigitChar(*mCurrent);
}

bool CppTokenizer::isPreprocessor()
{
    return *mCurrent=='#';
}

bool CppTokenizer::isWord()
{
    bool result = isIdentChar(*mCurrent);
    if (result && (*(mCurrent+1) == '"'))
        result = false;
    return result;
}

void CppTokenizer::simplify(QString &output)
{
    //remove \n \r;
    QString temp;
    for (const QChar& ch:output) {
        if (!isLineChar(ch))
            temp+=ch;
    }
    output = temp.trimmed();
}

void CppTokenizer::simplifyArgs(QString &output)
{
    QString temp;
    QString lastSpace = "";
    bool parentheseStart = true;
    foreach (const QChar& ch,output.trimmed()) {
        if (isSpaceChar(ch)) {
            if (!parentheseStart)
                lastSpace+=ch;
        } else if (ch==','){
            temp+=ch;
            lastSpace = "";
            parentheseStart = false;
        } else if (ch=='(') {
            temp+=ch;
            lastSpace = "";
            parentheseStart=true;
        } else if (ch==')') {
            temp+=ch;
            lastSpace = "";
            parentheseStart = false;
        } else {
            parentheseStart=false;
            if (!lastSpace.isEmpty()) {
                temp+=" ";
            }
            lastSpace = "";
            temp+=ch;
        }
    }
    output = temp;
}

//void CppTokenizer::skipAssignment()
//{
//    while (true) {
//        switch ((*mCurrent).unicode()) {
//        case '(': skipPair('(', ')');
//            break;
//        case '"': skipDoubleQuotes();
//            break;
//        case '\'': skipSingleQuote();
//            break;
//        case '{': skipPair('{', '}'); // support struct initializers
//            break;
//        case '/':
//            mCurrent++;
//            break;
//        default:
//          if ((*mCurrent == 'R') && (*(mCurrent+1) == '"'))
//              skipRawString();
//          else
//              mCurrent++;
//        }
//        if (*mCurrent == ','
//                || *mCurrent ==';'
//                || *mCurrent ==')'
//                || *mCurrent =='}'
//                || *mCurrent ==0)
//            break;
//    }
//}

void CppTokenizer::skipDoubleQuotes()
{
    mCurrent++;
    while (!(*mCurrent=='"' || *mCurrent == 0)) {
        if (*mCurrent == '\\')
            mCurrent+=2; // skip escaped char
        else
            mCurrent++;
    }
    if (*mCurrent!=0) {
        mCurrent++;
    }
}

void CppTokenizer::skipPair(const QChar &cStart, const QChar cEnd)
{
    mCurrent++;
    while (*mCurrent != 0) {
        if (*mCurrent == '(') {
            skipPair('(', ')');
        } else if (*mCurrent == '[') {
            skipPair('[', ']');
        } else if (*mCurrent == '{') {
            skipPair('{', '}');
        } else if (*mCurrent ==  cStart) {
            skipPair(cStart, cEnd);
        } else if (*mCurrent == cEnd) {
            mCurrent++; // skip over end
            break;
        } else if ((*mCurrent == 'R') && (*(mCurrent+1) == '"')) {
            if (cStart != '\'' && cStart!='\"')
                skipRawString(); // don't do it inside AnsiString!
            else
                mCurrent++;
        } else if (*mCurrent == '"') {
            if (cStart != '\'' && cStart!='\"')
                skipDoubleQuotes(); // don't do it inside AnsiString!
            else
                mCurrent++;
        } else if (*mCurrent == '\'') {
            if (cStart != '\'' && cStart!='\"')
                skipSingleQuote(); // don't do it inside AnsiString!
            else
                mCurrent++;
        } else {
            mCurrent++;
        }
    }
}

bool CppTokenizer::skipAngleBracketPair()
{
    QChar* backup=mCurrent;
    QVector<QChar> stack;
    while (*mCurrent != '\0') {
        switch((*mCurrent).unicode()) {
        case '<':
        case '(':
        case '[':
            stack.push_back(*mCurrent);
            break;
        case ')':
            while (!stack.isEmpty() && stack.back()!='(') {
                stack.pop_back();
            }
            //pop up '('
            if (stack.isEmpty()) {
                mCurrent=backup;
                return false;
            }
            stack.pop_back();
            break;
        case ']':
            while (!stack.isEmpty() && stack.back()!='[')
                stack.pop_back();
            //pop up '['
            if (stack.isEmpty()) {
                mCurrent=backup;
                return false;
            }
            stack.pop_back();
            break;
        case '>':
            if (stack.back()=='<')
                stack.pop_back();
            if (stack.isEmpty()) {
                mCurrent++;
                return true;
            }
            break;
        case '{':
        case '}':
        case ';':
        case '"':
        case '\'':
            mCurrent=backup;
            return false;
        case '-':
            if (*(mCurrent+1)=='>') {
                mCurrent=backup;
                return false;
            }
            break;
        case '.':
            if (*(mCurrent+1)!='.') {
                mCurrent=backup;
                return false;
            }
            // skip
            while (*(mCurrent+1)=='.')
                mCurrent++;
            break;
        }
        mCurrent++;
    }
    mCurrent=backup;
    return false;
}

void CppTokenizer::skipRawString()
{
    mCurrent++; //skip R
    bool noEscape = false;
    while(true) {
        mCurrent++;
        switch(mCurrent->unicode()) {
        case '(':
            noEscape = true;
            break;
        case ')':
            noEscape = false;
            break;
        }
        if (*mCurrent == 0)
            break;
        if ((*mCurrent == '"') && !noEscape)
            break;
    }
    if (*mCurrent!=0)
        mCurrent++;
}

void CppTokenizer::skipSingleQuote()
{
    mCurrent++;
    while (!(*mCurrent=='\'' || *mCurrent == 0)) {
        if (*mCurrent == '\\')
            mCurrent+=2; // skip escaped char
        else
            mCurrent++;
    }
    if (*mCurrent!=0) {
        mCurrent++;
    }
}

void CppTokenizer::skipSplitLine()
{
    mCurrent++; // skip '\'
    while ( isLineChar(*mCurrent)) // skip newline
        mCurrent++;
}

void CppTokenizer::skipTemplateArgs()
{
    if (*mCurrent != '<')
        return;

    if (skipAngleBracketPair())
        return;
    QChar* lastBracketPos = mCurrent;
    bool shouldExit=false;
    while (true) {
        switch(mCurrent->unicode()) {
        case '\0':
        case ';':
        case '}':
        case '{':
            shouldExit=true;
            break;
        case '>':
            lastBracketPos = mCurrent;
            break;
        }
        if (shouldExit)
            break;
        mCurrent++;
    }
    if (*lastBracketPos=='>')
        mCurrent = lastBracketPos+1; //skip '>';
}

void CppTokenizer::skipToEOL()
{
    while (true) {
        while (!isLineChar(*mCurrent) && (*mCurrent!=0)) {
            mCurrent++;
        }
        if (*mCurrent==0)
            return;

        bool splitLine = (*(mCurrent - 1) == '\\');

        while (isLineChar(*mCurrent))
            mCurrent++;

        if (!splitLine || *mCurrent==0)
            break;
    }
}

void CppTokenizer::skipToNextToken()
{
    while (isSpaceChar(*mCurrent) || isLineChar(*mCurrent))
        mCurrent++;
}

void CppTokenizer::advance()
{
    switch(mCurrent->unicode()) {
    case '\"':
        skipDoubleQuotes();
        break;
    case '\'':
        skipSingleQuote();
        break;
    case '\\':
        if (isLineChar(*(mCurrent + 1)))
            skipSplitLine();
        else
            mCurrent++;
        break;
    case 'R':
        if (*(mCurrent+1) == '"')
            skipRawString();
        else
            mCurrent++;
        break;
    default:
        mCurrent++;
    }
}
