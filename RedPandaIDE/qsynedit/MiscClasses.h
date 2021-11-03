#ifndef MISCCLASSES_H
#define MISCCLASSES_H

#include <QColor>
#include <QFont>
#include <QObject>
#include "Types.h"

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
    SynGutterBorderStyle mBorderStyle;
    bool mUseFontStyle;
    bool mVisible;
    int mAutoSizeDigitCount;
};

using PSynGutter = std::shared_ptr<SynGutter>;

class SynEditMark : public QObject {
    Q_OBJECT
public:
    explicit SynEditMark(QObject* parent = nullptr);
    int Char() const;
    void setChar(int value);

    int image() const;
    void setImage(int image);

    bool visible() const;
    void setVisible(bool visible);

    int bookmarkNum() const;
    void setBookmarkNum(int bookmarkNum);

    bool internalImage() const;
    void setInternalImage(bool internalImage);

    bool isBookmark() const ;

    int line() const;
    void setLine(int line);

signals:
    void changed();
protected:
    int mLine;
    int mChar;
    int mImage;
    bool mVisible;
    bool mInternalImage;
    int mBookmarkNum;

};

using PSynEditMark = std::shared_ptr<SynEditMark>;

using SynEditMarkList = QList<SynEditMark>;

using PSynEditMarkList = std::shared_ptr<SynEditMarkList>;

class SynBookMarkOpt: public QObject {
    Q_OBJECT
public:
    explicit SynBookMarkOpt(QObject* parent=nullptr);
    PSynIconList bookmarkImages() const;
    void setBookmarkImages(const PSynIconList &images);

    bool drawBookmarksFirst() const;
    void setDrawBookmarksFirst(bool drawBookmarksFirst);

    bool enableKeys() const;
    void setEnableKeys(bool enableKeys);

    bool glyphsVisible() const;
    void setGlyphsVisible(bool glyphsVisible);

    int leftMargin() const;
    void setLeftMargin(int leftMargin);

    int xOffset() const;
    void setXOffset(int xOffset);

signals:
    void changed();
private:
    PSynIconList mBookmarkImages;
    bool mDrawBookmarksFirst;
    bool mEnableKeys;
    bool mGlyphsVisible;
    int mLeftMargin;
    int mXOffset;
};

using PSynBookMarkOpt = std::shared_ptr<SynBookMarkOpt>;
#endif // MISCCLASSES_H
