#include "project.h"
#include "editor.h"
#include "mainwindow.h"
#include "utils.h"
#include "systemconsts.h"
#include "editorlist.h"
#include <parser/cppparser.h>
#include "utils.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include "settings.h"

Project::Project(const QString &filename, const QString &name, QObject *parent) : QObject(parent)
{
    mFilename = filename;
    mIniFile = std::make_shared<QSettings>(filename,QSettings::IniFormat);
    mParser = std::make_shared<CppParser>();
    mParser->setOnGetFileStream(
                std::bind(
                    &EditorList::getContentFromOpenedEditor,pMainWindow->editorList(),
                    std::placeholders::_1, std::placeholders::_2));
    resetCppParser(mParser);
    if (name == DEV_INTERNAL_OPEN)
        open();
    else {
        mName = name;
        mIniFile->beginGroup("Project");
        mIniFile->setValue("filename", mFilename);
        mIniFile->setValue("name", mName);
        mIniFile->endGroup();
        mNode = makeProjectNode();
    }
}

QString Project::directory() const
{
    QFileInfo fileInfo(mFilename);
    return fileInfo.absolutePath();
}

QString Project::executable() const
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

bool Project::modified() const
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
    newUnit->setNode(makeNewFileNode(baseFileName(newUnit->fileName()), false, parentNode));
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
    QString rcFile;
    if (!mOptions.privateResource.isEmpty()) {
        rcFile = QDir(directory()).filePath(mOptions.privateResource);
        if (changeFileExt(rcFile, DEV_PROJECT_EXT) == mFilename)
            rcFile = changeFileExt(mFilename,QString("_private") + RC_EXT);
    } else
        rcFile = changeFileExt(mFilename,QString("_private") + RC_EXT);
    rcFile = extractRelativePath(mFilename, rcFile);
    rcFile.replace(' ','_');

    // don't run the private resource file and header if not modified,
    // unless ForceSave is true
    if (!forceSave
            && fileExists(rcFile)
            && fileExists(changeFileExt(rcFile, H_EXT))
            && !mModified)
        return;

    QStringList contents;
    contents.append("/* THIS FILE WILL BE OVERWRITTEN BY DEV-C++ */");
    contents.append("/* DO NOT EDIT! */");
    contents.append("");

    if (mOptions.includeVersionInfo) {
      contents.append("#include <windows.h> // include for version info constants");
      contents.append("");
    }

    foreach (const PProjectUnit& unit, mUnits) {
        if (
                (getFileType(unit->fileName()) == FileType::WindowsResourceSource)
                && unit->compile() )
            contents.append("#include \"" +
                           genMakePath(
                               extractRelativePath(directory(), unit->fileName()),
                               false,
                               false) + "\"");
    }

    if (!mOptions.icon.isEmpty()) {
        contents.append("");
        QString icon = QDir(directory()).absoluteFilePath(mOptions.icon);
        if (fileExists(icon)) {
            icon = extractRelativePath(mFilename, icon);
            icon.replace('\\', '/');
            contents.append("A ICON \"" + icon + '"');
        } else
            mOptions.icon = "";
    }

    if (mOptions.supportXPThemes) {
      contents.append("");
      contents.append("//");
      contents.append("// SUPPORT FOR WINDOWS XP THEMES:");
      contents.append("// THIS WILL MAKE THE PROGRAM USE THE COMMON CONTROLS");
      contents.append("// LIBRARY VERSION 6.0 (IF IT IS AVAILABLE)");
      contents.append("//");
      if (!mOptions.exeOutput.isEmpty())
          contents.append(
                    "1 24 \"" +
                       genMakePath2(
                           includeTrailingPathDelimiter(mOptions.exeOutput)
                           + baseFileName(executable()))
                + ".Manifest\"");
      else
          contents.append("1 24 \"" + baseFileName(executable()) + ".Manifest\"");
    }

    if (mOptions.includeVersionInfo) {
        contents.append("");
        contents.append("//");
        contents.append("// TO CHANGE VERSION INFORMATION, EDIT PROJECT OPTIONS...");
        contents.append("//");
        contents.append("1 VERSIONINFO");
        contents.append("FILEVERSION " +
                       QString("%1,%2,%3,%4")
                       .arg(mOptions.versionInfo.major)
                       .arg(mOptions.versionInfo.minor)
                       .arg(mOptions.versionInfo.release)
                       .arg(mOptions.versionInfo.build));
        contents.append("PRODUCTVERSION " +
                       QString("%1,%2,%3,%4")
                       .arg(mOptions.versionInfo.major)
                       .arg(mOptions.versionInfo.minor)
                       .arg(mOptions.versionInfo.release)
                       .arg(mOptions.versionInfo.build));
        switch(mOptions.type) {
        case ProjectType::GUI:
        case ProjectType::Console:
            contents.append("FILETYPE VFT_APP");
            break;
        case ProjectType::StaticLib:
            contents.append("FILETYPE VFT_STATIC_LIB");
            break;
        case ProjectType::DynamicLib:
            contents.append("FILETYPE VFT_DLL");
            break;
        }
        contents.append("{");
        contents.append("  BLOCK \"StringFileInfo\"");
        contents.append("  {");
        contents.append("    BLOCK \"" +
                       QString("%1%2")
                       .arg(mOptions.versionInfo.languageID,4,16,QChar('0'))
                       .arg(mOptions.versionInfo.charsetID,4,16,QChar('0'))
                       + '"');
        contents.append("    {");
        contents.append("      VALUE \"CompanyName\", \""
                       + mOptions.versionInfo.companyName
                       + "\"");
        contents.append("      VALUE \"FileVersion\", \""
                       + mOptions.versionInfo.fileVersion
                       + "\"");
        contents.append("      VALUE \"FileDescription\", \""
                       + mOptions.versionInfo.fileDescription
                       + "\"");
        contents.append("      VALUE \"InternalName\", \""
                       + mOptions.versionInfo.internalName
                       + "\"");
        contents.append("      VALUE \"LegalCopyright\", \""
                       + mOptions.versionInfo.legalCopyright
                       + '"');
        contents.append("      VALUE \"LegalTrademarks\", \""
                       + mOptions.versionInfo.legalTrademarks
                       + "\"");
        contents.append("      VALUE \"OriginalFilename\", \""
                       + mOptions.versionInfo.originalFilename
                       + "\"");
        contents.append("      VALUE \"ProductName\", \""
                       + mOptions.versionInfo.productName + "\"");
        contents.append("      VALUE \"ProductVersion\", \""
                       + mOptions.versionInfo.productVersion + "\"");
        contents.append("    }");
        contents.append("  }");

        // additional block for windows 95->NT
        contents.append("  BLOCK \"VarFileInfo\"");
        contents.append("  {");
        contents.append("    VALUE \"Translation\", " +
                       QString("0x%1, %2")
                       .arg(mOptions.versionInfo.languageID,4,16,QChar('0'))
                       .arg(mOptions.versionInfo.charsetID));
        contents.append("  }");

        contents.append("}");
    }

    rcFile = QDir(directory()).absoluteFilePath(rcFile);
    if (contents.count() > 3) {
        StringsToFile(contents,rcFile);
        mOptions.privateResource = extractRelativePath(directory(), rcFile);
    } else {
      if (fileExists(rcFile))
          QFile::remove(rcFile);
      QString resFile = changeFileExt(rcFile, RES_EXT);
      if (fileExists(resFile))
          QFile::remove(resFile);
      mOptions.privateResource = "";
    }
