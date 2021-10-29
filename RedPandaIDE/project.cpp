#include "project.h"
#include "editor.h"
#include "mainwindow.h"
#include "utils.h"
#include "systemconsts.h"
#include "editorlist.h"
#include <parser/cppparser.h>
#include "utils.h"
#include "platform.h"
#include "projecttemplate.h"
#include "systemconsts.h"
#include "iconsmanager.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextCodec>
#include <QMessageBox>
#include <QFileIconProvider>
#include <QMimeData>
#include "settings.h"

Project::Project(const QString &filename, const QString &name, QObject *parent) :
    QObject(parent),
    mModel(this)
{
    mFilename = QFileInfo(filename).absoluteFilePath();
    mParser = std::make_shared<CppParser>();
    mParser->setOnGetFileStream(
                std::bind(
                    &EditorList::getContentFromOpenedEditor,pMainWindow->editorList(),
                    std::placeholders::_1, std::placeholders::_2));
    resetCppParser(mParser);
    if (name == DEV_INTERNAL_OPEN) {
        open();
        mModified = false;
    } else {
        mName = name;
        SimpleIni ini;
        ini.SetValue("Project","filename", toByteArray(extractRelativePath(directory(),mFilename)));
        ini.SetValue("Project","name", toByteArray(mName));
        ini.SaveFile(mFilename.toLocal8Bit());
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
//    QFile fileInfo(mFilename);
    SimpleIni ini;
    ini.LoadFile(mFilename.toLocal8Bit());
    loadOptions(ini);

    mNode = makeProjectNode();

    checkProjectFileForUpdate(ini);
    int uCount  = ini.GetLongValue("Project","UnitCount",0);
    createFolderNodes();
    QDir dir(directory());
    for (int i=0;i<uCount;i++) {
        PProjectUnit newUnit = std::make_shared<ProjectUnit>(this);
        QByteArray groupName = toByteArray(QString("Unit%1").arg(i+1));
        newUnit->setFileName(
                    dir.absoluteFilePath(
                        fromByteArray(ini.GetValue(groupName,"FileName",""))));
        if (!QFileInfo(newUnit->fileName()).exists()) {
            QMessageBox::critical(pMainWindow,
                                  tr("File Not Found"),
                                  tr("Project file '%1' can't be found!")
                                  .arg(newUnit->fileName()),
                                  QMessageBox::Ok);
            newUnit->setModified(true);
        } else {
            newUnit->setFolder(fromByteArray(ini.GetValue(groupName,"Folder","")));
            newUnit->setCompile(ini.GetBoolValue(groupName,"Compile", true));
            newUnit->setCompileCpp(
                        ini.GetBoolValue(groupName,"CompileCpp",mOptions.useGPP));

            newUnit->setLink(ini.GetBoolValue(groupName,"Link", true));
            newUnit->setPriority(ini.GetLongValue(groupName,"Priority", 1000));
            newUnit->setOverrideBuildCmd(ini.GetBoolValue(groupName,"OverrideBuildCmd", false));
            newUnit->setBuildCmd(fromByteArray(ini.GetValue(groupName,"BuildCmd", "")));
            QByteArray defaultEncoding = toByteArray(mOptions.encoding);
            if (ini.GetBoolValue(groupName,"DetectEncoding",true)){
                defaultEncoding = ENCODING_AUTO_DETECT;
            }
            newUnit->setEncoding(ini.GetValue(groupName, "FileEncoding",defaultEncoding));
            if (QTextCodec::codecForName(newUnit->encoding())==nullptr) {
                newUnit->setEncoding(ENCODING_AUTO_DETECT);
            }
            newUnit->setEditor(nullptr);
            newUnit->setNew(false);
            newUnit->setParent(this);
            newUnit->setNode(makeNewFileNode(extractFileName(newUnit->fileName()), false, folderNodeFromName(newUnit->folder())));
            newUnit->node()->unitIndex = mUnits.count();
            mUnits.append(newUnit);
        }
    }
    rebuildNodes();
}

void Project::setFileName(QString value)
{
    value = QFileInfo(value).absoluteFilePath();
    if (mFilename!=value) {
        QFile::rename(mFilename,value);
        mFilename = value;
        setModified(true);
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
    if (!newParent) {
        newParent = mNode;
    }
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
    node->unitIndex = -1;
    return node;
}

PProjectUnit Project::newUnit(PFolderNode parentNode, const QString& customFileName)
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
    int count = mUnits.count();
    mUnits.append(newUnit);

    // Set all properties
    newUnit->setFileName(s);
    newUnit->setNew(true);
    newUnit->setEditor(nullptr);
    newUnit->setFolder(getFolderPath(parentNode));
    newUnit->setNode(makeNewFileNode(extractFileName(newUnit->fileName()),
                                     false, parentNode));
    newUnit->node()->unitIndex = count;
    //parentNode.Expand(True);
    newUnit->setCompile(true);
    newUnit->setCompileCpp(mOptions.useGPP);
    newUnit->setLink(true);
    newUnit->setPriority(1000);
    newUnit->setOverrideBuildCmd(false);
    newUnit->setBuildCmd("");
    newUnit->setModified(true);
    newUnit->setEncoding(toByteArray(options().encoding));
    return newUnit;
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
            editor->activate();
            return editor;
        }
        QByteArray encoding;
        encoding = unit->encoding();
        editor = pMainWindow->editorList()->newEditor(fullPath, encoding, true, unit->isNew());
        editor->setInProject(true);
        unit->setEditor(editor);
        unit->setEncoding(encoding);
        editor->activate();
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
    QSettings layIni(changeFileExt(mFilename, "layout"),QSettings::IniFormat);
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
        SimpleIni ini;
        ini.LoadFile(mFilename.toLocal8Bit());
        QByteArray groupName = toByteArray(QString("Unit%1").arg(i+1));
        ini.Delete(groupName,"Open");
        ini.Delete(groupName,"Top");
        ini.Delete(groupName,"CursorCol");
        ini.Delete(groupName,"CursorRow");
        ini.Delete(groupName,"TopLine");
        ini.Delete(groupName,"LeftChar");
        ini.SaveFile(mFilename.toLocal8Bit());
    }
}

