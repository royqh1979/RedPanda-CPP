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
#ifndef PROJECTOPTIONS_H
#define PROJECTOPTIONS_H

#include <QMap>
#include <QWidget>
#include "compiler/compilerinfo.h"

enum class ProjectModelType {
    FileSystem,
    Custom
};

enum class ProjectClassBrowserType {
    CurrentFile,
    WholeProject
};

enum class ProjectType {
    GUI=0,
    Console=1,
    StaticLib=2,
    DynamicLib=3,
    MicroController=4,
};

struct ProjectVersionInfo{
    explicit ProjectVersionInfo();
    int major;
    int minor;
    int release;
    int build;
    int languageID;
    int charsetID;
    QString companyName;
    QString fileVersion;
    QString fileDescription;
    QString internalName;
    QString legalCopyright;
    QString legalTrademarks;
    QString originalFilename;
    QString productName;
    QString productVersion;
    bool autoIncBuildNr;
    bool syncProduct;
};

struct ProjectOptions{
    explicit ProjectOptions();
    ProjectType type;
    int version;
    QString compilerCmd;
    QString cppCompilerCmd;
    QString linkerCmd;
    QString resourceCmd;
    QStringList binDirs;
    QStringList includeDirs;
    QStringList libDirs;
    QString privateResource;
    QStringList resourceIncludes;
    QStringList makeIncludes;
    bool isCpp;
    QString icon;
    QString folderForOutput;
    QString folderForObjFiles;
    QString logFilename;
    bool logOutput;
    bool useCustomMakefile;
    QString customMakefile;
    bool usePrecompiledHeader;
    QString precompiledHeader;
    bool useCustomOutputFilename;
    QString customOutputFilename;
    QString hostApplication;
    bool includeVersionInfo;
    bool supportXPThemes;
    int compilerSet;
    QMap<QString,QString> compilerOptions;
    ProjectVersionInfo versionInfo;
    QString cmdLineArgs;
    bool staticLink;
    bool addCharset;
    QByteArray execEncoding;
    QByteArray encoding;
    ProjectModelType modelType;
    ProjectClassBrowserType classBrowserType;
    bool allowParallelBuilding;
    int parellelBuildingJobs;
};
#endif // PROJECTOPTIONS_H
