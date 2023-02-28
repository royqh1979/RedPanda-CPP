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
#ifndef SYNSEARCHBASE_H
#define SYNSEARCHBASE_H

#include <QObject>
#include <memory>

namespace QSynedit {

enum SearchOption {
    ssoMatchCase    = 0x0001,
    ssoWholeWord    = 0x0002,
    ssoBackwards    = 0x0004,
    ssoEntireScope  = 0x0008,
    ssoSelectedOnly = 0x0010,
    ssoWrapAround   = 0x0020,
    ssoRegExp       = 0x0080
};

Q_DECLARE_FLAGS(SearchOptions, SearchOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(SearchOptions)

class BaseSearcher : public QObject
{
    Q_OBJECT
public:
    explicit BaseSearcher(QObject *parent = nullptr);
    QString pattern();
    virtual void setPattern(const QString& value);
    virtual int length(int aIndex) = 0;
    virtual int result(int aIndex) = 0;
    virtual int resultCount() = 0;
    virtual int findAll(const QString& text) = 0;
    virtual QString replace(const QString& aOccurrence, const QString& aReplacement) = 0;
    SearchOptions options() const;
    virtual void setOptions(const SearchOptions &options);
protected:
    bool isDelimitChar(const QChar& ch) const;

private:
    QString mPattern;
    SearchOptions mOptions;
};

using PSynSearchBase = std::shared_ptr<BaseSearcher>;

}

#endif // SYNSEARCHBASE_H