void Project::saveUnitAs(int i, const QString &sFileName, bool syncEditor)
{
    if ((i < 0) || (i >= mUnits.count()))
        return;
    PProjectUnit unit = mUnits[i];
    if (fileExists(unit->fileName())) {
        unit->setNew(false);
    }
    if (unit->editor() && syncEditor) {
        //prevent recurse
        unit->editor()->saveAs(sFileName,true);
    }
    unit->setNew(false);
    unit->setFileName(sFileName);
    SimpleIni ini;
    ini.LoadFile(mFilename.toLocal8Bit());
    QByteArray groupName = toByteArray(QString("Unit%1").arg(i+1));
    ini.SetValue(
                groupName,
                "FileName",
                toByteArray(
                    extractRelativePath(
                        directory(),
                        sFileName)));
    ini.SaveFile(mFilename.toLocal8Bit());
    setModified(true);
    if (!syncEditor) {
        //the call it's from editor, we need to update model
        mModel.beginUpdate();
        mModel.endUpdate();
    }
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
    SimpleIni ini;
    ini.LoadFile(mFilename.toLocal8Bit());
    for (int idx = 0; idx < mUnits.count(); idx++) {
        PProjectUnit unit = mUnits[idx];
        bool rd_only = false;
        QByteArray groupName = toByteArray(QString("Unit%1").arg(count+1));
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
        ini.SetValue(
                    groupName,
                    "FileName",
                    toByteArray(
                        extractRelativePath(
                            directory(),
                            unit->fileName())));
        count++;
        switch(getFileType(unit->fileName())) {
        case FileType::CHeader:
        case FileType::CSource:
        case FileType::CppHeader:
        case FileType::CppSource:
            ini.SetLongValue(groupName,"CompileCpp", unit->compileCpp());
            break;
        case FileType::WindowsResourceSource:
            unit->setFolder("Resources");
        default:
            break;
        }
        unit->setNew(false);
        ini.SetValue(groupName,"Folder", toByteArray(unit->folder()));
        ini.SetLongValue(groupName,"Compile", unit->compile());
        ini.SetLongValue(groupName,"Link", unit->link());
        ini.SetLongValue(groupName,"Priority", unit->priority());
        ini.SetLongValue(groupName,"OverrideBuildCmd", unit->overrideBuildCmd());
        ini.SetValue(groupName,"BuildCmd", toByteArray(unit->buildCmd()));
        ini.SetLongValue(groupName,"DetectEncoding", unit->encoding()==ENCODING_AUTO_DETECT);
        ini.SetValue(groupName,"FileEncoding", toByteArray(unit->encoding()));
    }
    ini.SetLongValue("Project","UnitCount",count);
    ini.SaveFile(mFilename.toLocal8Bit());
    return true;
}

void Project::setCompilerOption(const QString &optionString, char value)
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

PFolderNode Project::pointerToNode(FolderNode *p, PFolderNode parent)
{
    if (!parent) {
        parent = mNode;
    }
    if (p==mNode.get())
        return mNode;
    foreach (const PFolderNode& node , parent->children) {
        if (node.get()==p)
            return node;
        PFolderNode result = pointerToNode(p,node);
        if (result)
            return result;
    }
    return PFolderNode();
}

void Project::setCompilerSet(int compilerSetIndex)
{
    if (mOptions.compilerSet != compilerSetIndex) {
        mOptions.compilerSet = compilerSetIndex;
        updateCompilerSetType();
    }
}

