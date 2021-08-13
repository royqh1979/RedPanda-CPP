#ifndef CPPTOKENIZER_H
#define CPPTOKENIZER_H

#include <QObject>
#include "parserutils.h"

class cpptokenizer : public QObject
{
    Q_OBJECT
public:
    struct Token {
      QString text;
      int line;
    };
    using PToken = std::shared_ptr<Token>;
    using TokenList = QVector<PToken>;
    explicit cpptokenizer(QObject *parent = nullptr);

signals:
private:
    void addToken(const QString& sText, int iLine);
    void countLines();
    PToken getToken(int index);
    void skipSplitLine();
    void skipToNextToken();
    void skipDoubleQuotes();
    void skipRawString();
    void skipSingleQuote();
    void skipPair(const QChar& cStart, const QChar cEnd, QSet<QChar> failChars = QSet<QChar>());
    void skipAssignment();
    void skipTemplateArgs();
    QString getArguments();
    QString getForInit();
    QString getNumber();
    QString getPreprocessor();
    QString getWord(
            bool bSkipParenthesis = false,
            bool bSkipArray = false,
            bool bSkipBlock = false);
    bool isWord();
    bool isNumber();
    bool isPreprocessor();
    bool isArguments();
    bool isForInit();
    QString getNextToken(
            bool bSkipParenthesis = false,
            bool bSkipArray = false,
            bool bSkipBlock = false);
    void simplify(QString& output);
    void simplifyArgs(QString& output);
    void advance();
    bool openFile(const QString& fileName);

private:
    QStringList mBuffer;
    QString mBufferStr;
    QChar* mStart;
    QChar* mCurrent;
    QChar* mLineCount;
    int mCurrentLine;
    QString mLastToken;
    int mEnd;
    TokenList mTokenList;
    QString mFilename;
};

#endif // CPPTOKENIZER_H
