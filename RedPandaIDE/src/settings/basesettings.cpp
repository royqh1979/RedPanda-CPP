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
#include "basesettings.h"
#include "../utils.h"

SettingsPersistor::SettingsPersistor(const QString &filename):
    mFilename{filename},
    mSettings{filename,QSettings::IniFormat}
{

}

void SettingsPersistor::beginGroup(const QString &group)
{
    mSettings.beginGroup(group);
}

void SettingsPersistor::endGroup()
{
    mSettings.endGroup();
}

void SettingsPersistor::remove(const QString &key)
{
    mSettings.remove(key);
}

void SettingsPersistor::saveValue(const QString &key, const QVariant &value)
{
    mSettings.setValue(key,value);
}

QVariant SettingsPersistor::value(const QString &key, const QVariant &defaultValue)
{
    return mSettings.value(key,defaultValue);
}

QStringList SettingsPersistor::allKeys() const
{
    return mSettings.allKeys();
}

QSettings::Status SettingsPersistor::sync()
{
    mSettings.sync();
    return mSettings.status();
}

const QString &SettingsPersistor::filename() const
{
    return mFilename;
}

BaseSettings::BaseSettings(SettingsPersistor *persistor, const QString &groupName):
    mPersistor{persistor},
    mGroup{groupName}
{
    Q_ASSERT(!groupName.isEmpty());
}

void BaseSettings::beginGroup()
{
    mPersistor->beginGroup(mGroup);
}

void BaseSettings::endGroup()
{
    mPersistor->endGroup();
}

void BaseSettings::remove(const QString &key)
{
    mPersistor->remove(key);
}

void BaseSettings::saveValue(const QString &key, const QVariant &value)
{
    mPersistor->saveValue(key,value);
}

void BaseSettings::saveValue(const QString &key, const QSet<QString> &set)
{
    Q_ASSERT(mPersistor!=nullptr);
    QStringList val;
    foreach(const QString& s,set) {
        val.append(s);
    }
    mPersistor->saveValue(key,val);
}

QVariant BaseSettings::value(const QString &key, const QVariant &defaultValue)
{
    if (mPersistor)
        return mPersistor->value(key,defaultValue);
    else
        return defaultValue;
}

bool BaseSettings::boolValue(const QString &key, bool defaultValue)
{
    return value(key,defaultValue).toBool();
}

QSize BaseSettings::sizeValue(const QString &key, const QSize& size)
{
    return value(key,size).toSize();
}

int BaseSettings::intValue(const QString &key, int defaultValue)
{
    return value(key,defaultValue).toInt();
}

double BaseSettings::doubleValue(const QString &key, double defaultValue)
{
    return value(key,defaultValue).toDouble();
}

unsigned int BaseSettings::uintValue(const QString &key, unsigned int defaultValue)
{
    return value(key,defaultValue).toUInt();
}

QStringList BaseSettings::stringListValue(const QString &key, const QStringList &defaultValue)
{
    return value(key,defaultValue).toStringList();
}

QSet<QString> BaseSettings::stringSetValue(const QString &key)
{
    QStringList lst=value(key,QStringList()).toStringList();
    QSet<QString> result;
    foreach(const QString& s, lst)
        result.insert(s);
    return result;
}

QColor BaseSettings::colorValue(const QString &key, const QColor& defaultValue)
{
    return value(key,defaultValue).value<QColor>();
}

QString BaseSettings::stringValue(const QString &key, const QString& defaultValue)
{
    return value(key,defaultValue).toString();
}

void BaseSettings::save()
{
    Q_ASSERT(mPersistor!=nullptr);
    beginGroup();
    doSave();
    endGroup();
}

void BaseSettings::load()
{
    if (mPersistor)
        beginGroup();
    doLoad();
    if (mPersistor)
        endGroup();
}