bool Project::assignTemplate(const std::shared_ptr<ProjectTemplate> aTemplate)
{
    if (!aTemplate) {
        return true;
    }

    mOptions = aTemplate->options();
    mOptions.compilerSet = pSettings->compilerSets().defaultIndex();
    updateCompilerSetType();
    mOptions.icon = aTemplate->icon();

    // Copy icon to project directory
    if (!mOptions.icon.isEmpty()) {
        QString originIcon = QDir(pSettings->dirs().templateDir()).absoluteFilePath(mOptions.icon);
        if (fileExists(originIcon)) {
            QString destIcon = changeFileExt(mFilename,ICON_EXT);
            QFile::copy(originIcon,destIcon);
            mOptions.icon = destIcon;
        } else {
            mOptions.icon = "";
        }
    }
    // Add list of files
    if (aTemplate->version() > 0) {
        for (int i=0;i<aTemplate->unitCount();i++) {
            // Pick file contents
            PTemplateUnit templateUnit = aTemplate->unit(i);
            QString s;
            PProjectUnit unit;
            if (aTemplate->options().useGPP) {
                s = templateUnit->CppText;
                unit = newUnit(mNode, templateUnit->CppName);
            } else {
                s = templateUnit->CText;
                unit = newUnit(mNode,templateUnit->CName);
            }

            Editor * editor = pMainWindow->editorList()->newEditor(
                        QDir(directory()).absoluteFilePath(unit->fileName()),
                        unit->encoding(),
                        true,
                        true);

            QString s2 = QDir(pSettings->dirs().templateDir()).absoluteFilePath(s);
            if (fileExists(s2) && !s.isEmpty()) {
                try {
                    editor->loadFile(s2);
                } catch(FileError& e) {
                    QMessageBox::critical(pMainWindow,
                                          tr("Error Load File"),
                                          e.reason());
                }
            } else {
                s.replace("#13#10","\r\n");
                editor->insertString(s,false);
            }
            unit->setEditor(editor);
            editor->save(true,false);
            editor->activate();
        }
    }
    return true;
}