//    if fileExists(Res) then
//      FileSetDate(Res, DateTimeToFileDate(Now)); // fix the "Clock skew detected" warning ;)

    // create XP manifest
    if (mOptions.supportXPThemes) {
        QStringList content;
        content.append("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
        content.append("<assembly");
        content.append("  xmlns=\"urn:schemas-microsoft-com:asm.v1\"");
        content.append("  manifestVersion=\"1.0\">");
        content.append("<assemblyIdentity");
        QString name = mName;
        name.replace(' ','_');
        content.append("    name=\"DevCpp.Apps." + name + '\"');
        content.append("    processorArchitecture=\"*\"");
        content.append("    version=\"1.0.0.0\"");
        content.append("    type=\"win32\"/>");
        content.append("<description>" + name + "</description>");
        content.append("<dependency>");
        content.append("    <dependentAssembly>");
        content.append("        <assemblyIdentity");
        content.append("            type=\"win32\"");
        content.append("            name=\"Microsoft.Windows.Common-Controls\"");
        content.append("            version=\"6.0.0.0\"");
        content.append("            processorArchitecture=\"*\"");
        content.append("            publicKeyToken=\"6595b64144ccf1df\"");
        content.append("            language=\"*\"");
        content.append("        />");
        content.append("    </dependentAssembly>");
        content.append("</dependency>");
        content.append("</assembly>");
        StringsToFile(content,executable() + ".Manifest");
    } else if (fileExists(executable() + ".Manifest"))
        QFile::remove(executable() + ".Manifest");

    // create private header file
    QString hFile = changeFileExt(rcFile, H_EXT);
    contents.clear();
    QString def = baseFileName(rcFile);
    def.replace(".","_");
    contents.append("/* THIS FILE WILL BE OVERWRITTEN BY DEV-C++ */");
    contents.append("/* DO NOT EDIT ! */");
    contents.append("");
    contents.append("#ifndef " + def);
    contents.append("#define " + def);
    contents.append("");
    contents.append("/* VERSION DEFINITIONS */");
    contents.append("#define VER_STRING\t" +
                   QString("\"%d.%d.%d.%d\"")
                   .arg(mOptions.versionInfo.major)
                   .arg(mOptions.versionInfo.minor)
                   .arg(mOptions.versionInfo.release)
                   .arg(mOptions.versionInfo.build));
    contents.append(QString("#define VER_MAJOR\t%1").arg(mOptions.versionInfo.major));
    contents.append(QString("#define VER_MINOR\t%1").arg(mOptions.versionInfo.minor));
    contents.append(QString("#define VER_RELEASE\t").arg(mOptions.versionInfo.release));
    contents.append(QString("#define VER_BUILD\t").arg(mOptions.versionInfo.build));
    contents.append(QString("#define COMPANY_NAME\t\"%1\"")
                   .arg(mOptions.versionInfo.companyName));
    contents.append(QString("#define FILE_VERSION\t\"%1\"")
                   .arg(mOptions.versionInfo.fileVersion));
    contents.append(QString("#define FILE_DESCRIPTION\t\"%1\"")
                   .arg(mOptions.versionInfo.fileDescription));
    contents.append(QString("#define INTERNAL_NAME\t\"%1\"")
                   .arg(mOptions.versionInfo.internalName));
    contents.append(QString("#define LEGAL_COPYRIGHT\t\"%1\"")
                   .arg(mOptions.versionInfo.legalCopyright));
    contents.append(QString("#define LEGAL_TRADEMARKS\t\"%1\"")
                   .arg(mOptions.versionInfo.legalTrademarks));
    contents.append(QString("#define ORIGINAL_FILENAME\t\"%1\"")
                   .arg(mOptions.versionInfo.originalFilename));
    contents.append(QString("#define PRODUCT_NAME\t\"%1\"")
                   .arg(mOptions.versionInfo.productName));
    contents.append(QString("#define PRODUCT_VERSION\t\"%1\"")
                   .arg(mOptions.versionInfo.productVersion));
    contents.append("");
    contents.append("#endif /*" + def + "*/");
    StringsToFile(contents,hFile);
}

