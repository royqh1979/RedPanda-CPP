#include "project.h"
#include "editor.h"
#include "mainwindow.h"
#include "utils.h"
#include "systemconsts.h"
#include "editorlist.h"
#include <parser/cppparser.h>
#include "utils.h"
#include "platform.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextCodec>
#include "settings.h"
#include <QDebug>

Project::Project(const QString &filename, const QString &name, QObject *parent) :
    QObject(parent),
    mModel(this)
{
    mFilename = filename;
    mIniFile = std::make_shared<QSettings>(filename,QSettings::IniFormat);
    mIniFile->setIniCodec(QTextCodec::codecForName(getDefaultSystemEncoding()));
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

Project::~Project()
{
    pMainWindow->editorList()->beginUpdate();
    foreach (const PProjectUnit& unit, mUnits) {
        if (unit->editor()) {
            pMainWindow->editorList()->forceCloseEditor(unit->editor());
            unit->setEditor(nullptr);
        }
    }
    pMainWindow->editorList()->endUpdate();
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
            exeFileName = changeFileExt(extractFileName(mFilename),STATIC_LIB_EXT);
            break;
        case ProjectType::DynamicLib:
            exeFileName = changeFileExt(extractFileName(mFilename),DYNAMIC_LIB_EXT);
            break;
        default:
            exeFileName = changeFileExt(extractFileName(mFilename),EXECUTABLE_EXT);
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
    mModel.beginUpdate();
    auto action = finally([this]{
        mModel.endUpdate();
    });
    QFile fileInfo(mFilename);
    loadOptions();

    mNode = makeProjectNode();

    checkProjectFileForUpdate();
    qDebug()<<"ini filename:"<<mIniFile->fileName();
    int uCount  = mIniFile->value("UnitCount",0).toInt();
    mIniFile->endGroup();
    //createFolderNodes;
    QDir dir(directory());
    for (int i=0;i<uCount;i++) {
        PProjectUnit newUnit = std::make_shared<ProjectUnit>(this);
        mIniFile->beginGroup(QString("Unit%1").arg(i+1));
        newUnit->setFileName(dir.absoluteFilePath(mIniFile->value("FileName","").toString()));
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
            newUnit->setEncoding(mIniFile->value("FileEncoding",ENCODING_SYSTEM_DEFAULT).toByteArray());

            newUnit->setEditor(nullptr);
            newUnit->setNew(false);
            newUnit->setParent(this);
            newUnit->setNode(makeNewFileNode(extractFileName(newUnit->fileName()), false, folderNodeFromName(newUnit->folder())));
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
        QFile::rename(mFilename,value);
        mFilename = value;
        setModified(true);
        mIniFile = std::make_shared<QSettings>(mFilename, QSettings::IniFormat);
        mIniFile->setIniCodec(QTextCodec::codecForName(getDefaultSystemEncoding()));
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

PFolderNode Project::makeNewFileNode(const QString &s, bool isFolder, PFolderNode newParent)
{
    PFolderNode node = std::make_shared<FolderNode>();
    newParent->children.append(node);
    node->parent = newParent;
    node->text = s;
    if (newParent) {
        node->level = newParent->level+1;
    }
    if (isFolder)
        node->unitIndex = -1;
    return node;
}

PFolderNode Project::makeProjectNode()
{
    PFolderNode node = std::make_shared<FolderNode>();
    node->text = mName;
    node->level = 0;
    return node;
}

int Project::newUnit(PFolderNode parentNode, const QString customFileName)
{
    PProjectUnit newUnit = std::make_shared<ProjectUnit>(this);

    // Select folder to add unit to
    if (!parentNode)
        parentNode = mNode; // project root node

    if (parentNode->unitIndex>=0) { //it's a file
        parentNode = mNode;
    }
    QString s;
    QDir dir(directory());
    // Find unused 'new' filename
    if (customFileName.isEmpty()) {
        do {
            s = dir.absoluteFilePath(tr("untitled")+QString("%1").arg(getNewFileNumber()));
        } while (fileExists(s));
    } else {
        s = dir.absoluteFilePath(customFileName);
    }
    // Add
    int result = mUnits.count();
    mUnits.append(newUnit);

    // Set all properties
    newUnit->setFileName(s);
    newUnit->setNew(true);
    newUnit->setEditor(nullptr);
    newUnit->setFolder(getFolderPath(parentNode));
    newUnit->setNode(makeNewFileNode(extractFileName(newUnit->fileName()),
                                     false, parentNode));
    newUnit->node()->unitIndex = result;
    //parentNode.Expand(True);
    newUnit->setCompile(true);
    newUnit->setCompileCpp(mOptions.useGPP);
    newUnit->setLink(true);
    newUnit->setPriority(1000);
    newUnit->setOverrideBuildCmd(false);
    newUnit->setBuildCmd("");
    newUnit->setModified(true);
    return result;
}

Editor *Project::openUnit(int index)
{
    if ((index < 0) || (index >= mUnits.count()))
        return nullptr;

    PProjectUnit unit = mUnits[index];

    if (!unit->fileName().isEmpty()) {
        QDir dir(directory());
        QString fullPath = dir.absoluteFilePath(unit->fileName());
        Editor * editor = pMainWindow->editorList()->getOpenedEditorByFilename(fullPath);
        if (editor) {//already opened in the editors
            editor->setInProject(true);
            return editor;
        }
        QByteArray encoding;
        encoding = mOptions.encoding.toLocal8Bit();
        editor = pMainWindow->editorList()->newEditor(fullPath, encoding, true, false);
        editor->setInProject(true);
        unit->setEditor(editor);
        unit->setEncoding(encoding);
        loadUnitLayout(editor,index);
        return editor;
    }
    return nullptr;
}

void Project::rebuildNodes()
{
    // Remember if folder nodes were expanded or collapsed
    // Create a list of expanded folder nodes
//    QStringList  oldPaths := TStringList.Create;
//      with MainForm.ProjectView do
//        for idx := 0 to Items.Count - 1 do begin
//          tempnode := Items[idx];
//          if tempnode.Expanded and (tempnode.Data = Pointer(-1)) then // data=pointer(-1) - it's folder
//            oldPaths.Add(GetFolderPath(tempnode));
//        end;

    mModel.beginUpdate();
    // Delete everything
    mNode->children.clear();

    // Recreate everything
    createFolderNodes();

    for (int idx=0;idx<mUnits.count();idx++) {
        mUnits[idx]->setNode(
                    makeNewFileNode(
                        extractRelativePath(filename(),mUnits[idx]->fileName()),
                        false,
                        folderNodeFromName(mUnits[idx]->folder())
                        )
                    );
        mUnits[idx]->node()->unitIndex = idx;
    }

//      // expand nodes expanded before recreating the project tree
//      fNode.Collapse(True);
//      with MainForm.ProjectView do
//        for idx := 0 to Items.Count - 1 do begin
//          tempnode := Items[idx];
//          if (tempnode.Data = Pointer(-1)) then //it's a folder
//            if oldPaths.IndexOf(GetFolderPath(tempnode)) >= 0 then
//              tempnode.Expand(False);
//        end;
//      FreeAndNil(oldPaths);

//      fNode.Expand(False);

    mModel.endUpdate();
    emit nodesChanged();
}

bool Project::removeEditor(int index, bool doClose)
{
    mModel.beginUpdate();
    auto action = finally([this]{
        mModel.endUpdate();
    });
    if (index<0 || index>=mUnits.count())
        return false;

    PProjectUnit unit = mUnits[index];

    // Attempt to close it
    if (doClose && (unit->editor())) {
        if (!pMainWindow->editorList()->closeEditor(unit->editor()))
            return false;
    }

//if not fUnits.GetItem(index).fNew then
    mIniFile->remove("Unit"+QString("%1").arg(index+1));
    PFolderNode node = unit->node();
    PFolderNode parent = node->parent.lock();
    if (parent) {
        parent->children.removeAll(node);
    }
    mUnits.removeAt(index);
    updateNodeIndexes();
    setModified(true);
    return true;
}

bool Project::removeFolder(PFolderNode node)
{
    mModel.beginUpdate();
    auto action = finally([this]{
        mModel.endUpdate();
    });
    // Sanity check
    if (!node)
        return false;

    // Check if this is actually a folder
    if (node->unitIndex>=0 || node->level<1)
        return false;

    // Let this function call itself
    removeFolderRecurse(node);

    // Update list of folders (sets modified)
    updateFolders();
    return true;
}

void Project::resetParserProjectFiles()
{
    mParser->clearProjectFiles();
    mParser->clearProjectIncludePaths();
    foreach (const PProjectUnit& unit, mUnits) {
        mParser->addFileToScan(unit->fileName());
    }
    foreach (const QString& s, mOptions.includes) {
        mParser->addProjectIncludePath(s);
    }
}

void Project::saveAll()
{
    if (!saveUnits())
        return;
    saveOptions(); // update other data, and save to disk
    saveLayout(); // save current opened files, and which is "active".

    // We have saved everything to disk, so mark unmodified
    setModified(false);
}

void Project::saveLayout()
{
    QString s = changeFileExt(mFilename, "layout");
    QSettings layIni(mFilename,QSettings::IniFormat);
    QStringList sl;
    // Write list of open project files
    for (int i=0;i<pMainWindow->editorList()->pageCount();i++) {
        Editor* e= (*(pMainWindow->editorList()))[i];
        if (e && e->inProject())
            sl.append(QString("%1").arg(indexInUnits(e)));
    }
    layIni.beginGroup("Editors");
    layIni.setValue("Order",sl.join(","));

    Editor *e, *e2;
    // Remember what files were visible
    pMainWindow->editorList()->getVisibleEditors(e, e2);
    if (e)
        layIni.setValue("Focused", indexInUnits(e));
    layIni.endGroup();
    // save editor info
    for (int i=0;i<mUnits.count();i++) {
        layIni.beginGroup(QString("Editor_%1").arg(i));
        PProjectUnit unit = mUnits[i];
        Editor* editor = unit->editor();
        if (editor) {
            layIni.setValue("CursorCol", editor->caretX());
            layIni.setValue("CursorRow", editor->caretY());
            layIni.setValue("TopLine", editor->topLine());
            layIni.setValue("LeftChar", editor->leftChar());
        }
        layIni.endGroup();
        // remove old data from project file
        mIniFile->beginGroup(QString("Unit%1").arg(i+1));
        mIniFile->remove("Open");
        mIniFile->remove("Top");
        mIniFile->remove("CursorCol");
        mIniFile->remove("CursorRow");
        mIniFile->remove("TopLine");
        mIniFile->remove("LeftChar");
        mIniFile->endGroup();
    }
}

void Project::saveUnitAs(int i, const QString &sFileName)
{
    if ((i < 0) || (i >= mUnits.count()))
        return;
    PProjectUnit unit = mUnits[i];
//    if (fileExists(unit->fileName())) {
//        unit->setNew(false);
//    }
    unit->setNew(false);
    unit->setFileName(sFileName);
    mIniFile->beginGroup(QString("Unit%1").arg(i+1));
    mIniFile->setValue("FileName",
                       extractRelativePath(
                           directory(),
                           sFileName));
    mIniFile->endGroup();
    mIniFile->sync();
    setModified(true);
}

void Project::saveUnitLayout(Editor *e, int index)
{
    if (!e)
        return;
    QSettings layIni = QSettings(changeFileExt(filename(), "layout"));
    layIni.beginGroup(QString("Editor_%1").arg(index));
    layIni.setValue("CursorCol", e->caretX());
    layIni.setValue("CursorRow", e->caretY());
    layIni.setValue("TopLine", e->topLine());
    layIni.setValue("LeftChar", e->leftChar());
    layIni.endGroup();
}

bool Project::saveUnits()
{
    int count = 0;
    for (int idx = 0; idx < mUnits.count(); idx++) {
        PProjectUnit unit = mUnits[idx];
        bool rd_only = false;
        mIniFile->beginGroup(QString("Unit%1").arg(count+1));
        if (unit->modified() && fileExists(unit->fileName())
            && isReadOnly(unit->fileName())) {
            // file is read-only
            QMessageBox::critical(pMainWindow,
                                  tr("Can't save file"),
                                  tr("Can't save file '%1'").arg(unit->fileName()),
                                  QMessageBox::Ok
                                  );
            rd_only = true;
        }
        if (!rd_only) {
            if (!unit->save() && unit->isNew())
                return false;
        }

        // saved new file or an existing file add to project file

        mIniFile->setValue("FileName",
                           extractRelativePath(
                               directory(),
                               unit->fileName()));
        count++;
        switch(getFileType(unit->fileName())) {
        case FileType::CHeader:
        case FileType::CSource:
        case FileType::CppHeader:
        case FileType::CppSource:
            mIniFile->setValue("CompileCpp", unit->compileCpp());
            break;
        case FileType::WindowsResourceSource:
            unit->setFolder("Resources");
        }

        mIniFile->setValue("Folder", unit->folder());
        mIniFile->setValue("Compile", unit->compile());
        mIniFile->setValue("Link", unit->link());
        mIniFile->setValue("Priority", unit->priority());
        mIniFile->setValue("OverrideBuildCmd", unit->overrideBuildCmd());
        mIniFile->setValue("BuildCmd", unit->buildCmd());
        mIniFile->setValue("DetectEncoding", unit->encoding()==ENCODING_AUTO_DETECT);
        mIniFile->setValue("FileEncoding", unit->encoding());
        mIniFile->endGroup();
    }
    mIniFile->beginGroup("Project");
    mIniFile->setValue("UnitCount",count);
    mIniFile->endGroup();
    mIniFile->sync();
    return true;
}

void Project::setCompilerOption(const QString &optionString, const QChar &value)
{
    if (mOptions.compilerSet<0 || mOptions.compilerSet>=pSettings->compilerSets().size()) {
        return;
    }
    std::shared_ptr<Settings::CompilerSet> compilerSet = pSettings->compilerSets().list()[mOptions.compilerSet];
    int optionIndex = compilerSet->findOptionIndex(optionString);
    // Does the option exist?
    if (optionIndex>=0){
        mOptions.compilerOptions[optionIndex] = value;
    }
}

void Project::updateFolders()
{
    mFolders.clear();
    updateFolderNode(mNode);
    for (int idx = 0; idx < mUnits.count();idx++)
        mUnits[idx]->setFolder(
                    getFolderPath(
                        mUnits[idx]->node()->parent.lock()
                        )
                    );
    setModified(true);
}

void Project::updateNodeIndexes()
{
    for (int idx = 0;idx<mUnits.count();idx++)
        mUnits[idx]->node()->unitIndex = idx;
}

void Project::saveOptions()
{
    mIniFile->beginGroup("Project");
    mIniFile->setValue("FileName", extractRelativePath(directory(), mFilename));
    mIniFile->setValue("Name", mName);
    mIniFile->setValue("Type", static_cast<int>(mOptions.type));
    mIniFile->setValue("Ver", 3); // Is 3 as of Red Panda Dev-C++ 7.0
    mIniFile->setValue("ObjFiles", mOptions.objFiles.join(";"));
    mIniFile->setValue("Includes", mOptions.includes.join(";"));
    mIniFile->setValue("Libs", mOptions.libs.join(";"));
    mIniFile->setValue("PrivateResource", mOptions.privateResource);
    mIniFile->setValue("ResourceIncludes", mOptions.resourceIncludes.join(";"));
    mIniFile->setValue("MakeIncludes", mOptions.makeIncludes.join(";"));
    mIniFile->setValue("Compiler", mOptions.compilerCmd);
    mIniFile->setValue("CppCompiler", mOptions.cppCompilerCmd);
    mIniFile->setValue("Linker", mOptions.linkerCmd);
    mIniFile->setValue("IsCpp", mOptions.useGPP);
    mIniFile->setValue("Icon", extractRelativePath(directory(), mOptions.icon));
    mIniFile->setValue("ExeOutput", mOptions.exeOutput);
    mIniFile->setValue("ObjectOutput", mOptions.objectOutput);
    mIniFile->setValue("LogOutput", mOptions.logOutput);
    mIniFile->setValue("LogOutputEnabled", mOptions.logOutputEnabled);
    mIniFile->setValue("OverrideOutput", mOptions.overrideOutput);
    mIniFile->setValue("OverrideOutputName", mOptions.overridenOutput);
    mIniFile->setValue("HostApplication", mOptions.hostApplication);
    mIniFile->setValue("UseCustomMakefile", mOptions.useCustomMakefile);
    mIniFile->setValue("CustomMakefile", mOptions.customMakefile);
    mIniFile->setValue("UsePrecompiledHeader", mOptions.usePrecompiledHeader);
    mIniFile->setValue("PrecompiledHeader", mOptions.precompiledHeader);
    mIniFile->setValue("CommandLine", mOptions.cmdLineArgs);
    mIniFile->setValue("Folders", mFolders.join(";"));
    mIniFile->setValue("IncludeVersionInfo", mOptions.includeVersionInfo);
    mIniFile->setValue("SupportXPThemes", mOptions.supportXPThemes);
    mIniFile->setValue("CompilerSet", mOptions.compilerSet);
    mIniFile->setValue("CompilerSettings", mOptions.compilerOptions);
    mIniFile->setValue("StaticLink", mOptions.staticLink);
    mIniFile->setValue("AddCharset", mOptions.addCharset);
    mIniFile->setValue("Encoding",mOptions.encoding);
    //for Red Panda Dev C++ 6 compatibility
    mIniFile->setValue("UseUTF8",mOptions.encoding == ENCODING_UTF8);
    mIniFile->endGroup();

    mIniFile->beginGroup("VersionInfo");
    mIniFile->setValue("Major", mOptions.versionInfo.major);

    mIniFile->setValue("Minor", mOptions.versionInfo.minor);
    mIniFile->setValue("Release", mOptions.versionInfo.release);
    mIniFile->setValue("Build", mOptions.versionInfo.build);
    mIniFile->setValue("LanguageID", mOptions.versionInfo.languageID);
    mIniFile->setValue("CharsetID", mOptions.versionInfo.charsetID);
    mIniFile->setValue("CompanyName", mOptions.versionInfo.companyName);
    mIniFile->setValue("FileVersion", mOptions.versionInfo.fileVersion);
    mIniFile->setValue("FileDescription", mOptions.versionInfo.fileDescription);
    mIniFile->setValue("InternalName", mOptions.versionInfo.internalName);
    mIniFile->setValue("LegalCopyright", mOptions.versionInfo.legalCopyright);
    mIniFile->setValue("LegalTrademarks", mOptions.versionInfo.legalTrademarks);
    mIniFile->setValue("OriginalFilename", mOptions.versionInfo.originalFilename);
    mIniFile->setValue("ProductName", mOptions.versionInfo.productName);
    mIniFile->setValue("ProductVersion", mOptions.versionInfo.productVersion);
    mIniFile->setValue("AutoIncBuildNr", mOptions.versionInfo.autoIncBuildNr);
    mIniFile->setValue("SyncProduct", mOptions.versionInfo.syncProduct);
    mIniFile->endGroup();

    //delete outdated dev4 project options
    mIniFile->beginGroup("Project");
    mIniFile->remove("NoConsole");
    mIniFile->remove("IsDLL");
    mIniFile->remove("ResFiles");
    mIniFile->remove("IncludeDirs");
    mIniFile->remove("CompilerOptions");
    mIniFile->remove("Use_GPP");
    mIniFile->endGroup();

    mIniFile->sync(); // force flush

}

void Project::addFolder(const QString &s)
{
    if (mFolders.indexOf(s)<0) {
        mModel.beginUpdate();
        auto action = finally([this]{
            mModel.endUpdate();
        });
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
    newUnit->setNode(makeNewFileNode(extractFileName(newUnit->fileName()), false, parentNode));
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
    if (rebuild) {
        mModel.beginUpdate();
        auto action = finally([this]{
            mModel.endUpdate();
        });
        rebuildNodes();
    }
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
                           + extractFileName(executable()))
                + ".Manifest\"");
      else
          contents.append("1 24 \"" + extractFileName(executable()) + ".Manifest\"");
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
    QString def = extractFileName(rcFile);
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
        sl = oldRes.split(';',Qt::SkipEmptyParts);
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

    PFolderNode p = node;
    while (p && p->unitIndex==-1) {
        if (!result.isEmpty())
            result = p->text + "/" + result;
        p = p->parent.lock();
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
    QStringList sl = temp.split(",",Qt::SkipEmptyParts);

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
    mIniFile->endGroup();
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

        mIniFile->beginGroup("Project");
        mOptions.type = static_cast<ProjectType>(mIniFile->value("type", 0).toInt());
        mOptions.compilerCmd = mIniFile->value("Compiler", "").toString();
        mOptions.cppCompilerCmd = mIniFile->value("CppCompiler", "").toString();
        mOptions.linkerCmd = mIniFile->value("Linker", "").toString();
        mOptions.objFiles = mIniFile->value("ObjFiles", "").toString().split(";",Qt::SkipEmptyParts);
        mOptions.libs = mIniFile->value("Libs", "").toString().split(";",Qt::SkipEmptyParts);
        mOptions.includes = mIniFile->value("Includes", "").toString().split(";",Qt::SkipEmptyParts);
        mOptions.privateResource = mIniFile->value("PrivateResource", "").toString();
        mOptions.resourceIncludes = mIniFile->value("ResourceIncludes", "").toString().split(";",Qt::SkipEmptyParts);
        mOptions.makeIncludes = mIniFile->value("MakeIncludes","").toString().split(";",Qt::SkipEmptyParts);
        mOptions.useGPP = mIniFile->value("IsCpp", false).toBool();
        mOptions.exeOutput = mIniFile->value("ExeOutput", "").toString();
        mOptions.objectOutput = mIniFile->value("ObjectOutput", "").toString();
        mOptions.logOutput = mIniFile->value("LogOutput","").toString();
        mOptions.logOutputEnabled = mIniFile->value("LogOutputEnabled", false).toBool();
        mOptions.overrideOutput = mIniFile->value("OverrideOutput", false).toBool();
        mOptions.overridenOutput = mIniFile->value("OverrideOutputName","").toString();
        mOptions.hostApplication = mIniFile->value("HostApplication","").toString();
        mOptions.useCustomMakefile = mIniFile->value("UseCustomMakefile", false).toBool();
        mOptions.customMakefile = mIniFile->value("CustomMakefile","").toString();
        mOptions.usePrecompiledHeader = mIniFile->value("UsePrecompiledHeader", false).toBool();
        mOptions.precompiledHeader = mIniFile->value("PrecompiledHeader","").toString();
        mOptions.cmdLineArgs = mIniFile->value("CommandLine","").toString();
        mFolders = mIniFile->value("Folders","").toString().split(";",Qt::SkipEmptyParts);
        mOptions.includeVersionInfo = mIniFile->value("IncludeVersionInfo", false).toBool();
        mOptions.supportXPThemes = mIniFile->value("SupportXPThemes", false).toBool();
        mOptions.compilerSet = mIniFile->value("CompilerSet", pSettings->compilerSets().defaultIndex()).toInt();

        if (mOptions.compilerSet >= pSettings->compilerSets().size()
                || mOptions.compilerSet < 0) { // TODO: change from indices to names
            QMessageBox::critical(
                        pMainWindow,
                        tr("Compiler not found"),
                        tr("The compiler set you have selected for this project, no longer exists.")
                        +"<BR />"
                        +tr("It will be substituted by the global compiler set."),
                        QMessageBox::Ok
                                  );
            mOptions.compilerSet = pSettings->compilerSets().defaultIndex();
            setModified(true);
        }
        mOptions.compilerOptions = mIniFile->value("CompilerSettings","").toString();
        mOptions.staticLink = mIniFile->value("StaticLink", true).toBool();
        mOptions.addCharset = mIniFile->value("AddCharset", true).toBool();
        bool useUTF8 = mIniFile->value("UseUTF8", false).toBool();
        if (useUTF8) {
            mOptions.encoding = mIniFile->value("Encoding", ENCODING_SYSTEM_DEFAULT).toString();
        } else {
            mOptions.encoding = mIniFile->value("Encoding", ENCODING_UTF8).toString();
        }
        mIniFile->endGroup();

        mIniFile->beginGroup("VersionInfo");
        mOptions.versionInfo.major = mIniFile->value("Major", 0).toInt();
        mOptions.versionInfo.minor = mIniFile->value("Minor", 1).toInt();
        mOptions.versionInfo.release = mIniFile->value("Release", 1).toInt();
        mOptions.versionInfo.build = mIniFile->value("Build", 1).toInt();
        mOptions.versionInfo.languageID = mIniFile->value("LanguageID", 0x0409).toInt();
        mOptions.versionInfo.charsetID = mIniFile->value("CharsetID", 0x04E4).toInt();
        mOptions.versionInfo.companyName = mIniFile->value("CompanyName","").toString();
        mOptions.versionInfo.fileVersion = mIniFile->value("FileVersion", "0.1").toString();
        mOptions.versionInfo.fileDescription = mIniFile->value("FileDescription",
          tr("Developed using the Red Panda Dev-C++ IDE")).toString();
        mOptions.versionInfo.internalName = mIniFile->value("InternalName","").toString();
        mOptions.versionInfo.legalCopyright = mIniFile->value("LegalCopyright","").toString();
        mOptions.versionInfo.legalTrademarks = mIniFile->value("LegalTrademarks","").toString();
        mOptions.versionInfo.originalFilename = mIniFile->value("OriginalFilename",
                                                                extractFileName(executable())).toString();
        mOptions.versionInfo.productName = mIniFile->value("ProductName", mName).toString();
        mOptions.versionInfo.productVersion = mIniFile->value("ProductVersion", "0.1.1.1").toString();
        mOptions.versionInfo.autoIncBuildNr = mIniFile->value("AutoIncBuildNr", false).toBool();
        mOptions.versionInfo.syncProduct = mIniFile->value("SyncProduct", false).toBool();
        mIniFile->endGroup();
    } else { // dev-c < 4
        mOptions.version = 2;
        mIniFile->beginGroup("Project");
        if (!mIniFile->value("NoConsole", true).toBool())
            mOptions.type = ProjectType::Console;
        else if (mIniFile->value("IsDLL", false).toBool())
            mOptions.type = ProjectType::DynamicLib;
        else
            mOptions.type = ProjectType::GUI;

        mOptions.privateResource = mIniFile->value("PrivateResource","").toString();
        mOptions.resourceIncludes = mIniFile->value("ResourceIncludes","").toString().split(";",Qt::SkipEmptyParts);
        mOptions.objFiles = mIniFile->value("ObjFiles","").toString().split(";",Qt::SkipEmptyParts);
        mOptions.includes = mIniFile->value("IncludeDirs","").toString().split(";",Qt::SkipEmptyParts);
        mOptions.compilerCmd = mIniFile->value("CompilerOptions","").toString();
        mOptions.useGPP = mIniFile->value("Use_GPP", false).toBool();
        mOptions.exeOutput = mIniFile->value("ExeOutput","").toString();
        mOptions.objectOutput = mIniFile->value("ObjectOutput","").toString();
        mOptions.overrideOutput = mIniFile->value("OverrideOutput", false).toBool();
        mOptions.overridenOutput = mIniFile->value("OverrideOutputName","").toString();
        mOptions.hostApplication = mIniFile->value("HostApplication","").toString();
        mIniFile->endGroup();
    }
}

void Project::loadUnitLayout(Editor *e, int index)
{
    if (!e)
        return;
    QSettings layIni(changeFileExt(filename(), "layout"), QSettings::IniFormat);
    layIni.beginGroup(QString("Editor_%1").arg(index));
    e->setCaretY(layIni.value("CursorRow",1).toInt());
    e->setCaretX(layIni.value("CursorCol",1).toInt());
    e->setTopLine(layIni.value("TopLine",1).toInt());
    e->setLeftChar(layIni.value("LeftChar",1).toInt());
    layIni.endGroup();
}

PCppParser Project::cppParser()
{
    return mParser;
}

void Project::sortUnitsByPriority()
{
    mModel.beginUpdate();
    auto action = finally([this]{
        mModel.endUpdate();
    });
    std::sort(mUnits.begin(),mUnits.end(),[](const PProjectUnit& u1, const PProjectUnit& u2)->bool{
        return (u1->priority()>u2->priority());
    });
    rebuildNodes();
}

void Project::sortUnitsByAlpha()
{
    mModel.beginUpdate();
    auto action = finally([this]{
        mModel.endUpdate();
    });
    std::sort(mUnits.begin(),mUnits.end(),[](const PProjectUnit& u1, const PProjectUnit& u2)->bool{
        return (extractFileName(u1->fileName())<extractFileName(u2->fileName()));
    });
    rebuildNodes();
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

void Project::removeFolderRecurse(PFolderNode node)
{
    if (!node)
        return ;
    // Recursively remove folders
    for (int i=node->children.count()-1;i>=0;i++) {
        PFolderNode childNode = node->children[i];
        // Remove folder inside folder
        if (childNode->unitIndex<0 && childNode->level>0) {
            removeFolderRecurse(childNode);
        // Or remove editors at this level
        } else if (childNode->unitIndex >= 0 && childNode->level > 0) {
            // Remove editor in folder from project
            int editorIndex = childNode->unitIndex;
            if (!removeEditor(editorIndex,true))
                return;
        }
    }

    PFolderNode parent = node->parent.lock();
    if (parent) {
        parent->children.removeAll(node);
    }
}

void Project::updateFolderNode(PFolderNode node)
{
    for (int i=0;i<node->children.count();i++){
        PFolderNode child;
        if (child->unitIndex<0) {
            mFolders.append(getFolderPath(child));
            updateFolderNode(child);
        }
    }
}

const QList<PProjectUnit> &Project::units() const
{
    return mUnits;
}

const ProjectOptions &Project::options() const
{
    return mOptions;
}

void Project::setOptions(const ProjectOptions &newOptions)
{
    mOptions = newOptions;
}

ProjectModel *Project::model()
{
    return &mModel;
}

const PFolderNode &Project::node() const
{
    return mNode;
}

void Project::setNode(const PFolderNode &newNode)
{
    mNode = newNode;
}

const QString &Project::name() const
{
    return mName;
}

void Project::setName(const QString &newName)
{
    mName = newName;
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
        mNode->text = extractFileName(mFileName);
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

ProjectModel::ProjectModel(Project *project, QObject *parent):
    QAbstractItemModel(parent),
    mProject(project)
{
    mUpdateCount = 0;
}

void ProjectModel::beginUpdate()
{
    if (mUpdateCount==0)
        beginResetModel();
    mUpdateCount++;
}

void ProjectModel::endUpdate()
{
    mUpdateCount--;
    if (mUpdateCount==0)
        endResetModel();
}

QModelIndex ProjectModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row,column,mProject->node().get());
    }
    FolderNode* parentNode = static_cast<FolderNode*>(parent.internalPointer());
    if (!parentNode) {
        return QModelIndex();
    }
    return createIndex(row,column,parentNode->children[row].get());
}

QModelIndex ProjectModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();
    FolderNode * node = static_cast<FolderNode*>(child.internalPointer());
    if (!node)
        return QModelIndex();
    PFolderNode parent = node->parent.lock();
    if (!parent) // root node
        return QModelIndex();
    PFolderNode grand = parent->parent.lock();
    if (!grand) {
        return createIndex(0,0,parent.get());
    }

    int row = grand->children.indexOf(parent);
    if (row<0)
        return QModelIndex();
    return createIndex(row,0,parent.get());
}

int ProjectModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 1;
    FolderNode* p = static_cast<FolderNode*>(parent.internalPointer());
    if (p) {
        return p->children.count();
    } else {
        return mProject->node()->children.count();
    }
}

int ProjectModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    FolderNode* p = static_cast<FolderNode*>(index.internalPointer());
    if (!p)
        return QVariant();
    if (role == Qt::DisplayRole) {
        return p->text;
    }
    return QVariant();
}