void Project::saveOptions()
{
    SimpleIni ini;
    ini.LoadFile(mFilename.toLocal8Bit());
    ini.SetValue("Project","FileName", toByteArray(extractRelativePath(directory(), mFilename)));
    ini.SetValue("Project","Name", toByteArray(mName));
    ini.SetLongValue("Project","Type", static_cast<int>(mOptions.type));
    ini.SetLongValue("Project","Ver", 3); // Is 3 as of Red Panda Dev-C++ 7.0
    ini.SetValue("Project","ObjFiles", toByteArray(mOptions.objFiles.join(";")));
    ini.SetValue("Project","Includes", toByteArray(mOptions.includes.join(";")));
    ini.SetValue("Project","Libs", toByteArray(mOptions.libs.join(";")));
    ini.SetValue("Project","PrivateResource", toByteArray(mOptions.privateResource));
    ini.SetValue("Project","ResourceIncludes", toByteArray(mOptions.resourceIncludes.join(";")));
    ini.SetValue("Project","MakeIncludes", toByteArray(mOptions.makeIncludes.join(";")));
    ini.SetValue("Project","Compiler", toByteArray(mOptions.compilerCmd));
    ini.SetValue("Project","CppCompiler", toByteArray(mOptions.cppCompilerCmd));
    ini.SetValue("Project","Linker", toByteArray(mOptions.linkerCmd));
    ini.SetLongValue("Project","IsCpp", mOptions.useGPP);
    ini.SetValue("Project","Icon", toByteArray(extractRelativePath(directory(), mOptions.icon)));
    ini.SetValue("Project","ExeOutput", toByteArray(mOptions.exeOutput));
    ini.SetValue("Project","ObjectOutput", toByteArray(mOptions.objectOutput));
    ini.SetValue("Project","LogOutput", toByteArray(mOptions.logOutput));
    ini.SetLongValue("Project","LogOutputEnabled", mOptions.logOutputEnabled);
    ini.SetLongValue("Project","OverrideOutput", mOptions.overrideOutput);
    ini.SetValue("Project","OverrideOutputName", toByteArray(mOptions.overridenOutput));
    ini.SetValue("Project","HostApplication", toByteArray(mOptions.hostApplication));
    ini.SetLongValue("Project","UseCustomMakefile", mOptions.useCustomMakefile);
    ini.SetValue("Project","CustomMakefile", toByteArray(mOptions.customMakefile));
    ini.SetLongValue("Project","UsePrecompiledHeader", mOptions.usePrecompiledHeader);
    ini.SetValue("Project","PrecompiledHeader", toByteArray(mOptions.precompiledHeader));
    ini.SetValue("Project","CommandLine", toByteArray(mOptions.cmdLineArgs));
    ini.SetValue("Project","Folders", toByteArray(mFolders.join(";")));
    ini.SetLongValue("Project","IncludeVersionInfo", mOptions.includeVersionInfo);
    ini.SetLongValue("Project","SupportXPThemes", mOptions.supportXPThemes);
    ini.SetLongValue("Project","CompilerSet", mOptions.compilerSet);
    ini.SetLongValue("Project","CompilerSetType", mOptions.compilerSetType);
    ini.SetValue("Project","CompilerSettings", mOptions.compilerOptions);
    ini.SetLongValue("Project","StaticLink", mOptions.staticLink);
    ini.SetLongValue("Project","AddCharset", mOptions.addCharset);
    ini.SetValue("Project","Encoding",toByteArray(mOptions.encoding));
    //for Red Panda Dev C++ 6 compatibility
    ini.SetLongValue("Project","UseUTF8",mOptions.encoding == ENCODING_UTF8);

    ini.SetLongValue("VersionInfo","Major", mOptions.versionInfo.major);
    ini.SetLongValue("VersionInfo","Minor", mOptions.versionInfo.minor);
    ini.SetLongValue("VersionInfo","Release", mOptions.versionInfo.release);
    ini.SetLongValue("VersionInfo","Build", mOptions.versionInfo.build);
    ini.SetLongValue("VersionInfo","LanguageID", mOptions.versionInfo.languageID);
    ini.SetLongValue("VersionInfo","CharsetID", mOptions.versionInfo.charsetID);
    ini.SetValue("VersionInfo","CompanyName", toByteArray(mOptions.versionInfo.companyName));
    ini.SetValue("VersionInfo","FileVersion", toByteArray(mOptions.versionInfo.fileVersion));
    ini.SetValue("VersionInfo","FileDescription", toByteArray(mOptions.versionInfo.fileDescription));
    ini.SetValue("VersionInfo","InternalName", toByteArray(mOptions.versionInfo.internalName));
    ini.SetValue("VersionInfo","LegalCopyright", toByteArray(mOptions.versionInfo.legalCopyright));
    ini.SetValue("VersionInfo","LegalTrademarks", toByteArray(mOptions.versionInfo.legalTrademarks));
    ini.SetValue("VersionInfo","OriginalFilename", toByteArray(mOptions.versionInfo.originalFilename));
    ini.SetValue("VersionInfo","ProductName", toByteArray(mOptions.versionInfo.productName));
    ini.SetValue("VersionInfo","ProductVersion", toByteArray(mOptions.versionInfo.productVersion));
    ini.SetLongValue("VersionInfo","AutoIncBuildNr", mOptions.versionInfo.autoIncBuildNr);
    ini.SetLongValue("VersionInfo","SyncProduct", mOptions.versionInfo.syncProduct);


    //delete outdated dev4 project options
    ini.Delete("Project","NoConsole");
    ini.Delete("Project","IsDLL");
    ini.Delete("Project","ResFiles");
    ini.Delete("Project","IncludeDirs");
    ini.Delete("Project","CompilerOptions");
    ini.Delete("Project","Use_GPP");

    ini.SaveFile(mFilename.toLocal8Bit());
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
        if (changeFileExt(rcFile, DEV_PROJECT_EXT) == mFilename) {
            QFileInfo fileInfo(mFilename);
            rcFile = includeTrailingPathDelimiter(fileInfo.absolutePath())
                    + fileInfo.baseName()
                    + "_private."
                    + RC_EXT;
        }
    } else {
        QFileInfo fileInfo(mFilename);
        rcFile = includeTrailingPathDelimiter(fileInfo.absolutePath())
                + fileInfo.baseName()
                + "_private."
                + RC_EXT;
    }
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
        QString icon = mOptions.icon;
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
                   QString("\"%1.%2.%3.%4\"")
                   .arg(mOptions.versionInfo.major)
                   .arg(mOptions.versionInfo.minor)
                   .arg(mOptions.versionInfo.release)
                   .arg(mOptions.versionInfo.build));
    contents.append(QString("#define VER_MAJOR\t%1").arg(mOptions.versionInfo.major));
    contents.append(QString("#define VER_MINOR\t%1").arg(mOptions.versionInfo.minor));
    contents.append(QString("#define VER_RELEASE\t%1").arg(mOptions.versionInfo.release));
    contents.append(QString("#define VER_BUILD\t%1").arg(mOptions.versionInfo.build));
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