void Project::checkProjectFileForUpdate()
{
    bool cnvt = false;
    mIniFile->beginGroup("Project");
    int uCount = mIniFile->value("UnitCount", 0).toInt();
    mIniFile->endGroup();
    // check if using old way to store resources and fix it
    QString oldRes = mIniFile->value("Resources", "").toString();
    if (!oldRes.isEmpty()) {
        QFile::copy(mFilename,mFilename+".bak");
        QStringList sl;
        sl = oldRes.split(';');
        for (int i=0;i<sl.count();i++){
            const QString& s = sl[i];
            mIniFile->beginGroup(QString("Unit%1").arg(uCount+i));
            mIniFile->setValue("Filename", s);
            mIniFile->setValue("Folder", "Resources");
            mIniFile->setValue("Compile",true);
            mIniFile->endGroup();
        }
        mIniFile->beginGroup("Project");
        mIniFile->setValue("UnitCount",uCount+sl.count());
        QString folders = mIniFile->value("Folders","").toString();
        if (!folders.isEmpty())
            folders += ",Resources";
        else
            folders = "Resources";
        mIniFile->setValue("Folders",folders);
        mIniFile->endGroup();
    }

    mIniFile->beginGroup("Project");
    mIniFile->remove("Resources");
    mIniFile->remove("Focused");
    mIniFile->remove("Order");
    mIniFile->remove("DebugInfo");
    mIniFile->remove("ProfileInfo");

    if (cnvt)
        QMessageBox::information(
                    pMainWindow,
                    tr("Project Updated"),
                    tr("Your project was succesfully updated to a newer file format!")
                    +"<br />"
                    +tr("If something has gone wrong, we kept a backup-file: '%1'...")
                    .arg(mFilename+".bak"),
                    QMessageBox::Ok);
}

