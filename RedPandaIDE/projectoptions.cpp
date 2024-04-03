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
#include "projectoptions.h"
#include "utils.h"

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
    fileDescription = "Developed using the Red Panda C++ IDE";
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
    version = 3;
    isCpp = false;
    logOutput = false;
    useCustomMakefile = false;
    usePrecompiledHeader = false;
    useCustomOutputFilename = false;
    includeVersionInfo = false;
    supportXPThemes = false;
    compilerSet = 0;
    staticLink = true;
    addCharset = true;
    modelType = ProjectModelType::FileSystem;
    classBrowserType = ProjectClassBrowserType::CurrentFile;
    execEncoding = ENCODING_SYSTEM_DEFAULT;
    allowParallelBuilding=false;
    parellelBuildingJobs=0;
}