void Project::checkProjectFileForUpdate(SimpleIni &ini)
{
    bool cnvt = false;
    int uCount = ini.GetLongValue("Project","UnitCount", 0);
    // check if using old way to store resources and fix it
    QString oldRes = QString::fromLocal8Bit(ini.GetValue("Project","Resources", ""));
    if (!oldRes.isEmpty()) {
        QFile::copy(mFilename,mFilename+".bak");
        QStringList sl;
        sl = oldRes.split(';',Qt::SkipEmptyParts);
        for (int i=0;i<sl.count();i++){
            const QString& s = sl[i];
            QByteArray groupName = toByteArray(QString("Unit%1").arg(uCount+i));
            ini.SetValue(groupName,"Filename", toByteArray(s));
            ini.SetValue(groupName,"Folder", "Resources");
            ini.SetLongValue(groupName,"Compile",true);
        }
        ini.SetLongValue("Project","UnitCount",uCount+sl.count());
        QString folders = QString::fromLocal8Bit(ini.GetValue("Project","Folders",""));
        if (!folders.isEmpty())
            folders += ",Resources";
        else
            folders = "Resources";
        ini.SetValue("Project","Folders",toByteArray(folders));
        cnvt = true;
        ini.Delete("Project","Resources");
        ini.Delete("Project","Focused");
        ini.Delete("Project","Order");
        ini.Delete("Project","DebugInfo");
        ini.Delete("Project","ProfileInfo");
        ini.SaveFile(mFilename.toLocal8Bit());
    }

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
            s.remove(0,i+1);
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
    loadLayout();
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

char Project::getCompilerOption(const QString &optionString)
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
    while (p && p->unitIndex==-1 && p!=mNode) {
        if (!result.isEmpty())
            result = p->text + "/" + result;
        else
            result = p->text;
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

void Project::loadOptions(SimpleIni& ini)
{
    mName = fromByteArray(ini.GetValue("Project","name", ""));
    QString icon = fromByteArray(ini.GetValue("Project", "icon", ""));
    if (icon.isEmpty()) {
        mOptions.icon = "";
    } else {
        mOptions.icon = QDir(directory()).absoluteFilePath(icon);
    }
    mOptions.version = ini.GetLongValue("Project", "Ver", 0);
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

        mOptions.type = static_cast<ProjectType>(ini.GetLongValue("Project", "type", 0));
        mOptions.compilerCmd = fromByteArray(ini.GetValue("Project", "Compiler", ""));
        mOptions.cppCompilerCmd = fromByteArray(ini.GetValue("Project", "CppCompiler", ""));
        mOptions.linkerCmd = fromByteArray(ini.GetValue("Project", "Linker", ""));
        mOptions.objFiles = fromByteArray(ini.GetValue("Project", "ObjFiles", "")).split(";",Qt::SkipEmptyParts);
        mOptions.libs = fromByteArray(ini.GetValue("Project", "Libs", "")).split(";",Qt::SkipEmptyParts);
        mOptions.includes = fromByteArray(ini.GetValue("Project", "Includes", "")).split(";",Qt::SkipEmptyParts);
        mOptions.privateResource = fromByteArray(ini.GetValue("Project", "PrivateResource", ""));
        mOptions.resourceIncludes = fromByteArray(ini.GetValue("Project", "ResourceIncludes", "")).split(";",Qt::SkipEmptyParts);
        mOptions.makeIncludes = fromByteArray(ini.GetValue("Project", "MakeIncludes", "")).split(";",Qt::SkipEmptyParts);
        mOptions.useGPP = ini.GetBoolValue("Project", "IsCpp", false);
        mOptions.exeOutput = fromByteArray(ini.GetValue("Project", "ExeOutput", ""));
        mOptions.objectOutput = fromByteArray(ini.GetValue("Project", "ObjectOutput", ""));
        mOptions.logOutput = fromByteArray(ini.GetValue("Project", "LogOutput", ""));
        mOptions.logOutputEnabled = ini.GetBoolValue("Project", "LogOutputEnabled", false);
        mOptions.overrideOutput = ini.GetBoolValue("Project", "OverrideOutput", false);
        mOptions.overridenOutput = fromByteArray(ini.GetValue("Project", "OverrideOutputName", ""));
        mOptions.hostApplication = fromByteArray(ini.GetValue("Project", "HostApplication", ""));
        mOptions.useCustomMakefile = ini.GetBoolValue("Project", "UseCustomMakefile", false);
        mOptions.customMakefile = fromByteArray(ini.GetValue("Project", "CustomMakefile", ""));
        mOptions.usePrecompiledHeader = ini.GetBoolValue("Project", "UsePrecompiledHeader", false);
        mOptions.precompiledHeader = fromByteArray(ini.GetValue("Project", "PrecompiledHeader", ""));
        mOptions.cmdLineArgs = fromByteArray(ini.GetValue("Project", "CommandLine", ""));
        mFolders = fromByteArray(ini.GetValue("Project", "Folders", "")).split(";",Qt::SkipEmptyParts);
        mOptions.includeVersionInfo = ini.GetBoolValue("Project", "IncludeVersionInfo", false);
        mOptions.supportXPThemes = ini.GetBoolValue("Project", "SupportXPThemes", false);
        mOptions.compilerSet = ini.GetLongValue("Project", "CompilerSet", pSettings->compilerSets().defaultIndex());

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
            int compilerSetType = ini.GetLongValue("Project","CompilerSetType",-1);
            if (compilerSetType>=0) {
                for (int i=0;i<pSettings->compilerSets().size();i++) {
                    Settings::PCompilerSet pSet = pSettings->compilerSets().getSet(i);
                    if (pSet && pSet->compilerSetType() == compilerSetType) {
                        mOptions.compilerSet = i;
                        break;
                    }
                }
            }
            setModified(true);
        }
        mOptions.compilerOptions = ini.GetValue("Project", "CompilerSettings", "");
        mOptions.staticLink = ini.GetBoolValue("Project", "StaticLink", true);
        mOptions.addCharset = ini.GetBoolValue("Project", "AddCharset", true);

        if (mOptions.compilerSetType<0) {
            updateCompilerSetType();
        }
        bool useUTF8 = ini.GetBoolValue("Project", "UseUTF8", false);
        if (useUTF8) {
            mOptions.encoding = fromByteArray(ini.GetValue("Project","Encoding", ENCODING_UTF8));
        } else {
            mOptions.encoding = fromByteArray(ini.GetValue("Project","Encoding", ENCODING_AUTO_DETECT));
        }

        mOptions.versionInfo.major = ini.GetLongValue("VersionInfo", "Major", 0);
        mOptions.versionInfo.minor = ini.GetLongValue("VersionInfo", "Minor", 1);
        mOptions.versionInfo.release = ini.GetLongValue("VersionInfo", "Release", 1);
        mOptions.versionInfo.build = ini.GetLongValue("VersionInfo", "Build", 1);
        mOptions.versionInfo.languageID = ini.GetLongValue("VersionInfo", "LanguageID", 0x0409);
        mOptions.versionInfo.charsetID = ini.GetLongValue("VersionInfo", "CharsetID", 0x04E4);
        mOptions.versionInfo.companyName = fromByteArray(ini.GetValue("VersionInfo", "CompanyName", ""));
        mOptions.versionInfo.fileVersion = fromByteArray(ini.GetValue("VersionInfo", "FileVersion", "0.1"));
        mOptions.versionInfo.fileDescription = fromByteArray(ini.GetValue("VersionInfo", "FileDescription",
          toByteArray(tr("Developed using the Red Panda Dev-C++ IDE"))));
        mOptions.versionInfo.internalName = fromByteArray(ini.GetValue("VersionInfo", "InternalName", ""));
        mOptions.versionInfo.legalCopyright = fromByteArray(ini.GetValue("VersionInfo", "LegalCopyright", ""));
        mOptions.versionInfo.legalTrademarks = fromByteArray(ini.GetValue("VersionInfo", "LegalTrademarks", ""));
        mOptions.versionInfo.originalFilename = fromByteArray(ini.GetValue("VersionInfo", "OriginalFilename",
                                                                toByteArray(extractFileName(executable()))));
        mOptions.versionInfo.productName = fromByteArray(ini.GetValue("VersionInfo", "ProductName", toByteArray(mName)));
        mOptions.versionInfo.productVersion = fromByteArray(ini.GetValue("VersionInfo", "ProductVersion", "0.1.1.1"));
        mOptions.versionInfo.autoIncBuildNr = ini.GetBoolValue("VersionInfo", "AutoIncBuildNr", false);
        mOptions.versionInfo.syncProduct = ini.GetBoolValue("VersionInfo", "SyncProduct", false);

    } else { // dev-c < 4
        mOptions.version = 2;
        if (!ini.GetBoolValue("VersionInfo", "NoConsole", true))
            mOptions.type = ProjectType::Console;
        else if (ini.GetBoolValue("VersionInfo", "IsDLL", false))
            mOptions.type = ProjectType::DynamicLib;
        else
            mOptions.type = ProjectType::GUI;

        mOptions.privateResource = fromByteArray(ini.GetValue("Project", "PrivateResource", ""));
        mOptions.resourceIncludes = fromByteArray(ini.GetValue("Project", "ResourceIncludes", "")).split(";",Qt::SkipEmptyParts);
        mOptions.objFiles = fromByteArray(ini.GetValue("Project", "ObjFiles", "")).split(";",Qt::SkipEmptyParts);
        mOptions.includes = fromByteArray(ini.GetValue("Project", "IncludeDirs", "")).split(";",Qt::SkipEmptyParts);
        mOptions.compilerCmd = fromByteArray(ini.GetValue("Project", "CompilerOptions", ""));
        mOptions.useGPP = ini.GetBoolValue("Project", "Use_GPP", false);
        mOptions.exeOutput = fromByteArray(ini.GetValue("Project", "ExeOutput", ""));
        mOptions.objectOutput = fromByteArray(ini.GetValue("Project", "ObjectOutput", ""));
        mOptions.overrideOutput = ini.GetBoolValue("Project", "OverrideOutput", false);
        mOptions.overridenOutput = fromByteArray(ini.GetValue("Project", "OverrideOutputName", ""));
        mOptions.hostApplication = fromByteArray(ini.GetValue("Project", "HostApplication", ""));
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
        PFolderNode child = node->children[i];
        if (child->unitIndex<0) {
            mFolders.append(getFolderPath(child));
            updateFolderNode(child);
        }
    }
}