void Project::closeUnit(int index)
{
    PProjectUnit unit = mUnits[index];
    if (unit->editor()) {
        saveUnitLayout(unit->editor(),index);
        pMainWindow->editorList()->forceCloseEditor(unit->editor());
        unit->setEditor(nullptr);
    }
}

void Project::createFolderNodes()
{
    mFolderNodes.clear();
    for (int idx=0;idx<mFolders.count();idx++) {
        PFolderNode node = mNode;
        QString s = mFolders[idx];
        int i = s.indexOf('/');
        while (i>=0) {
            PFolderNode findnode;
            for (int c=0;c<node->children.count();c++) {
                if (node->children[c]->text == s.mid(0,i))
                    findnode = node->children[c];
            }
            if (!findnode)
                node = makeNewFileNode(s.mid(0,i),true,node);
            else
                node = findnode;
            node->unitIndex = -1;
            s.remove(0,i);
            i = s.indexOf('/');
        }
        node = makeNewFileNode(s, true, node);
        node->unitIndex = -1;
        mFolderNodes.append(node);
    }
}

void Project::doAutoOpen()
{
    //todo:
//    case devData.AutoOpen of
//      0: begin
//          for i := 0 to pred(fUnits.Count) do
//            OpenUnit(i); // Open all
//          if fUnits.Count > 0 then
//            fUnits[0].Editor.Activate; // Show first
//        end;
//      1:
//        if fUnits.Count > 0 then
//          OpenUnit(0).Activate; // Open and show first
//      2:
//        LoadLayout; // Open previous selection
//    end;

}

bool Project::fileAlreadyExists(const QString &s)
{
    foreach (const PProjectUnit& unit, mUnits) {
        if (unit->fileName() == s)
            return true;
    }
    return false;
}

PFolderNode Project::folderNodeFromName(const QString &name)
{
    int index = mFolders.indexOf(name);
    if (index>=0) {
        return mFolderNodes[index];
    }
    return mNode;
}

QChar Project::getCompilerOption(const QString &optionString)
{
    // Does the option exist?
    Settings::PCompilerSet compilerSet = pSettings->compilerSets().getSet(mOptions.compilerSet);
    if (!compilerSet)
        return '0';
    int index = compilerSet->findOptionIndex(optionString);
    if (index>=0 && index<mOptions.compilerOptions.length()) {
        return mOptions.compilerOptions[index];
    }
    return '0';
}

QString Project::getFolderPath(PFolderNode node)
{
    QString result;
    if (!node)
        return result;

    if (node->unitIndex>=0) // not a folder
        return result;

    FolderNode* p = node.get();
    while (p && p->unitIndex==-1) {
        if (!result.isEmpty())
            result = p->text + "/" + result;
        p = p->parent;
    }
    return result;
}

int Project::getUnitFromString(const QString &s)
{
    return indexInUnits(s);
}

void Project::incrementBuildNumber()
{
    mOptions.versionInfo.build++;
    mOptions.versionInfo.fileVersion = QString("%1.%2.%3.%3")
            .arg(mOptions.versionInfo.major)
            .arg(mOptions.versionInfo.minor)
            .arg(mOptions.versionInfo.release)
            .arg(mOptions.versionInfo.build);
    if (mOptions.versionInfo.syncProduct)
        mOptions.versionInfo.productVersion = mOptions.versionInfo.fileVersion;
    setModified(true);
}

QString Project::listUnitStr(const QChar &separator)
{
    QStringList units;
    foreach (const PProjectUnit& unit, mUnits) {
        units.append('"'+unit->fileName()+'"');
    }
    return units.join(separator);
}

