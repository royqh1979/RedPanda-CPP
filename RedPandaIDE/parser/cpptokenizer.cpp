#include "cpptokenizer.h"

cpptokenizer::cpptokenizer(QObject *parent) : QObject(parent)
{

}

void cpptokenizer::addToken(const QString &sText, int iLine)
{
    PToken token = std::make_shared<Token>();
    token->text = sText;
    token->line = iLine;
    mTokenList.append(token);
}

void cpptokenizer::countLines()
{
    while ((*mLineCount != '\0') && (mLineCount < mCurrent)) {
        if (*mLineCount == '\n')
            mCurrentLine ++;
        mLineCount++;
    }
}

QString cpptokenizer::getArguments()
{
    QChar* offset = mCurrent;
    skipPair('(', ')');
    QString result(offset,mCurrent-offset);
    simplifyArgs(result);
    if ((*mCurrent == '.') || ((*mCurrent == '-') && (*(mCurrent + 1) == '>'))) {
        // skip '.' and '->'
        while ( !( *mCurrent == '\0'
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

QString cpptokenizer::getForInit()
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

QString cpptokenizer::getNumber()
{
    QChar* offset = mCurrent;

    if (isDigitChar(*mCurrent)) {
        while (isDigitChar(*mCurrent) || isHexChar(*mCurrent)) {
            advance();
        }
    }

    QString result;
    if (offset != mpCurrent) {
        result = QString(offset,mCurrent-offset);
        if (*mCurrent=='.') // keep '.' for decimal
            result += *mCurrent;
    }
    return result;
}

void cpptokenizer::advance()
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
    case '=':
        skipAssignment();
        break;
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