void Project::updateCompilerSetType()
{
    Settings::PCompilerSet defaultSet = pSettings->compilerSets().getSet(mOptions.compilerSet);
    if (defaultSet) {
        mOptions.compilerSetType=defaultSet->compilerSetType();
        mOptions.staticLink = defaultSet->staticLink();
        mOptions.compilerOptions = defaultSet->iniOptions();
    } else {
        mOptions.compilerSetType=CST_DEBUG;
        mOptions.staticLink = false;
    }
}

const QList<PProjectUnit> &Project::units() const
{
    return mUnits;
}

ProjectOptions &Project::options()
{
    return mOptions;
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
    if (newName != mName) {
        mName = newName;
        mNode->text = newName;
    }
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

void ProjectUnit::setFileName(QString newFileName)
{
    newFileName = QFileInfo(newFileName).absoluteFilePath();
    if (mFileName != newFileName) {
        mFileName = newFileName;
        if (mNode) {
            mNode->text = extractFileName(mFileName);
        }
    }
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
    } else if (mEditor && mEditor->modified()) {
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
    if (row<0 || row>=parentNode->children.count())
        return QModelIndex();
    return createIndex(row,column,parentNode->children[row].get());
}

QModelIndex ProjectModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();
    FolderNode * node = static_cast<FolderNode*>(child.internalPointer());
    if (!node)
        return QModelIndex();
    return getParentIndex(node);
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

int ProjectModel::columnCount(const QModelIndex &) const
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
    if (role == Qt::DisplayRole || role==Qt::EditRole) {
        return p->text;
    } else if (role == Qt::DecorationRole) {
        QFileIconProvider provider;
        if (p->unitIndex>=0) {
            return provider.icon(mProject->units()[p->unitIndex]->fileName());
        } else {
            QIcon icon = provider.icon(QFileIconProvider::Folder);
            if (icon.isNull())
                return *(pIconsManager->folder());
            return icon;
        }
    }
    return QVariant();
}

