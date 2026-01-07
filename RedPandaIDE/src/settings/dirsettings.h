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
#ifndef DIR_SETTINGS_H
#define DIR_SETTINGS_H
#include "basesettings.h"

#define SETTING_DIRS "Dirs"

class DirSettings: public BaseSettings {
public:
    enum class DataType {
        None,
        ColorScheme,
        IconSet,
        Theme,
        Template
    };
    explicit DirSettings(SettingsPersistor * persistor);
    QString appDir() const;
    QString appResourceDir() const;
    QString appLibexecDir() const;
    QString projectDir() const;
    QString data(DataType dataType = DataType::None) const;
    QString config(DataType dataType = DataType::None) const;
    QString executable() const;

    void setProjectDir(const QString &newProjectDir);

protected:
    void doSave() override;
    void doLoad() override;
private:
    QString mProjectDir;
};


#endif
//DIR_SETTINGS_H
