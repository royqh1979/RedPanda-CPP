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
#include "gutter.h"
#include "algorithm"
#include <qt_utils/utils.h>

namespace QSynedit {

Gutter::Gutter(QObject *parent):
    QObject(parent)
{
    mFont = QFont("Courier New",10);
    mColor= Qt::lightGray;
    mBorderColor = Qt::transparent;
    mTextColor = Qt::black;
    mShowLineNumbers = true;
    mDigitCount = 1;
    mLeadingZeros = false;
    mLeftOffset = 16;
    mRightOffset = 2;
    mVisible = true;
    mUseFontStyle = true;
    mAutoSize = true;
    mAutoSizeDigitCount = mDigitCount;
    mBorderStyle = GutterBorderStyle::Middle;
    mLineNumberStart = 1;
    mGradient = false;
    mGradientStartColor = Qt::transparent;
    mGradientEndColor = Qt::transparent;
    mGradientSteps = 48;
}

QFont Gutter::font() const
{
    return mFont;
}

void Gutter::setFont(const QFont &font)
{
    if (mFont != font) {
        mFont = font;
        setChanged();
    }
}

bool Gutter::autoSize() const
{
    return mAutoSize;
}

void Gutter::setAutoSize(bool value)
{
    if (mAutoSize != value) {
        mAutoSize = value;
        setChanged();
    }
}

void Gutter::setChanged()
{
    emit changed();
}

const QColor &Gutter::activeLineTextColor() const
{
    return mActiveLineTextColor;
}

void Gutter::setActiveLineTextColor(const QColor &newActiveLineTextColor)
{
    mActiveLineTextColor = newActiveLineTextColor;
}

QColor Gutter::textColor() const
{
    return mTextColor;
}

void Gutter::setTextColor(const QColor &value)
{
    if (mTextColor!=value) {
        mTextColor = value;
        setChanged();
    }
}

void Gutter::autoSizeDigitCount(int linesCount)
{
    if (mVisible && mAutoSize && mShowLineNumbers) {
        linesCount += (mLineNumberStart - 1);
    }
    int nDigits = std::max(QString::number(linesCount).length(), mDigitCount);
    if (mAutoSizeDigitCount!=nDigits) {
        mAutoSizeDigitCount = nDigits;
        setChanged();
    }
}

QString Gutter::formatLineNumber(int line)
{
    line += (mLineNumberStart - 1);
    QString result = QString::number(line);
    if (mLeadingZeros) {
        return QString(mAutoSizeDigitCount - result.length(),'0') + result;
    } else {
        return result;
    }
}

int Gutter::realGutterWidth(int charWidth)
{
    if (!mVisible) {
        return 0;
    }
    if (mShowLineNumbers) {
        return mLeftOffset + mRightOffset + mAutoSizeDigitCount * charWidth + 2;
    }
    return mLeftOffset + mRightOffset;
}

bool Gutter::visible() const
{
    return mVisible;
}

void Gutter::setVisible(bool visible)
{
    if (mVisible!=visible) {
        mVisible = visible;
        setChanged();
    }
}

bool Gutter::useFontStyle() const
{
    return mUseFontStyle;
}

void Gutter::setUseFontStyle(bool useFontStyle)
{
    if (mUseFontStyle!=useFontStyle) {
        mUseFontStyle = useFontStyle;
        setChanged();
    }
}

bool Gutter::showLineNumbers() const
{
    return mShowLineNumbers;
}

void Gutter::setShowLineNumbers(bool showLineNumbers)
{
    if (mShowLineNumbers!=showLineNumbers) {
        mShowLineNumbers = showLineNumbers;
        setChanged();
    }
}

int Gutter::rightOffset() const
{
    return mRightOffset;
}

void Gutter::setRightOffset(int rightOffset)
{
    int value = std::max(0, rightOffset);
    if (mRightOffset != value) {
        mRightOffset = value;
        setChanged();
    }
}

int Gutter::lineNumberStart() const
{
    return mLineNumberStart;
}

void Gutter::setLineNumberStart(int lineNumberStart)
{
    int value = std::max(0,lineNumberStart);
    if (mLineNumberStart!=value) {
        mLineNumberStart = value;
        setChanged();
    }
}

bool Gutter::zeroStart()
{
    return mLineNumberStart == 0;
}

int Gutter::leftOffset() const
{
    return mLeftOffset;
}

void Gutter::setLeftOffset(int leftOffset)
{
    int value = std::max(0,leftOffset);
    if (mLeftOffset != value) {
        mLeftOffset = value;
        setChanged();
    }
}

bool Gutter::leadingZeros() const
{
    return mLeadingZeros;
}

void Gutter::setLeadingZeros(bool value)
{
    if (mLeadingZeros!=value) {
        mLeadingZeros = value;
        setChanged();
    }
}

int Gutter::gradientSteps() const
{
    return mGradientSteps;
}

void Gutter::setGradientSteps(int value)
{
    if (mGradientSteps!=value) {
        mGradientSteps = value;
        if (mGradientSteps<2)
            mGradientSteps = 2;
        setChanged();
    }
}

QColor Gutter::gradientEndColor() const
{
    return mGradientEndColor;
}

void Gutter::setGradientEndColor(const QColor &value)
{
    if (mGradientEndColor!=value) {
        mGradientEndColor = value;
        setChanged();
    }
}

QColor Gutter::gradientStartColor() const
{
    return mGradientStartColor;
}

void Gutter::setGradientStartColor(const QColor &value)
{
    if (mGradientStartColor!=value) {
        mGradientStartColor = value;
        setChanged();
    }
}

bool Gutter::gradient() const
{
    return mGradient;
}

void Gutter::setGradient(bool value)
{
    if (mGradient!=value){
        mGradient = value;
        setChanged();
    }
}

GutterBorderStyle Gutter::borderStyle() const
{
    return mBorderStyle;
}

void Gutter::setBorderStyle(const GutterBorderStyle &value)
{
    if (mBorderStyle!=value) {
        mBorderStyle = value;
        setChanged();
    }
}

int Gutter::digitCount() const
{
    return mDigitCount;
}

void Gutter::setDigitCount(int value)
{
    if (mDigitCount != value ) {
        mDigitCount = value;
        setChanged();
    }
}

QColor Gutter::color() const
{
    return mColor;
}

void Gutter::setColor(const QColor &value)
{
    if (mColor!=value) {
        mColor = value;
        setChanged();
    }
}

QColor Gutter::borderColor() const
{
    return mBorderColor;
}

void Gutter::setBorderColor(const QColor &value)
{
    if (mBorderColor!=value) {
        mBorderColor = value;
        setChanged();
    }
}

}