Qt::ItemFlags ProjectModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    FolderNode* p = static_cast<FolderNode*>(index.internalPointer());
    if (!p)
        return Qt::NoItemFlags;
    if (p==mProject->node().get())
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    if (p->unitIndex<0) {
        flags.setFlag(Qt::ItemIsDropEnabled);
        flags.setFlag(Qt::ItemIsDragEnabled,false);
    }
    return flags;
}

bool ProjectModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    FolderNode* p = static_cast<FolderNode*>(index.internalPointer());
    PFolderNode node = mProject->pointerToNode(p);
    if (!node)
        return false;
    if (role == Qt::EditRole) {
        if (node == mProject->node())
            return false;
        int idx = node->unitIndex;
        if (idx >= 0) {
            //change unit name
            PProjectUnit unit = mProject->units()[idx];
            QString newName = value.toString().trimmed();
            if (newName.isEmpty())
                return false;
            if (newName ==  node->text)
                return false;
            QString oldName = unit->fileName();
            QString curDir = extractFilePath(oldName);
            newName = QDir(curDir).absoluteFilePath(newName);
            // Only continue if the user says so...
            if (fileExists(newName) && newName.compare(oldName, PATH_SENSITIVITY)!=0) {
                // don't remove when changing case for example
                if (QMessageBox::question(pMainWindow,
                                          tr("File exists"),
                                          tr("File '%1' already exists. Delete it now?")
                                          .arg(newName),
                                          QMessageBox::Yes | QMessageBox::No,
                                          QMessageBox::No) == QMessageBox::Yes) {
                    // Close the target file...
                    Editor * e= pMainWindow->editorList()->getOpenedEditorByFilename(newName);
                    if (e)
                        pMainWindow->editorList()->closeEditor(e);

                    // Remove it from the current project...
                    int projindex = mProject->indexInUnits(newName);
                    if (projindex>=0) {
                        mProject->removeEditor(projindex,false);
                    }

                    // All references to the file are removed. Delete the file from disk
                    if (!QFile::remove(newName)) {
                        QMessageBox::critical(pMainWindow,
                                              tr("Remove failed"),
                                              tr("Failed to remove file '%1'")
                                              .arg(newName),
                                              QMessageBox::Ok);
                        return false;
                    }
                } else {
                    return false;
                }
            }
            // Target filename does not exist anymore. Do a rename
            // change name in project file first (no actual file renaming on disk)
            mProject->saveUnitAs(idx,newName);

            // remove old file from monitor list
            pMainWindow->fileSystemWatcher()->removePath(oldName);

            // Finally, we can rename without issues
            if (!QFile::remove(oldName)){
                QMessageBox::critical(pMainWindow,
                                      tr("Remove failed"),
                                      tr("Failed to remove file '%1'")
                                      .arg(oldName),
                                      QMessageBox::Ok);
                mProject->saveUnitAs(idx,oldName);
                return false;
            }

            // Add new filename to file minitor
            pMainWindow->fileSystemWatcher()->removePath(oldName);
            return true;
        } else {
            //change folder name
            QString newName = value.toString().trimmed();
            if (newName.isEmpty())
                return false;
            if (newName ==  node->text)
                return false;
            node->text = newName;
            mProject->updateFolders();
            mProject->saveAll();
            return true;
        }

    }
    return false;
}

