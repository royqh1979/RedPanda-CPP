#include "synexporter.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QTextCodec>

SynExporter::SynExporter()
{
    mClipboardFormat = "text/plain";
    mFont = QGuiApplication::font();
    mBackgroundColor = QGuiApplication::palette().color(QPalette::Base);
    mForegroundColor = QGuiApplication::palette().color(QPalette::Text);
    mUseBackground = false;
    mExportAsText = false;
    clear();
    setTitle("");
}

void SynExporter::clear()
{
    mBuffer.clear();
    mLastStyle = SynFontStyle::fsNone;
    mLastBG = QGuiApplication::palette().color(QPalette::Base);
    mLastFG = QGuiApplication::palette().color(QPalette::Text);
}

void SynExporter::CopyToClipboard()
{
    if (mExportAsText) {
      CopyToClipboardFormat("text/plain");
    } else
      CopyToClipboardFormat(clipboardFormat());
}

bool SynExporter::exportAsText() const
{
    return mExportAsText;
}

void SynExporter::setExportAsText(bool Value)
{
    if (mExportAsText != Value) {
        mExportAsText = Value;
        clear();
    }
}

QFont SynExporter::font() const
{
    return mFont;
}

void SynExporter::setFont(const QFont &font)
{
    mFont = font;
}

PSynHighlighter SynExporter::highlighter() const
{
    return mHighlighter;
}

void SynExporter::setHighlighter(PSynHighlighter Value)
{
    if (mHighlighter != Value) {
        mHighlighter = Value;
        clear();
        if ((mHighlighter) && (mHighlighter->whitespaceAttribute()) && mUseBackground)
            mBackgroundColor = mHighlighter->whitespaceAttribute()->background();
    }
}

QString SynExporter::title() const
{
    return mTitle;
}

void SynExporter::setTitle(const QString &Value)
{
    if (mTitle != Value) {
      if (!Value.isEmpty())
          mTitle = Value;
      else
          mTitle = QObject::tr("Untitled");
    }
}

bool SynExporter::useBackground() const
{
    return mUseBackground;
}

void SynExporter::setUseBackground(bool Value)
{
    if (mUseBackground != Value) {
        mUseBackground = Value;
        clear();
        if ((mHighlighter) && (mHighlighter->whitespaceAttribute()) && mUseBackground)
            mBackgroundColor = mHighlighter->whitespaceAttribute()->background();
    }
}

FileEndingType SynExporter::fileEndingType() const
{
    return mFileEndingType;
}

void SynExporter::setFileEndingType(const FileEndingType &fileEndingType)
{
    mFileEndingType = fileEndingType;
}

QColor SynExporter::foregroundColor() const
{
    return mForegroundColor;
}

void SynExporter::setForegroundColor(const QColor &value)
{
    if (mForegroundColor != value) {
        mForegroundColor = value;
    }
}

QColor SynExporter::backgroundColor() const
{
    return mBackgroundColor;
}

void SynExporter::setBackgroundColor(const QColor &value)
{
    if (mBackgroundColor != value) {
        mBackgroundColor = value;
    }
}

QByteArray SynExporter::charset() const
{
    return mCharset;
}

void SynExporter::setCharset(const QByteArray &charset)
{
    mCharset = charset;
}

void SynExporter::AddData(const QString &AText)
{
    if (!AText.isEmpty()) {
        QTextCodec* codec = QTextCodec::codecForName(mCharset);
        mBuffer.append(codec->fromUnicode(AText));
    }
}

void SynExporter::AddDataNewLine(const QString &AText)
{
    AddData(AText);
    AddNewLine();
}

void SynExporter::AddNewLine()
{
    switch(mFileEndingType) {
    case FileEndingType::Linux:
        AddData("\n");
    case FileEndingType::Windows:
        AddData("\r\n");
    case FileEndingType::Mac:
        AddData("\r");
    }
}

void SynExporter::CopyToClipboardFormat(QByteArray AFormat)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    QMimeData * mimeData = new QMimeData();
    mimeData->setData(AFormat,mBuffer);
    clipboard->setMimeData(mimeData);
}

void SynExporter::FormatToken(QString &Token)
{
    AddData(Token);
}

int SynExporter::GetBufferSize()
{
    return mBuffer.size();
}

void SynExporter::InsertData(int APos, const QString &AText)
{
    if (!AText.isEmpty()) {
        QTextCodec* codec = QTextCodec::codecForName(mCharset);
        mBuffer.insert(APos,codec->fromUnicode(AText));
    }
}

QString SynExporter::ReplaceReservedChars(QString &AToken)
{
    if (AToken.isEmpty())
        return "";
    QString result;
    for (QChar ch:AToken) {
        if (mReplaceReserved.contains(ch)) {
            result += mReplaceReserved[ch];
        } else {
            result += ch;
        }
    }
    return result;
}

static QColor ValidatedColor(const QColor& color, const QColor& defaultColor) {
    if (color.isValid())
        return color;
    else
        return defaultColor;
}
void SynExporter::SetTokenAttribute(PSynHighlighterAttribute Attri)
{
    if (mFirstAttribute) {
        mFirstAttribute = false;
        mLastBG = ValidatedColor(Attri->background(), mBackgroundColor);
        mLastFG = ValidatedColor(Attri->foreground(), mForegroundColor);
        mLastStyle = Attri->styles();
        FormatBeforeFirstAttribute(
                    mUseBackground && (mLastBG != mBackgroundColor),
                    mLastFG != mForegroundColor, Attri->styles());
    } else {
        bool ChangedBG = mUseBackground &&
          (mLastBG != ValidatedColor(Attri->background(), mBackgroundColor));
        bool ChangedFG = (mLastFG != ValidatedColor(Attri->foreground(), mForegroundColor));
        if (ChangedBG || ChangedFG ||  (mLastStyle != Attri->styles())) {
            // which font style bits are to reset?
            SynFontStyles ChangedStyles = mLastStyle & ~(Attri->styles());
            FormatAttributeDone(ChangedBG, ChangedFG, ChangedStyles);
            // which font style bits are to set?
            ChangedStyles = Attri->styles() & ~(mLastStyle);
            mLastBG = ValidatedColor(Attri->background(), mBackgroundColor);
            mLastFG = ValidatedColor(Attri->foreground(), mForegroundColor);
            mLastStyle = Attri->styles();
            FormatAttributeInit(ChangedBG, ChangedFG, ChangedStyles);
        }
    }
}

QByteArray SynExporter::clipboardFormat()
{
    return this->mClipboardFormat;
}
