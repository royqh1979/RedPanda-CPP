#ifndef CPPTOKENIZER_H
#define CPPTOKENIZER_H

#include <QObject>
#include "parserutils.h"

class CppTokenizer
{
public:
    struct Token {
      QString text;
      int line;
    };
    using PToken = std::shared_ptr<Token>;
    using TokenList = QVector<PToken>;
    explicit CppTokenizer();

    void reset();
    void tokenize(const QStringList& buffer);
    void dumpTokens(const QString& fileName);
    const TokenList& tokens();
    PToken operator[](int i);
    int tokenCount();
signals:
private:
    void addToken(const QString& sText, int iLine);
    void advance();
    void countLines();
    PToken getToken(int index);

    QString getArguments();
    QString getForInit();
    QString getNextToken(
            bool bSkipParenthesis = false,
            bool bSkipArray = false,
            bool bSkipBlock = false);
    QString getNumber();
    QString getPreprocessor();
    QString getWord(
            bool bSkipParenthesis = false,
            bool bSkipArray = false,
            bool bSkipBlock = false);
    bool isArguments();
    bool isForInit();
    bool isNumber();
    bool isPreprocessor();
    bool isWord();
    void simplify(QString& output);
    void simplifyArgs(QString& output);
    void skipAssignment();
    void skipDoubleQuotes();
    void skipPair(const QChar& cStart, const QChar cEnd, const QSet<QChar>& failChars = QSet<QChar>());
    void skipRawString();
    void skipSingleQuote();
    void skipSplitLine();
    void skipTemplateArgs();
    void skipToEOL();
    void skipToNextToken();
    bool openFile(const QString& fileName);
    bool isLetterChar(const QChar& ch);
    bool isHexChar(const QChar& ch);
    bool isDigitChar(const QChar& ch);
    bool isSpaceChar(const QChar& ch);
    bool isLineChar(const QChar& ch);
    bool isBlankChar(const QChar& ch);
    bool isOperatorChar(const QChar& ch);

    bool currentWordEquals(QChar* wordStart, QChar *wordEnd, const QString& text);

private:
    QStringList mBuffer;
    QString mBufferStr;
    QChar* mStart;
    QChar* mCurrent;
    QChar* mLineCount;
    int mCurrentLine;
    QString mLastToken;
    TokenList mTokenList;
};

#endif // CPPTOKENIZER_H