QModelIndex ProjectModel::getParentIndex(FolderNode * node) const
{
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

bool ProjectModel::canDropMimeData(const QMimeData * data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex &parent) const
{

    if (!data || action != Qt::MoveAction)
        return false;
    if (!parent.isValid())
        return false;
    // check if the format is supported
    QStringList types = mimeTypes();
    if (types.isEmpty())
        return false;
    QString format = types.at(0);
    if (!data->hasFormat(format))
        return false;

    QModelIndex idx = parent;
//    if (row >= rowCount(parent) || row < 0) {
//        return false;
//    } else {
//        idx= index(row,column,parent);
//    }
    FolderNode* p= static_cast<FolderNode*>(idx.internalPointer());
    PFolderNode node = mProject->pointerToNode(p);
    if (node->unitIndex>=0)
        return false;
    QByteArray encoded = data->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    while (!stream.atEnd()) {
        int r, c;
        intptr_t v;
        stream >> r >> c >> v;
        FolderNode* droppedPointer= (FolderNode*)(v);
        PFolderNode droppedNode = mProject->pointerToNode(droppedPointer);
        PFolderNode oldParent = droppedNode->parent.lock();
        if (oldParent == node)
            return false;
    }
    return true;
}

Qt::DropActions ProjectModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool ProjectModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex &parent)
{
    // check if the action is supported
    if (!data || action != Qt::MoveAction)
        return false;
    // check if the format is supported
    QStringList types = mimeTypes();
    if (types.isEmpty())
        return false;
    QString format = types.at(0);
    if (!data->hasFormat(format))
        return false;

    if (!parent.isValid())
        return false;
    FolderNode* p= static_cast<FolderNode*>(parent.internalPointer());
    PFolderNode node = mProject->pointerToNode(p);

    QByteArray encoded = data->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    QVector<int> rows,cols;
    QVector<intptr_t> pointers;
    while (!stream.atEnd()) {
        int r, c;
        intptr_t v;
        stream >> r >> c >> v;
        rows.append(r);
        cols.append(c);
        pointers.append(v);
    }
    for (int i=pointers.count()-1;i>=0;i--) {
        int r = rows[i];
        intptr_t v = pointers[i];
        FolderNode* droppedPointer= (FolderNode*)(v);
        PFolderNode droppedNode = mProject->pointerToNode(droppedPointer);
        PFolderNode oldParent = droppedNode->parent.lock();
        QModelIndex oldParentIndex = getParentIndex(droppedPointer);
        beginRemoveRows(oldParentIndex,r,r);
        if (oldParent)
            oldParent->children.removeAt(r);
        endRemoveRows();
        droppedNode->parent = node;
        node->children.append(droppedNode);
        if (droppedNode->unitIndex>=0) {
            PProjectUnit unit = mProject->units()[droppedNode->unitIndex];
            unit->setFolder(mProject->getFolderPath(node));
        }
        QModelIndex newParentIndex = getParentIndex(droppedPointer);
        beginInsertRows(newParentIndex,node->children.count()-1,node->children.count()-1);
        endInsertRows();
        mProject->saveAll();
        return true;
    }

    return false;
}

QMimeData *ProjectModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.count() <= 0)
        return nullptr;
    QStringList types = mimeTypes();
    if (types.isEmpty())
        return nullptr;
    QMimeData *data = new QMimeData();
    QString format = types.at(0);
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    QModelIndexList::ConstIterator it = indexes.begin();
    QList<QUrl> urls;
    for (; it != indexes.end(); ++it) {
        stream << (*it).row() << (*it).column() << (intptr_t)((*it).internalPointer());
        FolderNode* p = static_cast<FolderNode*>((*it).internalPointer());
        if (p && p->unitIndex>=0) {
            urls.append(QUrl::fromLocalFile(mProject->units()[p->unitIndex]->fileName()));
        }
    }
    if (!urls.isEmpty())
        data->setUrls(urls);
    data->setData(format, encoded);
    return data;
}
