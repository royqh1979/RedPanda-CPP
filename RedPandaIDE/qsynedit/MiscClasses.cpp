#include "MiscClasses.h"
#include "algorithm"

SynGutter::SynGutter(QObject *parent):
    QObject(parent)
{
    mFont = QFont("Courier New",9);
    mColor= QColorConstants::Svg::lightgray;
    mBorderColor = QColorConstants::Transparent;
    mWidth = 30;
    mShowLineNumbers = true;
    mDigitCount = 4;
    mLeadingZeros = false;
    mLeftOffset = 16;
    mRightOffset = 2;
    mVisible = true;
    mUseFontStyle = false;
    mAutoSize = true;
    mAutoSizeDigitCount = mDigitCount;
    mBorderStyle = SynGutterBorderStyle::Middle;
    mLineNumberStart = 1;
    mGradient = false;
    mGradientStartColor = QColorConstants::Transparent;
    mGradientEndColor = QColorConstants::Transparent;
    mGradientSteps = 48;
}

QFont SynGutter::font() const
{
    return mFont;
}

void SynGutter::setFont(const QFont &font)
{
    if (mFont != font) {
        mFont = font;
        setChanged();
    }
}

bool SynGutter::autoSize() const
{
    return mAutoSize;
}

void SynGutter::setAutoSize(bool value)
{
    if (mAutoSize != value) {
        mAutoSize = value;
        setChanged();
    }
}

void SynGutter::setChanged()
{
    emit changed();
}

int SynGutter::width() const
{
    return mWidth;
}

void SynGutter::setWidth(int width)
{
    if (mWidth != width ) {
        mWidth = width;
        setChanged();
    }
}

void SynGutter::autoSizeDigitCount(int linesCount)
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

QString SynGutter::formatLineNumber(int line)
{
    line += (mLineNumberStart - 1);
    QString result = QString::number(line);
    return QString(mAutoSizeDigitCount - result.length(),'0') + result;
}

int SynGutter::realGutterWidth(int charWidth)
{
    if (!mVisible) {
        return 0;
    }
    if (mShowLineNumbers) {
        return mLeftOffset + mRightOffset + mAutoSizeDigitCount * charWidth + 2;
    }
    return mWidth;
}

bool SynGutter::visible() const
{
    return mVisible;
}

void SynGutter::setVisible(bool visible)
{
    if (mVisible!=visible) {
        mVisible = visible;
        setChanged();
    }
}

bool SynGutter::useFontStyle() const
{
    return mUseFontStyle;
}

void SynGutter::setUseFontStyle(bool useFontStyle)
{
    if (mUseFontStyle!=useFontStyle) {
        mUseFontStyle = useFontStyle;
        setChanged();
    }
}

bool SynGutter::showLineNumbers() const
{
    return mShowLineNumbers;
}

void SynGutter::setShowLineNumbers(bool showLineNumbers)
{
    if (mShowLineNumbers!=showLineNumbers) {
        mShowLineNumbers = showLineNumbers;
        setChanged();
    }
}

int SynGutter::rightOffset() const
{
    return mRightOffset;
}

void SynGutter::setRightOffset(int rightOffset)
{
    int value = std::max(0, rightOffset);
    if (mRightOffset != value) {
        mRightOffset = value;
        setChanged();
    }
}

int SynGutter::lineNumberStart() const
{
    return mLineNumberStart;
}

void SynGutter::setLineNumberStart(int lineNumberStart)
{
    int value = std::max(0,lineNumberStart);
    if (mLineNumberStart!=value) {
        mLineNumberStart = value;
        setChanged();
    }
}

bool SynGutter::zeroStart()
{
    return mLineNumberStart == 0;
}

int SynGutter::leftOffset() const
{
    return mLeftOffset;
}

void SynGutter::setLeftOffset(int leftOffset)
{
    int value = std::max(0,leftOffset);
    if (mLeftOffset != value) {
        mLeftOffset = value;
        setChanged();
    }
}

bool SynGutter::leadingZeros() const
{
    return mLeadingZeros;
}

void SynGutter::setLeadingZeros(bool value)
{
    if (mLeadingZeros!=value) {
        mLeadingZeros = value;
        setChanged();
    }
}

int SynGutter::gradientSteps() const
{
    return mGradientSteps;
}

void SynGutter::setGradientSteps(int value)
{
    if (mGradientSteps!=value) {
        mGradientSteps = value;
        if (mGradientSteps<2)
            mGradientSteps = 2;
        setChanged();
    }
}

QColor SynGutter::gradientEndColor() const
{
    return mGradientEndColor;
}

void SynGutter::setGradientEndColor(const QColor &value)
{
    if (mGradientEndColor!=value) {
        mGradientEndColor = value;
        setChanged();
    }
}

QColor SynGutter::gradientStartColor() const
{
    return mGradientStartColor;
}

void SynGutter::setGradientStartColor(const QColor &value)
{
    if (mGradientStartColor!=value) {
        mGradientStartColor = value;
        setChanged();
    }
}

bool SynGutter::gradient() const
{
    return mGradient;
}

void SynGutter::setGradient(bool value)
{
    if (mGradient!=value){
        mGradient = value;
        setChanged();
    }
}

SynGutterBorderStyle SynGutter::borderStyle() const
{
    return mBorderStyle;
}

void SynGutter::setBorderStyle(const SynGutterBorderStyle &value)
{
    if (mBorderStyle!=value) {
        mBorderStyle = value;
        setChanged();
    }
}

int SynGutter::digitCount() const
{
    return mDigitCount;
}

void SynGutter::setDigitCount(int value)
{
    if (mDigitCount != value ) {
        mDigitCount = value;
        setChanged();
    }
}

QColor SynGutter::color() const
{
    return mColor;
}

void SynGutter::setColor(const QColor &value)
{
    if (mColor!=value) {
        mColor = value;
        setChanged();
    }
}

QColor SynGutter::borderColor() const
{
    return mBorderColor;
}

void SynGutter::setBorderColor(const QColor &value)
{
    if (mBorderColor!=value) {
        mBorderColor = value;
        setChanged();
    }
}

