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

CppTokenizer::CppTokenizer()
{

}

void CppTokenizer::reset()
{
    mTokenList.clear();
    mBuffer.clear();
    mBufferStr.clear();
}

void CppTokenizer::tokenize(const QStringList &buffer)
{
    reset();

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
    bool bSkipBlocks = false;
    mCurrentLine = 1;
    while (true) {
        mLastToken = s;
        s = getNextToken(true, true, bSkipBlocks);
        simplify(s);
        if (s.isEmpty())
            break;
        else
            addToken(s,mCurrentLine);
    }
}

void CppTokenizer::dumpTokens(const QString &fileName)
{
    QFile file(fileName);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream stream(&file);
        foreach (const PToken& token,mTokenList) {
            stream<<QString("%1,%2").arg(token->line).arg(token->text)
#if QT_VERSION_CHECK(5,15,0)
                 <<Qt::endl;
#else
                 <<endl;
#endif
        }
    }
}

const CppTokenizer::TokenList &CppTokenizer::tokens()
{
    return mTokenList;
}

CppTokenizer::PToken CppTokenizer::operator[](int i)
{
    return mTokenList[i];
}

int CppTokenizer::tokenCount()
{
    return mTokenList.count();
}

void CppTokenizer::addToken(const QString &sText, int iLine)
{
    PToken token = std::make_shared<Token>();
    token->text = sText;
    token->line = iLine;
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

QString CppTokenizer::getArguments()
{
    QChar* offset = mCurrent;
    skipPair('(', ')');
    QString result(offset,mCurrent-offset);
    simplifyArgs(result);
    if ((*mCurrent == '.') || ((*mCurrent == '-') && (*(mCurrent + 1) == '>'))) {
        // skip '.' and '->'
        while ( !( *mCurrent == 0
                   || *mCurrent == '('
                   || *mCurrent == ';'
                   || *mCurrent == '{'
                   || *mCurrent == '}'
                   || *mCurrent == ')'
                 || isLineChar(*mCurrent)
                 || isSpaceChar(*mCurrent)) )
            mCurrent++;
    }
    skipToNextToken();
    return result;
}

QString CppTokenizer::getForInit()
{
    QChar* startOffset = mCurrent;

    // Step into the init statement
    mCurrent++;

    // Process until ; or end of file
    while (true) {
        QString s = getNextToken(true, true, false);
        simplify(s);
        if (!s.isEmpty())
            addToken(s,mCurrentLine);
        if ( (s == "") || (s == ";") || (s==":"))
            break;
        // : is used in for-each loop
    }

    // Skip to end of for loop
    mCurrent = startOffset;
    skipPair('(', ')');
    return "";
}

QString CppTokenizer::getNextToken(bool /* bSkipParenthesis */, bool bSkipArray, bool bSkipBlock)
{
    QString result;
    bool done = false;
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
        } else if (isArguments()) {
            countLines();
            result = getArguments();
            done = (result != "");
        } else if (isWord()) {
            countLines();
            result = getWord(false, bSkipArray, bSkipBlock);
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
            case '/':
                advance();
                break;
            case ':':
                if (*(mCurrent + 1) == ':') {
                    countLines();
                    mCurrent+=2;
                    // Append next token to this one
                    result = "::"+getWord(true, bSkipArray, bSkipBlock);
                    done = true;
                } else {
                    countLines();
                    result = *mCurrent;
                    advance();
                    done = true;
                }
                break;
            case '{':
            case '}':
            case ';':
            case ',':   //just return the brace or the ';'
                countLines();
                result = *mCurrent;
                advance();
                done = true;
                break;
            case '>':  // keep stream operators
                if (*(mCurrent + 1) == '>') {
                  countLines();
                  result = ">>";
                  advance();
                  done = true;
                } else
                  advance();
                break;
            case '<':
                if (*(mCurrent + 1) == '<') {
                    countLines();
                    result = "<<";
                    advance();
                    done = true;
                } else
                    advance();
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
            advance();
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

QString CppTokenizer::getWord(bool bSkipParenthesis, bool bSkipArray, bool bSkipBlock)
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
    // Append the operator characters and argument list to the operator word
    if ((currentWord == "operator") ||
            (currentWord == "operator*") ||
            (currentWord == "operator&")) {
        // Spaces between 'operator' and the operator itself are allowed
        while (isSpaceChar(*mCurrent))
            mCurrent++;
        // Find end of operator
        while (isOperatorChar(*mCurrent))
            mCurrent++;
        currentWord = QString(offset,mCurrent-offset);
    } else if (currentWord == "template") {
        bFoundTemplate = true;
    }


    QString result;
    // We found a word...
    if (!currentWord.isEmpty()) {
        result = currentWord;
        // Skip whitespace
        skipToNextToken();

        // Skip template contents, but keep template variable types
        if (*mCurrent == '<') {
            offset = mCurrent; //we don't skip
            skipTemplateArgs();

            if (!bFoundTemplate) {
                result += QString(offset, mCurrent-offset);
                skipToNextToken();
            }
        } else if (bSkipArray && (*mCurrent == '[')) {
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
        } else if (bSkipBlock && (*mCurrent == '{')) {
            skipPair('{', '}');
            skipToNextToken();
        }

        // Keep parent/child operators
        if (*mCurrent == '.') {
            result+=*mCurrent;
            mCurrent++;
        } else if ((*mCurrent == '-') && (*(mCurrent + 1) == '>')) {
            result+=QString(mCurrent,2);
            mCurrent+=2;
        } else if ((*mCurrent == ':') && (*(mCurrent + 1) == ':')) {
            if (result != "using") {
                result+=QString(mCurrent,2);
                mCurrent+=2;
                // Append next token to this one
                QString s = getWord(bSkipParenthesis, bSkipArray, bSkipBlock);
                result += s;
            }
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
    bool result = isLetterChar(*mCurrent);
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

void CppTokenizer::skipAssignment()
{
    while (true) {
        switch ((*mCurrent).unicode()) {
        case '(': skipPair('(', ')');
            break;
        case '"': skipDoubleQuotes();
            break;
        case '\'': skipSingleQuote();
            break;
        case '{': skipPair('{', '}'); // support struct initializers
            break;
        case '/':
            mCurrent++;
            break;
        default:
          if ((*mCurrent == 'R') && (*(mCurrent+1) == '"'))
              skipRawString();
          else
              mCurrent++;
        }
        if (*mCurrent == ','
                || *mCurrent ==';'
                || *mCurrent ==')'
                || *mCurrent =='}'
                || *mCurrent ==0)
            break;
    }
}

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

void CppTokenizer::skipPair(const QChar &cStart, const QChar cEnd, const QSet<QChar>& failChars)
{
    mCurrent++;
    while (*mCurrent != 0) {
        if ((*mCurrent == '(') && !failChars.contains('(')) {
            skipPair('(', ')', failChars);
        } else if ((*mCurrent == '[') && !failChars.contains('[')) {
            skipPair('[', ']', failChars);
        } else if ((*mCurrent == '{') && !failChars.contains('{')) {
            skipPair('{', '}', failChars);
        } else if (*mCurrent ==  cStart) {
            skipPair(cStart, cEnd, failChars);
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
        } else if (failChars.contains(*mCurrent)) {
            break;
        } else {
            mCurrent++;
        }
    }
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
    QChar* start = mCurrent;

    QSet<QChar> failSet;
    failSet.insert('{');
    failSet.insert('}');
    failSet.insert(';');
    skipPair('<', '>', failSet);

    // if we failed, return to where we came from
    if (start!=mCurrent && *(mCurrent - 1) != '>')
        mCurrent = start;
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
        advance();
}

bool CppTokenizer::isIdentChar(const QChar &ch)
{
    return ch=='_' || ch.isLetter() ;
}

void CppTokenizer::advance()
{
    switch(mCurrent->unicode()) {
    case '\"': skipDoubleQuotes();
        break;
    case '\'': skipSingleQuote();
        break;
    case '/':
        if (*(mCurrent + 1) == '=')
            skipAssignment();
        else
            mCurrent++;
        break;
    case '=': {
        if (mTokenList.size()>2
                && mTokenList[mTokenList.size()-2]->text == "using") {
            addToken("=",mCurrentLine);
            mCurrent++;
        } else
            skipAssignment();
        break;
    }
    case '&':
    case '*':
    case '!':
    case '|':
    case '+':
    case '-':
    case '~':
        if (*(mCurrent + 1) == '=')
            skipAssignment();
        else
            mCurrent++;
        break;
    case '\\':
        if (isLineChar(*(mCurrent + 1)))
            skipSplitLine();
        else
            mCurrent++;
        break;
    default:
        if ((*mCurrent == 'R') && (*(mCurrent+1) == '"'))
            skipRawString();
        else
            mCurrent++;
    }
}

bool CppTokenizer::isLetterChar(const QChar &ch)
{
//    return (ch>= 'A' && ch<='Z')
//            || (ch>='a' && ch<='z')
    return isIdentChar(ch)
            || ch == '_'
            || ch == '*'
            || ch == '&'
            || ch == '~';
}

bool CppTokenizer::isHexChar(const QChar &ch)
{
    return (ch >= 'A' && ch<='F')
            || (ch>='a' && ch<='f')
            || ch == 'x'
            || ch == 'L';
}

bool CppTokenizer::isDigitChar(const QChar &ch)
{
    return (ch>='0' && ch<='9');
}

bool CppTokenizer::isSpaceChar(const QChar &ch)
{
    return (ch == ' ' || ch == '\t');
}

bool CppTokenizer::isLineChar(const QChar &ch)
{
    return (ch=='\n' || ch=='\r');
}

bool CppTokenizer::isBlankChar(const QChar &ch)
{
    return (ch<=32);
}

bool CppTokenizer::isOperatorChar(const QChar &ch)
{
    switch (ch.unicode()) {
    case '+':
    case '-':
    case '/':
    case '*':
    case '[':
    case ']':
    case '=':
    case '%':
    case '!':
    case '&':
    case '|':
    case '>':
    case '<':
    case '^':
        return true;
    default:
        return false;
    }
}

bool CppTokenizer::currentWordEquals(QChar *wordStart, QChar *wordEnd, const QString& text)
{
    QString currentWord(wordStart, wordEnd-wordStart);
    return currentWord == text;
}
