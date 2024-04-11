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
#include <QVariant>
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
    QVector<IndentInfo> indents; // indents stack (needed by auto indent)
    IndentInfo lastUnindent;
    bool hasTrailingSpaces;
    QMap<QString,QVariant> extraData;

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

    QString name() const { return mName; }

    FontStyles styles() const { return mStyles; }
    void setStyles(const FontStyles &styles) {
        if (mStyles!=styles) {
            mStyles = styles;
        }
    }

    const QColor &foreground() const { return mForeground; }
    void setForeground(const QColor &color) { mForeground = color; }

    const QColor &background() const { return mBackground; }
    void setBackground(const QColor &background) { mBackground = background; }

    TokenType tokenType() const { return mTokenType; }

private:
    QColor mForeground;
    QColor mBackground;
    QString mName;
    FontStyles mStyles;
    TokenType mTokenType;
};

using PTokenAttribute = std::shared_ptr<TokenAttribute> ;

class Syntaxer {
public:
    explicit Syntaxer();
    Syntaxer(const Syntaxer&)=delete;
    Syntaxer& operator=(const Syntaxer&)=delete;

    QMap<QString, PTokenAttribute> attributes() const { return mAttributes; }

    const QSet<QChar>& wordBreakChars() const { return mWordBreakChars; }

    const PTokenAttribute& identifierAttribute() const { return mIdentifierAttribute; }

    const PTokenAttribute& keywordAttribute() const { return mKeywordAttribute; }

    const PTokenAttribute& commentAttribute() const { return mCommentAttribute; }

    const PTokenAttribute& stringAttribute() const { return mStringAttribute; }

    const PTokenAttribute& whitespaceAttribute() const { return mWhitespaceAttribute; }

    const PTokenAttribute& symbolAttribute() const { return mSymbolAttribute; }

    virtual bool isIdentChar(const QChar& ch) const;
    virtual bool isIdentStartChar(const QChar& ch) const;

    virtual bool isCommentNotFinished(int state) const = 0;
    virtual bool isStringNotFinished(int state) const = 0;
    virtual bool isDocstringNotFinished(int /* state */) const { return false; }
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
    virtual PTokenAttribute getAttribute(const QString& name) const;
    virtual QString commentSymbol();
    virtual QString blockCommentBeginSymbol();
    virtual QString blockCommentEndSymbol();

    virtual bool supportFolding() = 0;
    virtual bool needsLineState() = 0;


protected:
    PTokenAttribute mCommentAttribute;
    PTokenAttribute mIdentifierAttribute;
    PTokenAttribute mKeywordAttribute;
    PTokenAttribute mStringAttribute;
    PTokenAttribute mWhitespaceAttribute;
    PTokenAttribute mSymbolAttribute;

    void addAttribute(PTokenAttribute attribute) { mAttributes[attribute->name()]=attribute; }
    void clearAttributes() { mAttributes.clear(); }
    virtual int attributesCount() const { return mAttributes.size(); }

private:
    QMap<QString,PTokenAttribute> mAttributes;
    QSet<QChar> mWordBreakChars;
};

using PSyntaxer = std::shared_ptr<Syntaxer>;
using SyntaxerList = QVector<PSyntaxer>;
}
#endif // QSYNEDIT_SYNTAXER_H
