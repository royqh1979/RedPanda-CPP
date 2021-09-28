#include "synexporter.h"

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QMimeData>
#include <QTextCodec>
#include "../../platform.h"

SynExporter::SynExporter()
{
    mClipboardFormat = "text/plain";
    mFont = QGuiApplication::font();
    mBackgroundColor = QGuiApplication::palette().color(QPalette::Base);
    mForegroundColor = QGuiApplication::palette().color(QPalette::Text);
    mUseBackground = false;
    mExportAsText = false;
    mCharset = pCharsetInfoManager->getDefaultSystemEncoding();
    mFileEndingType = FileEndingType::Windows;
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

void SynExporter::ExportAll(PSynEditStringList ALines)
{
    ExportRange(ALines, BufferCoord{1, 1}, BufferCoord{INT_MAX, INT_MAX});
}

void SynExporter::ExportRange(PSynEditStringList ALines, BufferCoord Start, BufferCoord Stop)
{
    // abort if not all necessary conditions are met
    if (!ALines || !mHighlighter || (ALines->count() == 0))
        return;
    Stop.Line = std::max(1, std::min(Stop.Line, ALines->count()));
    Stop.Char = std::max(1, std::min(Stop.Char, ALines->getString(Stop.Line - 1).length() + 1));
    Start.Line = std::max(1, std::min(Start.Line, ALines->count()));
    Start.Char = std::max(1, std::min(Start.Char, ALines->getString(Start.Line - 1).length() + 1));
    if ( (Start.Line > ALines->count()) || (Start.Line > Stop.Line) )
        return;
    if ((Start.Line == Stop.Line) && (Start.Char >= Stop.Char))
        return;
    // initialization
    mBuffer.clear();
    // export all the lines into fBuffer
    mFirstAttribute = true;

    if (Start.Line == 1)
        mHighlighter->resetState();
    else
        mHighlighter->setState(ALines->ranges(Start.Line-2));
    for (int i = Start.Line; i<=Stop.Line; i++) {
        QString Line = ALines->getString(i-1);
        // order is important, since Start.Y might be equal to Stop.Y
//        if (i == Stop.Line)
//            Line.remove(Stop.Char-1, INT_MAX);
//        if ( (i = Start.Line) && (Start.Char > 1))
//            Line.remove(0, Start.Char - 1);
        // export the line
        mHighlighter->setLine(Line, i);
        while (!mHighlighter->eol()) {
            PSynHighlighterAttribute attri = mHighlighter->getTokenAttribute();
            int startPos = mHighlighter->getTokenPos();
            QString token = mHighlighter->getToken();
            if (i==Start.Line && (startPos+token.length() < Start.Char)) {
                mHighlighter->next();
                continue;
            }
            if (i==Stop.Line && (startPos >= Stop.Char-1)) {
                mHighlighter->next();
                continue;
            }
            if (i==Stop.Line && (startPos+token.length() > Stop.Char)) {
                token = token.remove(Stop.Char - startPos - 1);
            }
            if (i==Start.Line && startPos < Start.Char-1) {
                token = token.mid(Start.Char-1-startPos);
            }

            QString Token = ReplaceReservedChars(token);
            if (mOnFormatToken)
                mOnFormatToken(i, mHighlighter->getTokenPos(), mHighlighter->getToken(),attri);
            SetTokenAttribute(attri);
            FormatToken(Token);
            mHighlighter->next();
        }
        FormatNewLine();
    }
    if (!mFirstAttribute)
        FormatAfterLastAttribute();
    // insert header
    InsertData(0, GetHeader());
    // add footer
    AddData(GetFooter());
}

void SynExporter::SaveToFile(const QString &AFileName)
{
    QFile file(AFileName);
    if (file.open(QIODevice::WriteOnly)) {
        SaveToStream(file);
        file.close();
    }
}

void SynExporter::SaveToStream(QIODevice &AStream)
{
    AStream.write(mBuffer);
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

QString SynExporter::defaultFilter() const
{
    return mDefaultFilter;
}

void SynExporter::setDefaultFilter(const QString &defaultFilter)
{
    mDefaultFilter = defaultFilter;
}

void SynExporter::AddData(const QString &AText)
{
    if (!AText.isEmpty()) {
        QTextCodec* codec = getCodec();
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
    AddData(lineBreak());
}

void SynExporter::CopyToClipboardFormat(QByteArray AFormat)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    QMimeData * mimeData = new QMimeData();
    mimeData->setData(AFormat,mBuffer);
    clipboard->clear();
    clipboard->setMimeData(mimeData);
}

void SynExporter::FormatToken(const QString &Token)
{
    AddData(Token);
}

int SynExporter::GetBufferSize()
{
    return mBuffer.size();
}

QTextCodec * SynExporter::getCodec() {
    QTextCodec* codec = QTextCodec::codecForName(mCharset);
    if (codec == nullptr)
        codec = QTextCodec::codecForLocale();
    return codec;
}
void SynExporter::InsertData(int APos, const QString &AText)
{
    if (!AText.isEmpty()) {
        QTextCodec* codec = getCodec();
        mBuffer.insert(APos,codec->fromUnicode(AText));
    }
}

QString SynExporter::ReplaceReservedChars(const QString &AToken)
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

const QByteArray &SynExporter::buffer() const
{
    return mBuffer;
}

QByteArray SynExporter::clipboardFormat()
{
    return this->mClipboardFormat;
}

FormatTokenHandler SynExporter::onFormatToken() const
{
    return mOnFormatToken;
}

void SynExporter::setOnFormatToken(const FormatTokenHandler &onFormatToken)
{
    mOnFormatToken = onFormatToken;
}

QString SynExporter::lineBreak()
{
    switch(mFileEndingType) {
    case FileEndingType::Linux:
        return "\n";
    case FileEndingType::Windows:
        return "\r\n";
    case FileEndingType::Mac:
        return "\r";
    }
    return "\n";
}
