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
#ifndef SYNEXPORTER_H
#define SYNEXPORTER_H

#include <QString>
#include "../SynEdit.h"

namespace QSynedit {
using FormatTokenHandler = std::function<void(PHighlighter syntaxHighlighter, int Line, int column, const QString& token,
    PHighlighterAttribute& attr)>;
class SynExporter
{

public:
    explicit SynExporter(const QByteArray charset);

    /**
     * @brief Clears the output buffer and any internal data that relates to the last
     *       exported text.
     */
    virtual void clear();
    /**
     * @brief Copies the output buffer contents to the clipboard, as the native format
     *       or as text depending on the ExportAsText property.
     */
    void CopyToClipboard();
    /**
     * @brief Exports everything in the strings parameter to the output buffer.
     * @param ALines
     */
    void ExportAll(PDocument ALines);

    /**
     * @brief Exports the given range of the strings parameter to the output buffer.
     * @param ALines
     * @param Start
     * @param Stop
     */
    void ExportRange(PDocument ALines,
                     BufferCoord Start, BufferCoord Stop);
    /**
     * @brief Saves the contents of the output buffer to a file.
     * @param AFileName
     */
    void SaveToFile(const QString& AFileName);
    /**
     * @brief Saves the contents of the output buffer to a stream.
     * @param AStream
     */
    void SaveToStream(QIODevice& AStream);
    bool exportAsText() const;
    void setExportAsText(bool Value);

    QFont font() const;
    void setFont(const QFont &font);

    PHighlighter highlighter() const;
    void setHighlighter(PHighlighter Value);

    QString title() const;
    void setTitle(const QString &Value);

    bool useBackground() const;
    void setUseBackground(bool Value);

    FileEndingType fileEndingType() const;
    void setFileEndingType(const FileEndingType &fileEndingType);

    /**
     * @brief The clipboard format the exporter creates as native format.
     * @return
     */
    virtual QByteArray clipboardFormat();

    QColor foregroundColor() const;
    void setForegroundColor(const QColor &value);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor &value);

    QByteArray charset() const;
    void setCharset(const QByteArray &charset);

    QString defaultFilter() const;
    void setDefaultFilter(const QString &defaultFilter);

    FormatTokenHandler onFormatToken() const;
    void setOnFormatToken(const FormatTokenHandler &onFormatToken);

    const QByteArray& buffer() const;

protected:
    QByteArray mCharset;
    QColor mBackgroundColor;
    QColor mForegroundColor;
    QByteArray mClipboardFormat;
    QString mDefaultFilter;
    bool mExportAsText;
    QFont mFont;
    PHighlighter mHighlighter;
    QColor mLastBG;
    QColor mLastFG;
    FontStyles mLastStyle;
    QMap<QChar,QString> mReplaceReserved;
    QString mTitle;
    bool mUseBackground;
    FileEndingType mFileEndingType;

    QString lineBreak();

    /**
     * @brief Adds a string to the output buffer.
     * @param AText
     */
    void AddData(const QString& AText);

    /**
     * @brief Adds a string and a trailing newline to the output buffer.
     * @param AText
     */
    void AddDataNewLine(const QString& AText);

    /**
     * @brief Adds a newline to the output buffer.
     */
    void AddNewLine();


     /**
     * @brief Copies the data under this format to the clipboard. The clipboard has to
     *   be opened explicitly when more than one format is to be set.
     */
    void CopyToClipboardFormat(QByteArray AFormat);

    /**
     * @brief Has to be overridden in descendant classes to add the closing format
     *  strings to the output buffer.  The parameters can be used to track what
     *    changes are made for the next token.
     * @param BackgroundChanged
     * @param ForegroundChanged
     * @param FontStylesChanged
     */
    virtual void FormatAttributeDone(bool BackgroundChanged, bool ForegroundChanged,
                             FontStyles  FontStylesChanged) = 0;

    /**
     * @brief Has to be overridden in descendant classes to add the opening format
     *   strings to the output buffer.  The parameters can be used to track what
     *   changes have been made in respect to the previous token.
     * @param BackgroundChanged
     * @param ForegroundChanged
     * @param FontStylesChanged
     */
    virtual void FormatAttributeInit(bool BackgroundChanged, bool ForegroundChanged,
                                     FontStyles  FontStylesChanged) = 0;
    /**
     * @brief Has to be overridden in descendant classes to add the closing format
     *   strings to the output buffer after the last token has been written.
     */
    virtual void FormatAfterLastAttribute() = 0;

    /**
     * @brief Has to be overridden in descendant classes to add the opening format
     *   strings to the output buffer when the first token is about to be written.
     * @param BackgroundChanged
     * @param ForegroundChanged
     * @param FontStylesChanged
     */
    virtual void FormatBeforeFirstAttribute(bool BackgroundChanged, bool ForegroundChanged,
                                            FontStyles  FontStylesChanged) = 0;
    /**
     * @brief Can be overridden in descendant classes to add the formatted text of
     *   the actual token text to the output buffer.
     */
    virtual void FormatToken(const QString& Token) ;
    /**
     * @brief Has to be overridden in descendant classes to add a newline in the output
     *   format to the output buffer.
     */
    virtual void FormatNewLine() = 0;
    /**
     * @brief Returns the size of the formatted text in the output buffer, to be used
     *   in the format header or footer.
     * @return
     */
    int GetBufferSize();
    /**
     * @brief Has to be overridden in descendant classes to return the correct output
     *   format footer.
     * @return
     */
    virtual QString GetFooter() = 0;
    /**
     * @brief Has to be overridden in descendant classes to return the name of the
     *   output format.
     * @return
     */
    virtual QString GetFormatName() = 0;
    /**
     * @brief Has to be overridden in descendant classes to return the correct output
     *   format header.
     * @return
     */
    virtual QString GetHeader() = 0;
    /**
     * @brief Inserts a data block at the given position into the output buffer.  Is
     *   used to insert the format header after the exporting, since some header
     *     data may be known only after the conversion is done.
     * @param APos
     * @param AText
     */
    void InsertData(int APos, const QString& AText);
    /**
     * @brief Returns a string that has all the invalid chars of the output format
     *   replaced with the entries in the replacement array.
     */
    QString ReplaceReservedChars(const QString &AToken);
    /**
     * @brief Sets the token attribute of the next token to determine the changes
     *   of colors and font styles so the properties of the next token can be
     *     added to the output buffer.
     * @param Attri
     */
    virtual void SetTokenAttribute(PHighlighterAttribute Attri);

    QTextCodec *getCodec();
private:
    QByteArray mBuffer;
    bool mFirstAttribute;
    FormatTokenHandler mOnFormatToken;

};
}

#endif // SYNEXPORTER_H
