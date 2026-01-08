/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#ifndef BASE_SETTINGS_H
#define BASE_SETTINGS_H
#include <QString>
#include <QSettings>
#include <QSize>
#include <QColor>

class SettingsPersistor {
public:
    explicit SettingsPersistor(const QString& filename);
    explicit SettingsPersistor(SettingsPersistor&& settings) = delete;
    explicit SettingsPersistor(const SettingsPersistor& settings) = delete;

    SettingsPersistor& operator= (const SettingsPersistor& settings) = delete;
    SettingsPersistor& operator= (const SettingsPersistor&& settings) = delete;
    void beginGroup(const QString& group);
    void endGroup();
    void remove(const QString &key);
    void saveValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant& defaultValue = QVariant());
    QStringList allKeys() const;
    QSettings::Status sync();
    const QString &filename() const;

private:
    QString mFilename;
    QSettings mSettings;
};

class BaseSettings {
public:
    explicit BaseSettings(SettingsPersistor* persistor, const QString& groupName);
    void beginGroup();
    void endGroup();
    void remove(const QString &key);
    void saveValue(const QString &key, const QVariant &value);
    void saveValue(const QString &key, const QSet<QString>& set);
    QVariant value(const QString &key, const QVariant& defaultValue);
    bool boolValue(const QString &key, bool defaultValue);
    QSize sizeValue(const QString &key, const QSize& size=QSize());
    int intValue(const QString &key, int defaultValue);
    double doubleValue(const QString& key, double defaultValue);
    unsigned int uintValue(const QString &key, unsigned int defaultValue);
    QStringList stringListValue(const QString &key, const QStringList& defaultValue=QStringList());
    QSet<QString> stringSetValue(const QString &key);
    QColor colorValue(const QString &key, const QColor& defaultValue);
    QString stringValue(const QString &key, const QString& defaultValue);
    void save();
    void load();

protected:
    virtual void doSave() = 0;
    virtual void doLoad() = 0;
protected:
    SettingsPersistor* mPersistor;
    QString mGroup;
};

#endif
//BASE_SETTINGS_H
