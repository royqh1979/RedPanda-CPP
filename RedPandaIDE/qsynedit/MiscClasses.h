#ifndef MISCCLASSES_H
#define MISCCLASSES_H

#include <QColor>
#include <QFont>
#include <QObject>

enum class SynGutterBorderStyle {
    None,
    Middle,
    Right
};

class SynGutter : public QObject {
    Q_OBJECT
public:
    explicit SynGutter(QObject* parent = nullptr);
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

    SynGutterBorderStyle borderStyle() const;
    void setBorderStyle(const SynGutterBorderStyle &value);

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

    int width() const;
    void setWidth(int width);

    void autoSizeDigitCount(int linesCount);
    QString formatLineNumber(int line);
    int realGutterWidth(int charWidth);

signals:
    void changed();
private:
    void setChanged();
private:
    bool mAutoSize;
    QColor mBorderColor;
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
    SynGutterBorderStyle mBorderStyle;
    bool mUseFontStyle;
    bool mVisible;
    int mWidth;
    int mAutoSizeDigitCount;

};

#endif // MISCCLASSES_H
