#ifndef SYNHIGHLIGTERBASE_H
#define SYNHIGHLIGTERBASE_H

#include <QColor>
#include <QObject>
#include <memory>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QVector>
#include "../Types.h"

enum SynIndentType {
    sitBrace = 0,
    sitParenthesis = 1,
    sitBracket = 2,
    sitStatement = 3,
};

struct SynRangeState {
    int state;  // current syntax parsing state
    int spaceState; // the last syntax parsing state before meeting space
    int braceLevel; // current braces embedding level (needed by rainbow color)
    int bracketLevel; // current brackets embedding level (needed by rainbow color)
    int parenthesisLevel; // current parenthesis embedding level (needed by rainbow color)
    int leftBraces; // unpairing left braces in the current line ( needed by block folding)
    int rightBraces; // unparing right braces in the current line (needed by block folding)
    QVector<int> indents; // indents stack (needed by auto indent)
    int firstIndentThisLine; /* index of first indent that appended to the indents
                              *  stack at this line ( need by auto indent) */
    QVector<int> matchingIndents; /* the indent matched ( and removed )
                              but not started at this line
                                (need by auto indent) */
    bool operator==(const SynRangeState& s2);
    int getLastIndent();
};

typedef int SynTokenKind;

enum class SynHighlighterTokenType {
    Default, Space, Comment,
    PreprocessDirective, String, StringEscapeSequence,
    Identifier, Symbol,
    Character, Keyword, Number};

enum class SynHighlighterClass {
    Composition,
    CppHighlighter,
    AsmHighlighter
};

enum class SynHighlighterLanguage {
    Asssembly,
    Cpp,
};

class SynHighlighterAttribute {
public:
    explicit SynHighlighterAttribute(const QString& name);

    QString name() const;
    void setName(const QString &name);

    SynFontStyles styles() const;
    void setStyles(const SynFontStyles &styles);

    QColor foreground() const;
    void setForeground(const QColor &color);

    QColor background() const;
    void setBackground(const QColor &background);

private:
    QColor mForeground;
    QColor mBackground;
    QString mName;
    SynFontStyles mStyles;
};

typedef std::shared_ptr<SynHighlighterAttribute> PSynHighlighterAttribute;
using SynHighlighterAttributeList = QVector<PSynHighlighterAttribute>;

class SynHighlighter {
public:
    explicit SynHighlighter();

    const QMap<QString, PSynHighlighterAttribute>& attributes() const;

    const QSet<QChar>& wordBreakChars() const;



    PSynHighlighterAttribute commentAttribute() const;

    PSynHighlighterAttribute identifierAttribute() const;

    PSynHighlighterAttribute keywordAttribute() const;

    PSynHighlighterAttribute stringAttribute() const;

    PSynHighlighterAttribute whitespaceAttribute() const;

    PSynHighlighterAttribute symbolAttribute() const;

    virtual bool isIdentChar(const QChar& ch) const;

    virtual SynHighlighterClass getClass() const = 0;
    virtual QString getName() const = 0;

    virtual bool getTokenFinished() const = 0;
    virtual bool isLastLineCommentNotFinished(int state) const = 0;
    virtual bool isLastLineStringNotFinished(int state) const = 0;
    virtual bool eol() const = 0;
    virtual SynRangeState getRangeState() const = 0;
    virtual QString getToken() const=0;
    virtual PSynHighlighterAttribute getTokenAttribute() const=0;
    virtual SynHighlighterTokenType getTokenType();
    virtual SynTokenKind getTokenKind() = 0;
    virtual int getTokenPos() = 0;
    virtual bool isKeyword(const QString& word);
    virtual void next() = 0;
    virtual void nextToEol();
    virtual void setState(const SynRangeState& rangeState) = 0;
    virtual void setLine(const QString& newLine, int lineNumber) = 0;
    virtual void resetState() = 0;

    virtual QString languageName() = 0;
    virtual SynHighlighterLanguage language() = 0;

    virtual bool isSpaceChar(const QChar& ch);
    virtual bool isWordBreakChar(const QChar& ch);
    bool enabled() const;
    void setEnabled(bool value);
    virtual PSynHighlighterAttribute getAttribute(const QString& name) const;

protected:
    PSynHighlighterAttribute mCommentAttribute;
    PSynHighlighterAttribute mIdentifierAttribute;
    PSynHighlighterAttribute mKeywordAttribute;
    PSynHighlighterAttribute mStringAttribute;
    PSynHighlighterAttribute mWhitespaceAttribute;
    PSynHighlighterAttribute mSymbolAttribute;

    void addAttribute(PSynHighlighterAttribute attribute);
    void clearAttributes();
    virtual int attributesCount() const;

private:
    QMap<QString,PSynHighlighterAttribute> mAttributes;
    bool mEnabled;
    QSet<QChar> mWordBreakChars;
};

using PSynHighlighter = std::shared_ptr<SynHighlighter>;
using SynHighlighterList = QVector<PSynHighlighter>;

#endif // SYNHIGHLIGTERBASE_H
