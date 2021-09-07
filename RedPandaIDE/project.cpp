#include "project.h"
#include "editor.h"
#include "mainwindow.h"
#include "utils.h"
#include "systemconsts.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

Project::Project(QObject *parent) : QObject(parent)
{

}

QString Project::directory()
{
    QFileInfo fileInfo(mFilename);
    return fileInfo.absolutePath();
}

QString Project::executableName()
{
    QString exeFileName;
    if (mOptions.overrideOutput && !mOptions.overridenOutput.isEmpty()) {
        exeFileName = mOptions.overridenOutput;
    } else {
        switch(mOptions.type) {
        case ProjectType::StaticLib:
            exeFileName = changeFileExt(baseFileName(mFilename),STATIC_LIB_EXT);
            break;
        case ProjectType::DynamicLib:
            exeFileName = changeFileExt(baseFileName(mFilename),DYNAMIC_LIB_EXT);
            break;
        default:
            exeFileName = changeFileExt(baseFileName(mFilename),EXECUTABLE_EXT);
        }
    }
    QString exePath;
    if (!mOptions.exeOutput.isEmpty()) {
        QDir baseDir(directory());
        exePath = baseDir.filePath(mOptions.exeOutput);
    } else {
        exePath = directory();
    }
    QDir exeDir(exePath);
    return exeDir.filePath(exeFileName);
}

QString Project::makeFileName()
{
    if (mOptions.useCustomMakefile)
        return mOptions.customMakefile;
    else
        return QDir(directory()).filePath(MAKEFILE_NAME);
}

bool Project::modified()
{
    // Project file modified? Done
    if (mModified)
        return true;// quick exit avoids loop over all units

    // Otherwise, check all units
    foreach (const PProjectUnit& unit, mUnits){
        if (unit->modified())
            return true;
    }
    return false;
}

void Project::open()
{
    QFile fileInfo(mFilename);
    if (fileInfo.exists()
            && !fileInfo.isWritable()) {
        if (QMessageBox::question(pMainWindow,
                                  tr("Remove Readonly Attribute"),
                                  tr("Project file '%1' is readonly.<br /> Remove the readonly attribute?")
                                  .arg(mFilename),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes) == QMessageBox::Yes) {
            fileInfo.setPermissions(
                        QFileDevice::WriteOwner
                        | QFileDevice::WriteGroup
                        | QFileDevice::WriteUser
                        );
        }
    }
    loadOptions();

    mNode = makeProjectNode();

    checkProjectFileForUpdate();

    mIniFile->beginGroup("Project");
    int uCount  = mIniFile->value("UnitCount",0).toInt();
    mIniFile->endGroup();
    //createFolderNodes;
    QDir dir(directory());
    for (int i=0;i<uCount;i++) {
        PProjectUnit newUnit = std::make_shared<ProjectUnit>();
        mIniFile->beginGroup(QString("Unit%1").arg(i));
        newUnit->setFileName(dir.filePath(mIniFile->value("FileName","").toString()));
        if (!QFileInfo(newUnit->fileName()).exists()) {
            QMessageBox::critical(pMainWindow,
                                  tr("File Not Found"),
                                  tr("Project file '%1' can't be found!")
                                  .arg(newUnit->fileName()),
                                  QMessageBox::Ok);
            newUnit->setModified(true);
        } else {
            newUnit->setFolder(mIniFile->value("Folder","").toString());
            newUnit->setCompile(mIniFile->value("Compile", true).toBool());
            newUnit->setCompileCpp(
                        mIniFile->value("CompileCpp",mOptions.useGPP).toBool());

            newUnit->setLink(mIniFile->value("Link", true).toBool());
            newUnit->setPriority(mIniFile->value("Priority", 1000).toInt());
            newUnit->setOverrideBuildCmd(mIniFile->value("OverrideBuildCmd", false).toInt());
            newUnit->setBuildCmd(mIniFile->value("BuildCmd", "").toString());
            newUnit->setDetectEncoding(mIniFile->value("DetectEncoding", mOptions.useUTF8).toBool());
            newUnit->setEncoding(mIniFile->value("Encoding",ENCODING_SYSTEM_DEFAULT).toByteArray());

            newUnit->setEditor(nullptr);
            newUnit->setNew(false);
            newUnit->setParent(this);
            newUnit->setNode(makeNewFileNode(baseFileName(newUnit->fileName()), false, folderNodeFromName(newUnit->folder())));
            newUnit->node()->unitIndex = mUnits.count();
            mUnits.append(newUnit);
        }
        mIniFile->endGroup();
    }
    rebuildNodes();
}

