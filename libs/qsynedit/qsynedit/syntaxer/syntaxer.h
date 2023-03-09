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
#ifndef QSYNEDIT_SYNTAXER_H
#define QSYNEDIT_SYNTAXER_H

#include <QColor>
#include <QObject>
#include <memory>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QVector>
#include "../types.h"

namespace QSynedit {

enum class IndentType {
    Brace,
    Parenthesis,
    Bracket,
    Statement,
    Block,
    None
};

struct IndentInfo {
    IndentType type;
    int line;
    bool operator==(const IndentInfo &i2) const;
};

struct SyntaxState {
    int state;  // current syntax parsing state
    int blockLevel; // needed by block folding
    int blockStarted;  // needed by block folding
    int blockEnded;    // needed by block folding;
    int blockEndedLastLine; //needed by block folding;
    int braceLevel; // current braces embedding level (needed by rainbow color)
    int bracketLevel; // current brackets embedding level (needed by rainbow color)
    int parenthesisLevel; // current parenthesis embedding level (needed by rainbow color)
//    int leftBraces; // unpairing left braces in the current line ( needed by block folding)
//    int rightBraces; // unparing right braces in the current line (needed by block folding)
    QVector<IndentInfo> indents;
    IndentInfo lastUnindent;
//    QVector<int> indents; // indents stack (needed by auto indent)
//    int firstIndentThisLine; /* index of first indent that appended to the indents
//                              *  stack at this line ( need by auto indent) */
//    QVector<int> matchingIndents; /* the indent matched ( and removed )
//                              but not started at this line
//                                (need by auto indent) */
    bool hasTrailingSpaces;
    bool operator==(const SyntaxState& s2);
    IndentInfo getLastIndent();
    IndentType getLastIndentType();
    SyntaxState();
};

enum class TokenType {
    Default,
    Comment, // any comment
    Space,

    String,   // a string constant: "this is a string"
    Character, // a character constant: 'c', '\n'
    Number, // a number constant: 234, 0xff

    Identifier, // any variable name

    Keyword, // any keyword

    Operator, // "sizeof", "+", "*", etc.

    Preprocessor,    //generic Preprocessor

    Error,

    Embeded  //language embeded in others
    };

class TokenAttribute {
public:
    explicit TokenAttribute(const QString& name, TokenType mTokenType);
    TokenAttribute(const TokenAttribute&)=delete;
    TokenAttribute& operator=(const TokenAttribute&)=delete;

    QString name() const;

    FontStyles styles() const;
    void setStyles(const FontStyles &styles);

    const QColor &foreground() const;
    void setForeground(const QColor &color);

    const QColor &background() const;
    void setBackground(const QColor &background);

    TokenType tokenType() const;

private:
    QColor mForeground;
    QColor mBackground;
    QString mName;
    FontStyles mStyles;
    TokenType mTokenType;
};

typedef std::shared_ptr<TokenAttribute> PTokenAttribute;

class Syntaxer {
public:
    explicit Syntaxer();
    Syntaxer(const Syntaxer&)=delete;
    Syntaxer& operator=(const Syntaxer&)=delete;

    const QMap<QString, PTokenAttribute>& attributes() const;

    const QSet<QChar>& wordBreakChars() const;

    const PTokenAttribute& identifierAttribute() const;

    const PTokenAttribute& keywordAttribute() const;

    const PTokenAttribute& commentAttribute() const;

    const PTokenAttribute& stringAttribute() const;

    const PTokenAttribute& whitespaceAttribute() const;

    const PTokenAttribute& symbolAttribute() const;

    virtual bool isIdentChar(const QChar& ch) const;

    virtual bool getTokenFinished() const = 0;
    virtual bool isLastLineCommentNotFinished(int state) const = 0;
    virtual bool isLastLineStringNotFinished(int state) const = 0;
    virtual bool eol() const = 0;
    virtual SyntaxState getState() const = 0;
    virtual QString getToken() const=0;
    virtual const PTokenAttribute &getTokenAttribute() const=0;
    virtual int getTokenPos() = 0;
    virtual bool isKeyword(const QString& word);
    virtual void next() = 0;
    virtual void nextToEol();
    virtual void setState(const SyntaxState& rangeState) = 0;
    virtual void setLine(const QString& newLine, int lineNumber) = 0;
    virtual void resetState() = 0;
    virtual QSet<QString> keywords();
    virtual QMap<QString,QSet<QString>> scopedKeywords();

    virtual QString languageName() = 0;
    virtual ProgrammingLanguage language() = 0;

    virtual QString foldString(QString startLine);

    virtual bool supportBraceLevel();
    virtual bool isSpaceChar(const QChar& ch);
    virtual bool isWordBreakChar(const QChar& ch);
    bool enabled() const;
    void setEnabled(bool value);
    virtual PTokenAttribute getAttribute(const QString& name) const;
    virtual QString commentSymbol();
    virtual QString blockCommentBeginSymbol();
    virtual QString blockCommentEndSymbol();


protected:
    PTokenAttribute mCommentAttribute;
    PTokenAttribute mIdentifierAttribute;
    PTokenAttribute mKeywordAttribute;
    PTokenAttribute mStringAttribute;
    PTokenAttribute mWhitespaceAttribute;
    PTokenAttribute mSymbolAttribute;

    void addAttribute(PTokenAttribute attribute);
    void clearAttributes();
    virtual int attributesCount() const;

private:
    QMap<QString,PTokenAttribute> mAttributes;
    bool mEnabled;
    QSet<QChar> mWordBreakChars;
};

using PSyntaxer = std::shared_ptr<Syntaxer>;
using SyntaxerList = QVector<PSyntaxer>;
}
#endif // SYNHIGHLIGTERBASE_H