void Project::loadLayout()
{
    QSettings layIni = QSettings(changeFileExt(filename(), "layout"),QSettings::IniFormat);
    layIni.beginGroup("Editors");
    int topLeft = layIni.value("Focused", -1).toInt();
    //TopRight := layIni.ReadInteger('Editors', 'FocusedRight', -1);
    QString temp =layIni.value("Order", "").toString();
    layIni.endGroup();
    QStringList sl = temp.split(",");

    foreach (const QString& s,sl) {
        bool ok;
        int currIdx = s.toInt(&ok);
        if (ok) {
            openUnit(currIdx);
        }
    }
    if (topLeft>=0 && topLeft<mUnits.count() && mUnits[topLeft]->editor()) {
        mUnits[topLeft]->editor()->activate();
    }

}

void Project::loadOptions()
{
    mIniFile->beginGroup("Project");
    mName = mIniFile->value("name", "").toString();
    mOptions.icon = mIniFile->value("icon", "").toString();
    mOptions.version = mIniFile->value("Ver", 0).toInt();
    if (mOptions.version > 0) { // ver > 0 is at least a v5 project
        if (mOptions.version < 2) {
            mOptions.version = 2;
            QMessageBox::information(pMainWindow,
                                     tr("Settings need update"),
                                     tr("The compiler settings format of Dev-C++ has changed.")
                                     +"<BR /><BR />"
                                     +tr("Please update your settings at Project >> Project Options >> Compiler and save your project."),
                                     QMessageBox::Ok);
        }

        mOptions.type = static_cast<ProjectType>(mIniFile->value("type", 0).toInt());
        mOptions.compilerCmd = mIniFile->value("Compiler", "").toString();
        mOptions.cppCompilerCmd = mIniFile->value("CppCompiler", "").toString();
        fOptions.LinkerCmd := ReadString('Project', 'Linker', '');
        fOptions.ObjFiles.DelimitedText := ReadString('Project', 'ObjFiles', '');
        fOptions.Libs.DelimitedText := ReadString('Project', 'Libs', '');
        fOptions.Includes.DelimitedText := ReadString('Project', 'Includes', '');
        fOptions.PrivateResource := ReadString('Project', 'PrivateResource', '');
        fOptions.ResourceIncludes.DelimitedText := ReadString('Project', 'ResourceIncludes', '');
        fOptions.MakeIncludes.DelimitedText := ReadString('Project', 'MakeIncludes', '');
        fOptions.UseGpp := ReadBool('Project', 'IsCpp', FALSE);
        fOptions.ExeOutput := ReadString('Project', 'ExeOutput', '');
        fOptions.ObjectOutput := ReadString('Project', 'ObjectOutput', '');
        fOptions.LogOutput := ReadString('Project', 'LogOutput', '');
        fOptions.LogOutputEnabled := ReadBool('Project', 'LogOutputEnabled', FALSE);
        fOptions.OverrideOutput := ReadBool('Project', 'OverrideOutput', FALSE);
        fOptions.OverridenOutput := ReadString('Project', 'OverrideOutputName', '');
        fOptions.HostApplication := ReadString('Project', 'HostApplication', '');
        fOptions.UseCustomMakefile := ReadBool('Project', 'UseCustomMakefile', FALSE);
        fOptions.CustomMakefile := ReadString('Project', 'CustomMakefile', '');
        fOptions.UsePrecompiledHeader := ReadBool('Project', 'UsePrecompiledHeader', FALSE);
        fOptions.PrecompiledHeader := ReadString('Project', 'PrecompiledHeader', '');
        fOptions.CmdLineArgs := ReadString('Project', 'CommandLine', '');
        fFolders.CommaText := ReadString('Project', 'Folders', '');
        fOptions.IncludeVersionInfo := ReadBool('Project', 'IncludeVersionInfo', False);
        fOptions.SupportXPThemes := ReadBool('Project', 'SupportXPThemes', False);
        fOptions.CompilerSet := ReadInteger('Project', 'CompilerSet', devCompilerSets.DefaultSetIndex);
        if (fOptions.CompilerSet >= devCompilerSets.Count) or (fOptions.CompilerSet < 0)  then begin // TODO: change from indices to names
          MessageDlg(Lang[ID_MSG_COMPILERNOTFOUND], mtError, [mbOk], 0);
          fOptions.CompilerSet := devCompilerSets.DefaultSetIndex;
          Modified := True;
        end;
        fOptions.CompilerOptions := ReadString('Project', 'CompilerSettings', '');
        fOptions.StaticLink := ReadBool('Project','StaticLink',True);
        fOptions.AddCharset := ReadBool('Project','AddCharset',True);
        fOptions.UseUTF8 := ReadBool('Project','UseUTF8',False);
        fOptions.VersionInfo.Major := ReadInteger('VersionInfo', 'Major', 0);
        fOptions.VersionInfo.Minor := ReadInteger('VersionInfo', 'Minor', 1);
        fOptions.VersionInfo.Release := ReadInteger('VersionInfo', 'Release', 1);
        fOptions.VersionInfo.Build := ReadInteger('VersionInfo', 'Build', 1);
        fOptions.VersionInfo.LanguageID := ReadInteger('VersionInfo', 'LanguageID', $0409);
        fOptions.VersionInfo.CharsetID := ReadInteger('VersionInfo', 'CharsetID', $04E4);
        fOptions.VersionInfo.CompanyName := ReadString('VersionInfo', 'CompanyName', '');
        fOptions.VersionInfo.FileVersion := ReadString('VersionInfo', 'FileVersion', '0.1');
        fOptions.VersionInfo.FileDescription := ReadString('VersionInfo', 'FileDescription',
          'Developed using the Dev-C++ IDE');
        fOptions.VersionInfo.InternalName := ReadString('VersionInfo', 'InternalName', '');
        fOptions.VersionInfo.LegalCopyright := ReadString('VersionInfo', 'LegalCopyright', '');
        fOptions.VersionInfo.LegalTrademarks := ReadString('VersionInfo', 'LegalTrademarks', '');
        fOptions.VersionInfo.OriginalFilename := ReadString('VersionInfo', 'OriginalFilename',
          ExtractFilename(Executable));
        fOptions.VersionInfo.ProductName := ReadString('VersionInfo', 'ProductName', Name);
        fOptions.VersionInfo.ProductVersion := ReadString('VersionInfo', 'ProductVersion', '0.1.1.1');
        fOptions.VersionInfo.AutoIncBuildNr := ReadBool('VersionInfo', 'AutoIncBuildNr', False);
        fOptions.VersionInfo.SyncProduct := ReadBool('VersionInfo', 'SyncProduct', False);
      end else begin // dev-c < 4
        fOptions.Ver := -1;
        if not ReadBool('Project', 'NoConsole', TRUE) then
          fOptions.typ := dptCon
        else if ReadBool('Project', 'IsDLL', FALSE) then
          fOptions.Typ := dptDyn
        else
          fOptions.typ := dptGUI;

        fOptions.PrivateResource := ReadString('Project', 'PrivateResource', '');
        fOptions.ResourceIncludes.DelimitedText := ReadString('Project', 'ResourceIncludes', '');
        fOptions.ObjFiles.Add(ReadString('Project', 'ObjFiles', ''));
        fOptions.Includes.Add(ReadString('Project', 'IncludeDirs', ''));
        fOptions.CompilerCmd := ReadString('Project', 'CompilerOptions', '');
        fOptions.usegpp := ReadBool('Project', 'Use_GPP', FALSE);
        fOptions.ExeOutput := ReadString('Project', 'ExeOutput', '');
        fOptions.ObjectOutput := ReadString('Project', 'ObjectOutput', '');
        fOptions.OverrideOutput := ReadBool('Project', 'OverrideOutput', FALSE);
        fOptions.OverridenOutput := ReadString('Project', 'OverrideOutputName', '');
        fOptions.HostApplication := ReadString('Project', 'HostApplication', '');
      end;
    end;
}

PCppParser Project::cppParser()
{
    return mParser;
}

void Project::sortUnitsByPriority()
{
    std::sort(mUnits.begin(),mUnits.end(),[](const PProjectUnit& u1, const PProjectUnit& u2)->bool{
        return (u1->priority()>u2->priority());
    });
}

int Project::indexInUnits(const QString &fileName) const
{
    QDir dir(directory());
    for (int i=0;i<mUnits.count();i++) {
        PProjectUnit unit = mUnits[i];
        if (dir.absoluteFilePath(fileName) == dir.absoluteFilePath(unit->fileName()))
            return i;
    }
    return -1;
}

int Project::indexInUnits(const Editor *editor) const
{
    if (!editor)
        return -1;
    return indexInUnits(editor->filename());
}

std::shared_ptr<QSettings> &Project::iniFile()
{
    return mIniFile;
}

void Project::setIniFile(const std::shared_ptr<QSettings> &newIniFile)
{
    mIniFile = newIniFile;
}

const QString &Project::filename() const
{
    return mFilename;
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
