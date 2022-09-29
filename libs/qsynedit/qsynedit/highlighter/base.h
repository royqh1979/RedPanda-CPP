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

typedef int TokenKind;

enum class TokenType {
    Default, Space, Comment,
    PreprocessDirective, String, StringEscapeSequence,
    Identifier, Symbol,
    Character, Keyword, Number};

enum class HighlighterClass {
    Composition,
    CppHighlighter,
    AsmHighlighter,
    GLSLHighlighter
};

enum class HighlighterLanguage {
    Asssembly,
    Cpp,
    GLSL
};

class HighlighterAttribute {
public:
    explicit HighlighterAttribute(const QString& name);

    QString name() const;
    void setName(const QString &name);

    FontStyles styles() const;
    void setStyles(const FontStyles &styles);

    QColor foreground() const;
    void setForeground(const QColor &color);

    QColor background() const;
    void setBackground(const QColor &background);

private:
    QColor mForeground;
    QColor mBackground;
    QString mName;
    FontStyles mStyles;
};

typedef std::shared_ptr<HighlighterAttribute> PHighlighterAttribute;
using HighlighterAttributeList = QVector<PHighlighterAttribute>;

class Highlighter {
public:
    explicit Highlighter();

    const QMap<QString, PHighlighterAttribute>& attributes() const;

    const QSet<QChar>& wordBreakChars() const;


    PHighlighterAttribute commentAttribute() const;

    PHighlighterAttribute identifierAttribute() const;

    PHighlighterAttribute keywordAttribute() const;

    PHighlighterAttribute stringAttribute() const;

    PHighlighterAttribute whitespaceAttribute() const;

    PHighlighterAttribute symbolAttribute() const;

    virtual bool isIdentChar(const QChar& ch) const;

    virtual HighlighterClass getClass() const = 0;
    virtual QString getName() const = 0;

    virtual bool getTokenFinished() const = 0;
    virtual bool isLastLineCommentNotFinished(int state) const = 0;
    virtual bool isLastLineStringNotFinished(int state) const = 0;
    virtual bool eol() const = 0;
    virtual HighlighterState getState() const = 0;
    virtual QString getToken() const=0;
    virtual PHighlighterAttribute getTokenAttribute() const=0;
    virtual TokenType getTokenType();
    virtual TokenKind getTokenKind() = 0;
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
