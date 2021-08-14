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

QString cpptokenizer::getNextToken(bool bSkipParenthesis, bool bSkipArray, bool bSkipBlock)
{
    QString result;
    bool done = false;
    while (true) {
        skipToNextToken();
        if (*mCurrent == '\0')
            break;
        if (isPreprocessor()) {
            countLines();
            result = getPreprocessor(); // don't count preprocessor lines
            if (result.startsWith("#include")) { // if we find
                int delimPos = result.lastIndexOf(':');
                if (delimPos >= 0) {
                    bool ok;
                    mCurrentLine = result.mid(delimPos+1).toInt(&ok)-1; // fCurrLine is 0 based
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
            case '\0':
                done = true;
                break;
            case '/':
                advance();
                break;
            case '{':
            case '}':
            case ';':
            case ',':
            case ':':  //just return the brace or the ';'
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

QString cpptokenizer::getPreprocessor()
{
    QChar *offset = mCurrent;
    skipToEOL();
    return QString(offset, mCurrent-offset);
}

QString cpptokenizer::getWord(bool bSkipParenthesis, bool bSkipArray, bool bSkipBlock)
{
    bool bFoundTemplate = false;
    //  bIsSmartPointer:=False;

    // Skip spaces
    skipToNextToken();

    // Get next word...
    QChar* offset = mCurrent;

    // Copy the word ahead of us
    while (isLetterChar(*mCurrent) || isDigitChar(*mCurrent))
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
            result+=QString(mCurrent,2);
            mCurrent+=2;
            // Append next token to this one
            QString s = getWord(bSkipParenthesis, bSkipArray, bSkipBlock);
            result += s;
        }
    }
    return result;
}

bool cpptokenizer::isArguments()
{
    return *mCurrent == '(';
}

bool cpptokenizer::isForInit()
{
    return (*mCurrent == '(') && (mLastToken == "for");
}

bool cpptokenizer::isNumber()
{
    return isDigitChar(*mCurrent);
}

bool cpptokenizer::isPreprocessor()
{
    return *mCurrent=='#';
}

bool cpptokenizer::isWord()
{
    bool result = isLetterChar(*mCurrent);
    if (result && (*(mCurrent+1) == '"'))
        result = false;
    return result;
}

void cpptokenizer::simplify(QString &output)
{
    //remove \n \r;
    QString temp;
    for (QChar ch:output) {
        if (!isLineChar(ch))
            temp+=ch;
    }
    output = temp.trimmed();
}

void cpptokenizer::simplifyArgs(QString &output)
{
    QString temp;
    QString lastSpace = "";
    bool parentheseStart = true;
    for (QChar ch:output.trimmed()) {
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

void cpptokenizer::skipAssignment()
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
                || *mCurrent =='\0')
            break;
    }
}

void cpptokenizer::skipDoubleQuotes()
{
    mCurrent++;
    while (!(*mCurrent=='"' || *mCurrent == '\0')) {
        if (*mCurrent == '\\')
            mCurrent+=2; // skip escaped char
        else
            mCurrent++;
    }
    if (*mCurrent!='\0') {
        mCurrent++;
    }
}

void cpptokenizer::skipPair(const QChar &cStart, const QChar cEnd, const QSet<QChar>& failChars)
{
    mCurrent++;
    while (*mCurrent != '\0') do {
      if (pCurrent^ = '(') and not ('(' in FailChars) then begin
        SkipPair('(', ')', FailChars);
      end else if (pCurrent^ = '[') and not ('[' in FailChars) then begin
        SkipPair('[', ']', FailChars);
      end else if (pCurrent^ = '{') and not ('}' in FailChars) then begin
        SkipPair('{', '}', FailChars);
      end else if (pCurrent^ = cStart) then begin
        SkipPair(cStart, cEnd, FailChars);
      end else if pCurrent^ = cEnd then begin
        Inc(pCurrent); // skip over end
        break;
      end else if (pCurrent^ = 'R') and ((pCurrent+1)^ = '"') then begin
        if cStart <> '''' then
          SkipRawString // don't do it inside AnsiString!
        else
          Inc(pCurrent);
      end else if pCurrent^ = '"' then begin
        if cStart <> '''' then
          SkipDoubleQuotes // don't do it inside AnsiString!
        else
          Inc(pCurrent);
      end else if pCurrent^ = '''' then begin
        SkipSingleQuote;
      end else if pCurrent^ = '/' then begin
        if (pCurrent + 1)^ = '/' then
          SkipToEOL
        else if (pCurrent + 1)^ = '*' then
          SkipCStyleComment // skips over */
        else
          Inc(pCurrent);
      end else if pCurrent^ in FailChars then begin
        Exit;
      end else
        Inc(pCurrent);
    end;
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

bool cpptokenizer::isLetterChar(const QChar &ch)
{
    return (ch>= 'A' && ch<='Z')
            || (ch>='a' && ch<='z')
            || ch == '_'
            || ch == '*'
            || ch == '&'
            || ch == '~';
}

bool cpptokenizer::isHexChar(const QChar &ch)
{
    return (ch >= 'A' && ch<='F')
            || (ch>='a' && ch<='f')
            || ch == 'x'
            || ch == 'L';
}

bool cpptokenizer::isDigitChar(const QChar &ch)
{
    return (ch>='0' && ch<='9');
}

bool cpptokenizer::isSpaceChar(const QChar &ch)
{
    return (ch == ' ' || ch == '\t');
}

bool cpptokenizer::isLineChar(const QChar &ch)
{
    return (ch=='\n' || ch=='\r');
}

bool cpptokenizer::isBlankChar(const QChar &ch)
{
    return (ch<=32);
}

bool cpptokenizer::isOperatorChar(const QChar &ch)
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

bool cpptokenizer::currentWordEquals(QChar *wordStart, QChar *wordEnd, const QString& text)
{
    QString currentWord(wordStart, wordEnd-wordStart);
    return currentWord == text;
}
