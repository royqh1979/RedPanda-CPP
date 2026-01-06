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
