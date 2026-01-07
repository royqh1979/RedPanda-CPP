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
#include "compilesettings.h"

CompileSettings::CompileSettings(SettingsPersistor *persistor):
    BaseSettings(persistor, SETTING_COMPILE)
{

}

const QString &CompileSettings::NASMPath() const
{
    return mNASMPath;
}

void CompileSettings::setNASMPath(const QString &newNASMPath)
{
    mNASMPath = newNASMPath;
}

bool CompileSettings::GASLinkCStandardLib() const
{
    return mGASLinkCStandardLib;
}

void CompileSettings::setGASLinkCStandardLib(bool newGASLinkCStandardLib)
{
    mGASLinkCStandardLib = newGASLinkCStandardLib;
}

bool CompileSettings::NASMLinkCStandardLib() const
{
    return mNASMLinkCStandardLib;
}

void CompileSettings::setNASMLinkCStandardLib(bool newLinkCStandardLib)
{
    mNASMLinkCStandardLib = newLinkCStandardLib;
}

void CompileSettings::doSave()
{
    saveValue("NASM", mNASMPath);
    saveValue("NASM_Link_C_STANDARD_LIB", mNASMLinkCStandardLib);
    saveValue("GAS_Link_C_STANDARD_LIB", mGASLinkCStandardLib);
}


void CompileSettings::doLoad()
{
    mNASMPath = stringValue("NASM", "");
    mNASMLinkCStandardLib = boolValue("NASM_Link_C_STANDARD_LIB", true);
    mGASLinkCStandardLib = boolValue("GAS_Link_C_STANDARD_LIB", true);
}
