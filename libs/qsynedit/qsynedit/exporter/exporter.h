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
#ifndef EXPORTER_H
#define EXPORTER_H

#include <QString>
#include <memory>
#include <functional>
#include <QIODevice>
#include <QFont>
#include <QColor>
#include <QMap>
#include "qt_utils/utils.h"
#include "../types.h"


namespace QSynedit {
class Document;
using PDocument = std::shared_ptr<Document>;
class Syntaxer;
using PSyntaxer = std::shared_ptr<Syntaxer>;
class TokenAttribute;
using PTokenAttribute = std::shared_ptr<TokenAttribute>;

using FormatTokenHandler = std::function<void(PSyntaxer syntaxHighlighter, int line, int column, const QString& token,
    PTokenAttribute& attr)>;
class Exporter
{

public:
    explicit Exporter(int tabSize, const QByteArray charset);
    Exporter(const Exporter&)=delete;
    Exporter& operator=(const Exporter&)=delete;

    /**
     * @brief Clears the output buffer and any internal data that relates to the last
     *       exported text.
     */
    virtual void clear();
    /**
     * @brief Exports everything in the strings parameter to the output buffer.
     * @param doc
     */
    void exportAll(const PDocument& doc);

    /**
     * @brief Exports the given range of the strings parameter to the output buffer.
     * @param doc
     * @param start
     * @param stop
     */
    void exportRange(const PDocument& doc,
                     BufferCoord start, BufferCoord stop);
    /**
     * @brief Saves the contents of the output buffer to a file.
     * @param AFileName
     */
    void saveToFile(const QString& filename);
    /**
     * @brief Saves the contents of the output buffer to a stream.
     * @param stream
     */
    void writeToStream(QIODevice& stream);

    QFont font() const;
    void setFont(const QFont &font);

    PSyntaxer syntaxer() const;
    void setSyntaxer(PSyntaxer value);

    QString title() const;
    void setTitle(const QString &Value);

    bool useBackground() const;
    void setUseBackground(bool Value);

    NewlineType fileEndingType() const;
    void setFileEndingType(const NewlineType &fileEndingType);

    /**
     * @brief The clipboard format the exporter creates as native format.
     * @return
     */
    QByteArray clipboardFormat();

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

    QByteArray buffer() const;
    const QString& text() const;

    bool exportLineNumber() const;
    void setExportLineNumber(bool newExportLineNumber);

    bool recalcLineNumber() const;
    void setRecalcLineNumber(bool newRecalcLineNumber);

    bool lineNumberStartFromZero() const;
    void setLineNumberStartFromZero(bool newLineNumberStartFromZero);

    QColor lineNumberColor() const;
    void setLineNumberColor(const QColor &newLineNumberColor);

    QColor lineNumberBackgroundColor() const;
    void setLineNumberBackgroundColor(const QColor &newLineNumberBackgroundColor);

protected:
    int mTabSize;
    QByteArray mClipboardFormat;
    QByteArray mCharset;
    QFont mFont;
    QColor mBackgroundColor;
    QColor mForegroundColor;
    QString mDefaultFilter;
    PSyntaxer mSyntaxer;
    QColor mLastBG;
    QColor mLastFG;
    FontStyles mLastStyle;
    QMap<QChar,QString> mReplaceReserved;
    QString mTitle;
    bool mUseBackground;
    NewlineType mFileEndingType;
    bool mExportLineNumber;
    bool mRecalcLineNumber;
    bool mLineNumberStartFromZero;
    QColor mLineNumberColor;
    QColor mLineNumberBackgroundColor;

    QString lineBreak();

    /**
     * @brief Adds a string to the output buffer.
     * @param text
     */
    void addData(const QString& text);

    /**
     * @brief Adds a string and a trailing newline to the output buffer.
     * @param text
     */
    void addDataNewLine(const QString& text);

    /**
     * @brief Adds a newline to the output buffer.
     */
    void addNewLine();

    /**
     * @brief Has to be overridden in descendant classes to add the closing format
     *  strings to the output buffer.  The parameters can be used to track what
     *    changes are made for the next token.
     * @param backgroundChanged
     * @param foregroundChanged
     * @param fontStyles
     */
    virtual void formatAttributeDone(bool backgroundChanged, bool foregroundChanged,
                             FontStyles  fontStyles) = 0;

    /**
     * @brief Has to be overridden in descendant classes to add the opening format
     *   strings to the output buffer.  The parameters can be used to track what
     *   changes have been made in respect to the previous token.
     * @param backgroundChanged
     * @param foregroundChanged
     * @param fontStyles
     */
    virtual void formatAttributeInit(bool backgroundChanged, bool foregroundChanged,
                                     FontStyles  fontStyles) = 0;
    /**
     * @brief Has to be overridden in descendant classes to add the closing format
     *   strings to the output buffer after the last token has been written.
     */
    virtual void formatAfterLastAttribute() = 0;

    /**
     * @brief Has to be overridden in descendant classes to add the opening format
     *   strings to the output buffer when the first token is about to be written.
     * @param backgroundChanged
     * @param foregroundChanged
     * @param fontStyles
     */
    virtual void formatBeforeFirstAttribute(bool backgroundChanged, bool foregroundChanged,
                                            FontStyles  fontStyles) = 0;
    /**
     * @brief Can be overridden in descendant classes to add the formatted text of
     *   the actual token text to the output buffer.
     */
    virtual void formatToken(const QString& token) ;
    /**
     * @brief Has to be overridden in descendant classes to add a newline in the output
     *   format to the output buffer.
     */
    virtual void formatNewLine() = 0;
    /**
     * @brief Returns the size of the formatted text in the output buffer, to be used
     *   in the format header or footer.
     * @return
     */
    int getBufferSize() const;
    /**
     * @brief Has to be overridden in descendant classes to return the correct output
     *   format footer.
     * @return
     */
    virtual QString getFooter() = 0;
    /**
     * @brief Has to be overridden in descendant classes to return the name of the
     *   output format.
     * @return
     */
    virtual QString getFormatName() = 0;
    /**
     * @brief Has to be overridden in descendant classes to return the correct output
     *   format header.
     * @return
     */
    virtual QString getHeader() = 0;
    /**
     * @brief Inserts a data block at the given position into the output buffer.  Is
     *   used to insert the format header after the exporting, since some header
     *     data may be known only after the conversion is done.
     * @param pos
     * @param text
     */
    void insertData(int pos, const QString& text);
    /**
     * @brief Returns a string that has all the invalid chars of the output format
     *   replaced with the entries in the replacement array.
     */
    QString replaceReservedChars(const QString &token);
    /**
     * @brief Sets the token attribute of the next token to determine the changes
     *   of colors and font styles so the properties of the next token can be
     *     added to the output buffer.
     * @param attri
     */
    virtual void setTokenAttribute(PTokenAttribute attri);

    virtual QString getStartLineNumberString(int startLine, int endLine);
    virtual QString getLineNumberString(int line);
    virtual QString getEndLineNumberString(int startLine, int endLine);


    TextEncoder getEncoder() const;
private:
    QString mText;
    bool mFirstAttribute;
    FormatTokenHandler mOnFormatToken;

};
}

#endif // EXPORTER_H
