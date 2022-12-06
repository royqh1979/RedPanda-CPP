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

namespace QSynedit {
enum SynIndentType {
    sitBrace = 0,
    sitParenthesis = 1,
    sitBracket = 2,
    sitStatement = 3,
};

struct HighlighterState {
    int state;  // current syntax parsing state
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
    bool operator==(const HighlighterState& s2);
    int getLastIndent();
    HighlighterState();
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

enum class HighlighterLanguage {
    DecideBySuffix,
    Composition,
    Asssembly,
    Cpp,
    GLSL,
    Makefile,
    Custom
};

class HighlighterAttribute {
public:
    explicit HighlighterAttribute(const QString& name, TokenType mTokenType);

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

typedef std::shared_ptr<HighlighterAttribute> PHighlighterAttribute;
using HighlighterAttributeList = QVector<PHighlighterAttribute>;

class Highlighter {
public:
    explicit Highlighter();

    const QMap<QString, PHighlighterAttribute>& attributes() const;

    const QSet<QChar>& wordBreakChars() const;

    const PHighlighterAttribute& identifierAttribute() const;

    const PHighlighterAttribute& keywordAttribute() const;

    const PHighlighterAttribute& commentAttribute() const;

    const PHighlighterAttribute& stringAttribute() const;

    const PHighlighterAttribute& whitespaceAttribute() const;

    const PHighlighterAttribute& symbolAttribute() const;

    virtual bool isIdentChar(const QChar& ch) const;

    virtual bool getTokenFinished() const = 0;
    virtual bool isLastLineCommentNotFinished(int state) const = 0;
    virtual bool isLastLineStringNotFinished(int state) const = 0;
    virtual bool eol() const = 0;
    virtual HighlighterState getState() const = 0;
    virtual QString getToken() const=0;
    virtual const PHighlighterAttribute &getTokenAttribute() const=0;
    virtual int getTokenPos() = 0;
    virtual bool isKeyword(const QString& word);
    virtual void next() = 0;
    virtual void nextToEol();
    virtual void setState(const HighlighterState& rangeState) = 0;
    virtual void setLine(const QString& newLine, int lineNumber) = 0;
    virtual void resetState() = 0;
    virtual QSet<QString> keywords() const;

    virtual QString languageName() = 0;
    virtual HighlighterLanguage language() = 0;

    virtual QString foldString();

    virtual bool supportBraceLevel();
    virtual bool isSpaceChar(const QChar& ch);
    virtual bool isWordBreakChar(const QChar& ch);
    bool enabled() const;
    void setEnabled(bool value);
    virtual PHighlighterAttribute getAttribute(const QString& name) const;

protected:
    PHighlighterAttribute mCommentAttribute;
    PHighlighterAttribute mIdentifierAttribute;
    PHighlighterAttribute mKeywordAttribute;
    PHighlighterAttribute mStringAttribute;
    PHighlighterAttribute mWhitespaceAttribute;
    PHighlighterAttribute mSymbolAttribute;

    void addAttribute(PHighlighterAttribute attribute);
    void clearAttributes();
    virtual int attributesCount() const;

private:
    QMap<QString,PHighlighterAttribute> mAttributes;
    bool mEnabled;
    QSet<QChar> mWordBreakChars;
};

using PHighlighter = std::shared_ptr<Highlighter>;
using HighlighterList = QVector<PHighlighter>;

}
#endif // SYNHIGHLIGTERBASE_H