void Project::setFileName(const QString &value)
{
    if (mFilename!=value) {
        mIniFile->sync();
        mIniFile.reset();
        QFile::copy(mFilename,value);
        mFilename = value;
        setModified(true);
        mIniFile = std::make_shared<QSettings>(mFilename);
    }
}

void Project::setModified(bool value)
{
    QFile file(mFilename);
    // only mark modified if *not* read-only
    if (!file.exists()
            || (file.exists() && file.isWritable())) {
        mModified=value;
        emit modifyChanged(mModified);
    }
}

void Project::addFolder(const QString &s)
{
    if (mFolders.indexOf(s)<0) {
        mFolders.append(s);
        rebuildNodes();
        //todo: MainForm.ProjectView.Select(FolderNodeFromName(s));
        //folderNodeFromName(s)->makeVisible();
        setModified(true);
    }
}

PProjectUnit Project::addUnit(const QString &inFileName, PFolderNode parentNode, bool rebuild)
{
    PProjectUnit newUnit;
    // Don't add if it already exists
    if (fileAlreadyExists(inFileName)) {
        QMessageBox::critical(pMainWindow,
                                 tr("File Exists"),
                                 tr("File '%1' is already in the project"),
                              QMessageBox::Ok);
        return newUnit;
    }
    newUnit = std::make_shared<ProjectUnit>(this);

    // Set all properties
    newUnit->setFileName(QDir(directory()).filePath(inFileName));
    newUnit->setNew(false);
    newUnit->setEditor(nullptr);
    newUnit->setFolder(getFolderPath(parentNode));
    newUnit->setNode(makeNewFileNode(baseFileName(newUnit->fileName()), false, parentNode);
    newUnit->node()->unitIndex = mUnits.count();
    mUnits.append(newUnit);

  // Determine compilation flags
    switch(getFileType(inFileName)) {
    case FileType::CSource:
        newUnit->setCompile(true);
        newUnit->setCompileCpp(mOptions.useGPP);
        newUnit->setLink(true);
        break;
    case FileType::CppSource:
        newUnit->setCompile(true);
        newUnit->setCompileCpp(true);
        newUnit->setLink(true);
        break;
    case FileType::WindowsResourceSource:
        newUnit->setCompile(true);
        newUnit->setCompileCpp(mOptions.useGPP);
        newUnit->setLink(false);
        break;
    default:
        newUnit->setCompile(false);
        newUnit->setCompileCpp(false);
        newUnit->setLink(false);
    }
    newUnit->setPriority(1000);
    newUnit->setOverrideBuildCmd(false);
    newUnit->setBuildCmd("");
    if (rebuild)
        rebuildNodes();
    setModified(true);
    return newUnit;
}

void Project::buildPrivateResource(bool forceSave)
{
    int comp = 0;
    foreach (const PProjectUnit& unit,mUnits) {
        if (
                (getFileType(unit->fileName()) == FileType::WindowsResourceSource)
                && unit->compile() )
            comp++;
    }

    // if project has no other resources included
    // and does not have an icon
    // and does not include the XP style manifest
    // and does not include version info
    // then do not create a private resource file
    if ((comp == 0) &&
            (! mOptions.supportXPThemes)
            && (! mOptions.includeVersionInfo)
            && (mOptions.icon == "")) {
        mOptions.privateResource="";
        return;
    }

    // change private resource from <project_filename>.res
    // to <project_filename>_private.res
    //
    // in many cases (like in importing a MSVC project)
    // the project's resource file has already the
    // <project_filename>.res filename.
    QString res;
    if (!mOptions.privateResource.isEmpty()) {
        res = QDir(directory()).filePath(mOptions.privateResource);
        if (changeFileExt(res, DEV_PROJECT_EXT) == mFilename)
            res = changeFileExt(mFilename,QString("_private") + RC_EXT);
    } else
        res = changeFileExt(mFilename,QString("_private") + RC_EXT);
    res = extractRelativePath(mFilename, res);
    res.replace(' ','_');

    // don't run the private resource file and header if not modified,
    // unless ForceSave is true
    if (!forceSave
            && fileExists(res)
            && fileExists(changeFileExt(res, H_EXT))
            && !mModified)
        return;

    QStringList resFile;
    resFile.append("/* THIS FILE WILL BE OVERWRITTEN BY DEV-C++ */");
    resFile.append("/* DO NOT EDIT! */");
    resFile.append("");

    if (mOptions.includeVersionInfo) {
      resFile.append("#include <windows.h> // include for version info constants");
      resFile.append("");
    end;

    foreach (const PProjectUnit& unit, mUnits) {
        if (
                (getFileType(unit->fileName()) == FileType::WindowsResourceSource)
                && unit->compile() )
            resFile.append("#include \"" +
                           genMakePath(
                               extractRelativePath(directory(), unit->fileName()),
                               false,
                               false) + "\"");
    }

    if (!mOptions.icon.isEmpty()) {
        resFile.append("");
        QString icon = QDir(directory()).absoluteFilePath(mOptions.icon);
        if (fileExists(icon)) {
            icon = extractRelativePath(mFilename, icon);
            icon.replace('\\', '/');
            resFile.append("A ICON \"" + icon + '"');
        } else
            mOptions.icon = "";
    }

    if (mOptions.supportXPThemes) {
      resFile.append("");
      resFile.append("//");
      resFile.append("// SUPPORT FOR WINDOWS XP THEMES:");
      resFile.append("// THIS WILL MAKE THE PROGRAM USE THE COMMON CONTROLS");
      resFile.append("// LIBRARY VERSION 6.0 (IF IT IS AVAILABLE)");
      resFile.append("//");
      if (!mOptions.exeOutput.isEmpty())
          resFile.append(
                    "1 24 \"" +
                       genMakePath2(
                           includeTrailingPathDelimiter(mOptions.exeOutput)
                           + baseFileName(executable()))
                + ".Manifest\"");
      else
          resFile.append("1 24 \"" + baseFileName(executable()) + ".Manifest\"");
    }

    if Options.IncludeVersionInfo then begin
      resFile.append("');
      resFile.append("//');
      resFile.append("// TO CHANGE VERSION INFORMATION, EDIT PROJECT OPTIONS...');
      resFile.append("//');
      resFile.append("1 VERSIONINFO');
      resFile.append("FILEVERSION ' + Format('%d,%d,%d,%d', [Options.VersionInfo.Major, Options.VersionInfo.Minor,
        Options.VersionInfo.Release, Options.VersionInfo.Build]));
      resFile.append("PRODUCTVERSION ' + Format('%d,%d,%d,%d', [Options.VersionInfo.Major, Options.VersionInfo.Minor,
        Options.VersionInfo.Release, Options.VersionInfo.Build]));
      case Options.typ of
        dptGUI,
          dptCon: resFile.append("FILETYPE VFT_APP');
        dptStat: resFile.append("FILETYPE VFT_STATIC_LIB');
        dptDyn: resFile.append("FILETYPE VFT_DLL');
      end;
      resFile.append("{');
      resFile.append("  BLOCK "StringFileInfo"');
      resFile.append("  {');
      resFile.append("    BLOCK "' + Format('%4.4x%4.4x', [fOptions.VersionInfo.LanguageID, fOptions.VersionInfo.CharsetID])
        +
        '"');
      resFile.append("    {');
      resFile.append("      VALUE "CompanyName", "' + fOptions.VersionInfo.CompanyName + '"');
      resFile.append("      VALUE "FileVersion", "' + fOptions.VersionInfo.FileVersion + '"');
      resFile.append("      VALUE "FileDescription", "' + fOptions.VersionInfo.FileDescription + '"');
      resFile.append("      VALUE "InternalName", "' + fOptions.VersionInfo.InternalName + '"');
      resFile.append("      VALUE "LegalCopyright", "' + fOptions.VersionInfo.LegalCopyright + '"');
      resFile.append("      VALUE "LegalTrademarks", "' + fOptions.VersionInfo.LegalTrademarks + '"');
      resFile.append("      VALUE "OriginalFilename", "' + fOptions.VersionInfo.OriginalFilename + '"');
      resFile.append("      VALUE "ProductName", "' + fOptions.VersionInfo.ProductName + '"');
      resFile.append("      VALUE "ProductVersion", "' + fOptions.VersionInfo.ProductVersion + '"');
      resFile.append("    }');
      resFile.append("  }');

      // additional block for windows 95->NT
      resFile.append("  BLOCK "VarFileInfo"');
      resFile.append("  {');
      resFile.append("    VALUE "Translation", ' + Format('0x%4.4x, %4.4d', [fOptions.VersionInfo.LanguageID,
        fOptions.VersionInfo.CharsetID]));
      resFile.append("  }');

      resFile.append("}');
    end;

    Res := GetRealPath(Res, Directory);
    if resFile.Count > 3 then begin
      if FileExists(Res) and not ForceSave then begin
        Original := TStringList.Create;
        Original.LoadFromFile(Res);
        if CompareStr(Original.Text, resFile.Text) <> 0 then begin
          resFile.SaveToFile(Res);
        end;
        Original.Free;
      end else begin
        resFile.SaveToFile(Res);
      end;
      fOptions.PrivateResource := ExtractRelativePath(Directory, Res);
    end else begin
      if FileExists(Res) then
        DeleteFile(PAnsiChar(Res));
      Res := ChangeFileExt(Res, RES_EXT);
      if FileExists(Res) then
        DeleteFile(PAnsiChar(Res));
      fOptions.PrivateResource := '';
    end;
    if FileExists(Res) then
      FileSetDate(Res, DateTimeToFileDate(Now)); // fix the "Clock skew detected" warning ;)

    // create XP manifest
    if fOptions.SupportXPThemes then begin
      resFile.Clear;
      resFile.append("<?xml version="1.0" encoding="UTF-8" standalone="yes"?>');
      resFile.append("<assembly');
      resFile.append("  xmlns="urn:schemas-microsoft-com:asm.v1"');
      resFile.append("  manifestVersion="1.0">');
      resFile.append("<assemblyIdentity');
      resFile.append("    name="DevCpp.Apps.' + StringReplace(Name, ' ', '_', [rfReplaceAll]) + '"');
      resFile.append("    processorArchitecture="*"');
      resFile.append("    version="1.0.0.0"');
      resFile.append("    type="win32"/>');
      resFile.append("<description>' + Name + '</description>');
      resFile.append("<dependency>');
      resFile.append("    <dependentAssembly>');
      resFile.append("        <assemblyIdentity');
      resFile.append("            type="win32"');
      resFile.append("            name="Microsoft.Windows.Common-Controls"');
      resFile.append("            version="6.0.0.0"');
      resFile.append("            processorArchitecture="*"');
      resFile.append("            publicKeyToken="6595b64144ccf1df"');
      resFile.append("            language="*"');
      resFile.append("        />');
      resFile.append("    </dependentAssembly>');
      resFile.append("</dependency>');
      resFile.append("</assembly>');
      resFile.SaveToFile(Executable + '.Manifest');
      FileSetDate(Executable + '.Manifest', DateTimeToFileDate(Now)); // fix the "Clock skew detected" warning ;)
    end else if FileExists(Executable + '.Manifest') then
      DeleteFile(PAnsiChar(Executable + '.Manifest'));

    // create private header file
    Res := ChangeFileExt(Res, H_EXT);
    resFile.Clear;
    Def := StringReplace(ExtractFilename(UpperCase(Res)), '.', '_', [rfReplaceAll]);
    resFile.append("/* THIS FILE WILL BE OVERWRITTEN BY DEV-C++ */');
    resFile.append("/* DO NOT EDIT ! */');
    resFile.append("');
    resFile.append("#ifndef ' + Def);
    resFile.append("#define ' + Def);
    resFile.append("');
    resFile.append("/* VERSION DEFINITIONS */');
    resFile.append("#define VER_STRING'#9 + Format('"%d.%d.%d.%d"', [fOptions.VersionInfo.Major, fOptions.VersionInfo.Minor,
      fOptions.VersionInfo.Release, fOptions.VersionInfo.Build]));
    resFile.append("#define VER_MAJOR'#9 + IntToStr(fOptions.VersionInfo.Major));
    resFile.append("#define VER_MINOR'#9 + IntToStr(fOptions.VersionInfo.Minor));
    resFile.append("#define VER_RELEASE'#9 + IntToStr(fOptions.VersionInfo.Release));
    resFile.append("#define VER_BUILD'#9 + IntToStr(fOptions.VersionInfo.Build));
    resFile.append("#define COMPANY_NAME'#9'"' + fOptions.VersionInfo.CompanyName + '"');
    resFile.append("#define FILE_VERSION'#9'"' + fOptions.VersionInfo.FileVersion + '"');
    resFile.append("#define FILE_DESCRIPTION'#9'"' + fOptions.VersionInfo.FileDescription + '"');
    resFile.append("#define INTERNAL_NAME'#9'"' + fOptions.VersionInfo.InternalName + '"');
    resFile.append("#define LEGAL_COPYRIGHT'#9'"' + fOptions.VersionInfo.LegalCopyright + '"');
    resFile.append("#define LEGAL_TRADEMARKS'#9'"' + fOptions.VersionInfo.LegalTrademarks + '"');
    resFile.append("#define ORIGINAL_FILENAME'#9'"' + fOptions.VersionInfo.OriginalFilename + '"');
    resFile.append("#define PRODUCT_NAME'#9'"' + fOptions.VersionInfo.ProductName + '"');
    resFile.append("#define PRODUCT_VERSION'#9'"' + fOptions.VersionInfo.ProductVersion + '"');
    resFile.append("');
    resFile.append("#endif /*' + Def + '*/');
    resFile.SaveToFile(Res);

    if FileExists(Res) then
      FileSetDate(Res, DateTimeToFileDate(Now)); // fix the "Clock skew detected" warning ;)

    resFile.Free;
}

void Project::sortUnitsByPriority()
{
    std::sort(mUnits.begin(),mUnits.end(),[](const PProjectUnit& u1, const PProjectUnit& u2)->bool{
        return (u1->priority()>u2->priority());
    });
}

ProjectUnit::ProjectUnit(Project* parent)
{
    mEditor = nullptr;
    mNode = nullptr;
    mParent = parent;
}

Project *ProjectUnit::parent() const
{
    return mParent;
}

void ProjectUnit::setParent(Project* newParent)
{
    mParent = newParent;
}

Editor *ProjectUnit::editor() const
{
    return mEditor;
}

void ProjectUnit::setEditor(Editor *newEditor)
{
    mEditor = newEditor;
}

const QString &ProjectUnit::fileName() const
{
    return mFileName;
}

void ProjectUnit::setFileName(const QString &newFileName)
{
    mFileName = newFileName;
}

bool ProjectUnit::isNew() const
{
    return mNew;
}

void ProjectUnit::setNew(bool newNew)
{
    mNew = newNew;
}

const QString &ProjectUnit::folder() const
{
    return mFolder;
}

void ProjectUnit::setFolder(const QString &newFolder)
{
    mFolder = newFolder;
}

bool ProjectUnit::compile() const
{
    return mCompile;
}

void ProjectUnit::setCompile(bool newCompile)
{
    mCompile = newCompile;
}

bool ProjectUnit::compileCpp() const
{
    return mCompileCpp;
}

void ProjectUnit::setCompileCpp(bool newCompileCpp)
{
    mCompileCpp = newCompileCpp;
}

bool ProjectUnit::overrideBuildCmd() const
{
    return mOverrideBuildCmd;
}

void ProjectUnit::setOverrideBuildCmd(bool newOverrideBuildCmd)
{
    mOverrideBuildCmd = newOverrideBuildCmd;
}

const QString &ProjectUnit::buildCmd() const
{
    return mBuildCmd;
}

void ProjectUnit::setBuildCmd(const QString &newBuildCmd)
{
    mBuildCmd = newBuildCmd;
}

bool ProjectUnit::link() const
{
    return mLink;
}

void ProjectUnit::setLink(bool newLink)
{
    mLink = newLink;
}

int ProjectUnit::priority() const
{
    return mPriority;
}

void ProjectUnit::setPriority(int newPriority)
{
    mPriority = newPriority;
}

bool ProjectUnit::detectEncoding() const
{
    return mDetectEncoding;
}

void ProjectUnit::setDetectEncoding(bool newDetectEncoding)
{
    mDetectEncoding = newDetectEncoding;
}

const QByteArray &ProjectUnit::encoding() const
{
    return mEncoding;
}

void ProjectUnit::setEncoding(const QByteArray &newEncoding)
{
    mEncoding = newEncoding;
}

bool ProjectUnit::modified() const
{
    if (mEditor) {
        return mEditor->modified();
    } else {
        return false;
    }
}

void ProjectUnit::setModified(bool value)
{
    // Mark the change in the coupled editor
    if (mEditor) {
        return mEditor->setModified(value);
    }

    // If modified is set to true, mark project as modified too
    if (value) {
        mParent->setModified(true);
    }
}

bool ProjectUnit::save()
{
    bool previous=pMainWindow->fileSystemWatcher()->blockSignals(true);
    auto action = finally([&previous](){
        pMainWindow->fileSystemWatcher()->blockSignals(previous);
    });
    bool result=true;
    if (!mEditor && !fileExists(mFileName)) {
        // file is neither open, nor saved
        QStringList temp;
        StringsToFile(temp,mFileName);
    } else if (mEditor and modified()) {
        result = mEditor->save();
    }
    if (mNode) {
        mNode->text = baseFileName(mFileName);
    }
    return result;
}

PFolderNode &ProjectUnit::node()
{
    return mNode;
}

void ProjectUnit::setNode(const PFolderNode &newNode)
{
    mNode = newNode;
}
