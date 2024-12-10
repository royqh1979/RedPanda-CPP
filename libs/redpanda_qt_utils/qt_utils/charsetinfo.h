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
#ifndef QT_UTILS_CHARSETINFO_H
#define QT_UTILS_CHARSETINFO_H
#include <QByteArray>
#include <QString>
#include <memory>
#include <QObject>

struct CharsetInfo{
    int codepage;
    QByteArray name;
    QString language;
    QString localeName;
    bool enabled;
    explicit CharsetInfo(int codepage,
                         const QByteArray& name,
                         const QString& language,
                         const QString& locale,
                         bool enabled);
};

using PCharsetInfo = std::shared_ptr<CharsetInfo>;

class CharsetInfoManager: public QObject {
    Q_OBJECT
public:
    explicit CharsetInfoManager(const QString& localeName);

    explicit CharsetInfoManager(CharsetInfoManager&) = delete;
    CharsetInfoManager& operator=(CharsetInfoManager&) = delete;

    QByteArray getDefaultSystemEncoding();
    QByteArray getDefaultConsoleEncoding();
    PCharsetInfo findCharsetByCodepage(int codepage);
    QStringList languageNames();
    QList<PCharsetInfo> findCharsetsByLanguageName(const QString& languageName);
    QList<PCharsetInfo> findCharsetByLocale(const QString& localeName);
    QString findLanguageByCharsetName(const QString& encodingName);
    const QString &localeName() const;

private:
    QList<PCharsetInfo> mCodePages;
    QString mLocaleName;
};

using PCharsetInfoManager = std::shared_ptr<CharsetInfo>;

extern CharsetInfoManager* pCharsetInfoManager;

#endif // PLATFORM_H
