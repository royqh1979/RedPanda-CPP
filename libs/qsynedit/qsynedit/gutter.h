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
#ifndef GUTTER_H
#define GUTTER_H

#include <QColor>
#include <QFont>
#include <QObject>
#include "types.h"

namespace QSynedit {

enum class GutterBorderStyle {
    None,
    Middle,
    Right
};

class Gutter : public QObject {
    Q_OBJECT
public:
    explicit Gutter(QObject* parent = nullptr);
    Gutter(const Gutter&)=delete;
    Gutter& operator=(const Gutter&)=delete;
    QFont font() const;
    void setFont(const QFont &value);

    bool autoSize() const;
    void setAutoSize(bool value);

    QColor borderColor() const;
    void setBorderColor(const QColor &value);

    QColor color() const;
    void setColor(const QColor &value);

    int digitCount() const;
    void setDigitCount(int value);

    GutterBorderStyle borderStyle() const;
    void setBorderStyle(const GutterBorderStyle &value);

    bool gradient() const;
    void setGradient(bool value);

    QColor gradientStartColor() const;
    void setGradientStartColor(const QColor &value);

    QColor gradientEndColor() const;
    void setGradientEndColor(const QColor &value);

    int gradientSteps() const;
    void setGradientSteps(int value);

    bool leadingZeros() const;
    void setLeadingZeros(bool value);


    int leftOffset() const;
    void setLeftOffset(int leftOffset);

    int lineNumberStart() const;
    void setLineNumberStart(int lineNumberStart);

    bool zeroStart();
    int rightOffset() const;
    void setRightOffset(int rightOffset);

    bool showLineNumbers() const;
    void setShowLineNumbers(bool showLineNumbers);

    bool useFontStyle() const;
    void setUseFontStyle(bool useFontStyle);

    bool visible() const;
    void setVisible(bool visible);

    void autoSizeDigitCount(int linesCount);
    QString formatLineNumber(int line);
    int realGutterWidth(int charWidth);

    QColor textColor() const;
    void setTextColor(const QColor &value);

    const QColor &activeLineTextColor() const;
    void setActiveLineTextColor(const QColor &newActiveLineTextColor);

signals:
    void changed();
private:
    void setChanged();
private:
    bool mAutoSize;
    QColor mBorderColor;
    QColor mTextColor;
    QColor mActiveLineTextColor;
    QColor mColor;
    int mDigitCount;
    QFont mFont;
    bool mGradient;
    QColor mGradientStartColor;
    QColor mGradientEndColor;
    int mGradientSteps;
    bool mLeadingZeros;
    int mLeftOffset;
    int mLineNumberStart;
    int mRightOffset;
    bool mShowLineNumbers;
    GutterBorderStyle mBorderStyle;
    bool mUseFontStyle;
    bool mVisible;
    int mAutoSizeDigitCount;
};

using PSynGutter = std::shared_ptr<Gutter>;

}

#endif // GUTTER_H
