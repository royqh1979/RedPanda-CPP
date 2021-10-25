#include "projectoptions.h"

ProjectVersionInfo::ProjectVersionInfo()
{
    major = 1;
    minor = 0;
    release = 0;
    build = 0;
    languageID = 0x0409; // US English
    charsetID = 0x04E4; // Windows multilingual
    companyName = "";
    fileVersion = "";
    fileDescription = "Developed using the Red Panda Dev-C++ IDE";
    internalName = "";
    legalCopyright = "";
    legalTrademarks = "";
    originalFilename = "";
    productName = "";
    productVersion = "";
    autoIncBuildNr = false;
    syncProduct = true;
}

ProjectOptions::ProjectOptions()
{
    type = ProjectType::GUI;
    version = 2;
    useGPP = false;
    logOutputEnabled = false;
    useCustomMakefile = false;
    usePrecompiledHeader = false;
    overrideOutput = false;
    includeVersionInfo = false;
    supportXPThemes = false;
    compilerSet = 0;
    compilerSetType = 0;
    staticLink = true;
    addCharset = true;
}
