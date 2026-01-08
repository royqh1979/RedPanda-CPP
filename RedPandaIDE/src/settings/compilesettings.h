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
#ifndef COMPILE_SETTINGS_H
#define COMPILE_SETTINGS_H
#include "basesettings.h"

#define SETTING_COMPILE "Compile"

class CompileSettings: public BaseSettings {
public:
    explicit CompileSettings(SettingsPersistor *persistor);
    const QString &NASMPath() const;
    void setNASMPath(const QString &newNASMPath);
    bool NASMLinkCStandardLib() const;
    void setNASMLinkCStandardLib(bool newLinkCStandardLib);

    bool GASLinkCStandardLib() const;
    void setGASLinkCStandardLib(bool newGASLinkCStandardLib);

private:
    QString mNASMPath;
    bool mNASMLinkCStandardLib;
    bool mGASLinkCStandardLib;
    // _Base interface
protected:
    void doSave() override;
    void doLoad() override;

};


#endif
//COMPILE_SETTINGS_H
