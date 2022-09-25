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
#include "project.h"
#include "editor.h"
#include "utils.h"
#include "systemconsts.h"
#include "editorlist.h"
#include <parser/cppparser.h>
#include "utils.h"
#include "charsetinfo.h"
#include "projecttemplate.h"
#include "systemconsts.h"
#include "iconsmanager.h"

#include <QFileSystemWatcher>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextCodec>
#include <QMessageBox>
#include <QDirIterator>
#include <QMimeDatabase>
#include <QDesktopServices>
#include "customfileiconprovider.h"
#include <QMimeData>
#include "settings.h"
#include "vcs/gitrepository.h"

Project::Project(const QString &filename, const QString &name,
                 EditorList* editorList,
                 QFileSystemWatcher* fileSystemWatcher,
                 QObject *parent) :
    QObject(parent),
    mModel(this),
    mEditorList(editorList),
    mFileSystemWatcher(fileSystemWatcher)
{
    mFilename = QFileInfo(filename).absoluteFilePath();
    mParser = std::make_shared<CppParser>();
    mParser->setOnGetFileStream(
                std::bind(
                    &EditorList::getContentFromOpenedEditor,mEditorList,
                    std::placeholders::_1, std::placeholders::_2));
    if (name == DEV_INTERNAL_OPEN) {
        mModified = false;
        open();
    } else {
        mName = name;
        SimpleIni ini;
        ini.SetValue("Project","filename", toByteArray(extractRelativePath(directory(),mFilename)));
        ini.SetValue("Project","name", toByteArray(mName));
        ini.SaveFile(mFilename.toLocal8Bit());
        mRootNode = makeProjectNode();
    }
    resetCppParser(mParser,mOptions.compilerSet);
}

Project::~Project()
{
    mEditorList->beginUpdate();
    foreach (const PProjectUnit& unit, mUnits) {
        Editor * editor = unitEditor(unit);
        if (editor) {
            mEditorList->forceCloseEditor(editor);
        }
    }
    mEditorList->endUpdate();
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
        if (unit->modified()) {
            return true;
        }
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

    mRootNode = makeProjectNode();

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
//        if (!QFileInfo(newUnit->fileName()).exists()) {
//            QMessageBox::critical(nullptr,
//                                  tr("File Not Found"),
//                                  tr("Project file '%1' can't be found!")
//                                  .arg(newUnit->fileName()),
//                                  QMessageBox::Ok);
//            newUnit->setModified(true);
//        } else {
        newUnit->setFileMissing(!QFileInfo(newUnit->fileName()).exists());
        newUnit->setFolder(fromByteArray(ini.GetValue(groupName,"Folder","")));
        newUnit->setCompile(ini.GetBoolValue(groupName,"Compile", true));
        newUnit->setCompileCpp(
                    ini.GetBoolValue(groupName,"CompileCpp",mOptions.isCpp));

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
        newUnit->setNew(false);
        newUnit->setParent(this);
        newUnit->setNode(makeNewFileNode(extractFileName(newUnit->fileName()), false, folderNodeFromName(newUnit->folder())));
        newUnit->node()->unitIndex = mUnits.count();
        mUnits.append(newUnit);
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
    if (mModified!=value) {
        mModified=value;
        emit modifyChanged(mModified);
    }
}

PProjectModelNode Project::makeNewFileNode(const QString &s, bool isFolder, PProjectModelNode newParent)
{
    PProjectModelNode node = std::make_shared<ProjectModelNode>();
    if (!newParent) {
        newParent = mRootNode;
    }
    newParent->children.append(node);
    node->parent = newParent;
    node->text = s;
    if (newParent) {
        node->level = newParent->level+1;
    }
    if (isFolder) {
        node->unitIndex = -1;
        node->priority = 0;
        node->folderNodeType = ProjectSpecialFolderNode::NonSpecial;
    } else {
        node->folderNodeType = ProjectSpecialFolderNode::NotFolder;
    }
    return node;
}

PProjectModelNode Project::makeProjectNode()
{
    PProjectModelNode node = std::make_shared<ProjectModelNode>();
    node->text = mName;
    node->level = 0;
    node->unitIndex = -1;
    node->folderNodeType = ProjectSpecialFolderNode::NonSpecial;
    return node;
}

PProjectUnit Project::newUnit(PProjectModelNode parentNode, const QString& customFileName)
{
    PProjectUnit newUnit = std::make_shared<ProjectUnit>(this);

    // Select folder to add unit to
    if (!parentNode)
        parentNode = mRootNode; // project root node

    if (parentNode->unitIndex>=0) { //it's a file
        parentNode = mRootNode;
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
    newUnit->setFolder(getFolderPath(parentNode));
    newUnit->setNode(makeNewFileNode(extractFileName(newUnit->fileName()),
                                     false, parentNode));
    newUnit->node()->unitIndex = count;
    //parentNode.Expand(True);
    switch(getFileType(customFileName)) {
    case FileType::CSource:
        newUnit->setCompile(true);
        newUnit->setCompileCpp(false);
        newUnit->setLink(true);
        break;
    case FileType::CppSource:
        newUnit->setCompile(true);
        newUnit->setCompileCpp(true);
        newUnit->setLink(true);
        break;
    case FileType::WindowsResourceSource:
        newUnit->setCompile(true);
        newUnit->setCompileCpp(mOptions.isCpp);
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
    newUnit->setModified(true);
    newUnit->setEncoding(toByteArray(mOptions.encoding));
    return newUnit;
}

Editor *Project::openUnit(int index, bool forceOpen)
{
    if ((index < 0) || (index >= mUnits.count()))
        return nullptr;

    PProjectUnit unit = mUnits[index];

    if (!unit->fileName().isEmpty() && fileExists(unit->fileName())) {
        if (getFileType(unit->fileName())==FileType::Other) {
            if (forceOpen)
                QDesktopServices::openUrl(QUrl::fromLocalFile(unit->fileName()));
            return nullptr;
        }

        Editor * editor = mEditorList->getOpenedEditorByFilename(unit->fileName());
        if (editor) {//already opened in the editors
            editor->setInProject(true);
            editor->activate();
            return editor;
        }
        QByteArray encoding;
        encoding = unit->encoding();
        editor = mEditorList->newEditor(unit->fileName(), encoding, true, unit->isNew());
        if (editor) {
            editor->setInProject(true);
            //unit->setEncoding(encoding);
            editor->activate();
            loadUnitLayout(editor,index);
            return editor;
        }
    }
    return nullptr;
}

Editor *Project::unitEditor(const PProjectUnit &unit) const
{
    return mEditorList->getOpenedEditorByFilename(unit->fileName());
}

Editor *Project::unitEditor(const ProjectUnit *unit) const
{
    return mEditorList->getOpenedEditorByFilename(unit->fileName());
}

void Project::rebuildNodes()
{
    mModel.beginUpdate();
    // Delete everything
    mRootNode->children.clear();
    mFolderNodes.clear();
    mSpecialNodes.clear();
    mFileSystemFolderNodes.clear();

    // Recreate everything
    switch(mOptions.modelType) {
    case ProjectModelType::Custom:
        createFolderNodes();

        for (int idx=0;idx<mUnits.count();idx++) {
            QFileInfo fileInfo(mUnits[idx]->fileName());
            mUnits[idx]->setNode(
                        makeNewFileNode(
                            fileInfo.fileName(),
                            false,
                            folderNodeFromName(mUnits[idx]->folder())
                            )
                        );
            mUnits[idx]->node()->unitIndex = idx;
            mUnits[idx]->node()->priority = mUnits[idx]->priority();
        }
        break;
    case ProjectModelType::FileSystem:
        createFileSystemFolderNodes();

        for (int idx=0;idx<mUnits.count();idx++) {
            QFileInfo fileInfo(mUnits[idx]->fileName());
            mUnits[idx]->setNode(
                        makeNewFileNode(
                            fileInfo.fileName(),
                            false,
                            getParentFolderNode(
                                mUnits[idx]->fileName())
                            )
                        );
            mUnits[idx]->node()->unitIndex = idx;
            mUnits[idx]->node()->priority = mUnits[idx]->priority();
        }

        break;
    }

    mModel.endUpdate();
    emit nodesChanged();
}

bool Project::removeUnit(int index, bool doClose , bool removeFile)
{
    mModel.beginUpdate();
    auto action = finally([this]{
        mModel.endUpdate();
    });
    if (index<0 || index>=mUnits.count())
        return false;

    PProjectUnit unit = mUnits[index];

//    qDebug()<<unit->fileName();
//    qDebug()<<(qint64)unit->editor();
    // Attempt to close it
    if (doClose) {
        Editor* editor = unitEditor(unit);
        if (editor) {
            editor->setInProject(false);
            mEditorList->closeEditor(editor);
        }
    }

    if (removeFile) {
        QFile::remove(unit->fileName());
    }

//if not fUnits.GetItem(index).fNew then
    PProjectModelNode node = unit->node();
    PProjectModelNode parent = node->parent.lock();
    if (parent) {
        parent->children.removeAll(node);
    }
    mUnits.removeAt(index);
    updateNodeIndexes();
    setModified(true);
    return true;
}

bool Project::removeFolder(PProjectModelNode node)
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
    foreach (const QString& s, mOptions.includeDirs) {
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
    SimpleIni layIni;
    QStringList sl;
    // Write list of open project files
    for (int i=0;i<mEditorList->pageCount();i++) {
        Editor* e= (*mEditorList)[i];
        if (e && e->inProject())
            sl.append(QString("%1").arg(indexInUnits(e)));
    }
    layIni.SetValue("Editors","Order",sl.join(",").toUtf8());

    Editor *e, *e2;
    // Remember what files were visible
    mEditorList->getVisibleEditors(e, e2);
    if (e)
        layIni.SetLongValue("Editors","Focused", indexInUnits(e));
    // save editor info
    for (int i=0;i<mUnits.count();i++) {
        QByteArray groupName = QString("Editor_%1").arg(i).toUtf8();
        PProjectUnit unit = mUnits[i];
        Editor* editor = unitEditor(unit);
        if (editor) {
            layIni.SetLongValue(groupName,"CursorCol", editor->caretX());
            layIni.SetLongValue(groupName,"CursorRow", editor->caretY());
            layIni.SetLongValue(groupName,"TopLine", editor->topLine());
            layIni.SetLongValue(groupName,"LeftChar", editor->leftChar());
        }
        // remove old data from project file
//        SimpleIni ini;
//        ini.LoadFile(filename().toLocal8Bit());
//        groupName = toByteArray(QString("Unit%1").arg(i+1));
//        ini.Delete(groupName,"Open");
//        ini.Delete(groupName,"Top");
//        ini.Delete(groupName,"CursorCol");
//        ini.Delete(groupName,"CursorRow");
//        ini.Delete(groupName,"TopLine");
//        ini.Delete(groupName,"LeftChar");
//        ini.SaveFile(filename().toLocal8Bit());
    }
    layIni.SaveFile(changeFileExt(filename(), "layout").toLocal8Bit());
}

void Project::saveUnitAs(int i, const QString &sFileName, bool syncEditor)
{
    if ((i < 0) || (i >= mUnits.count()))
        return;
    PProjectUnit unit = mUnits[i];
    if (fileExists(unit->fileName())) {
        unit->setNew(false);
    }
    Editor * editor=unitEditor(unit);
    if (editor && syncEditor) {
        //prevent recurse
        editor->saveAs(sFileName,true);
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
    SimpleIni layIni;
    QByteArray groupName = (QString("Editor_%1").arg(index)).toUtf8();
    layIni.SetLongValue(groupName,"CursorCol", e->caretX());
    layIni.SetLongValue(groupName,"CursorRow", e->caretY());
    layIni.SetLongValue(groupName,"TopLine", e->topLine());
    layIni.SetLongValue(groupName,"LeftChar", e->leftChar());
    layIni.SaveFile((changeFileExt(filename(), "layout")).toLocal8Bit());
}

bool Project::saveUnits()
{
    int count = 0;
    SimpleIni ini;
    SI_Error error = ini.LoadFile(mFilename.toLocal8Bit());
    if (error != SI_Error::SI_OK)
        return false;
    for (int idx = 0; idx < mUnits.count(); idx++) {
        PProjectUnit unit = mUnits[idx];
        QByteArray groupName = toByteArray(QString("Unit%1").arg(count+1));
        if (!unit->FileMissing()) {
            bool rd_only = false;
            if (unit->modified() && fileExists(unit->fileName())
                && isReadOnly(unit->fileName())) {
                // file is read-only
                QMessageBox::critical(nullptr,
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
        if (unit->encoding() != options().encoding)
            ini.SetValue(groupName,"FileEncoding", toByteArray(unit->encoding()));
    }
    ini.SetLongValue("Project","UnitCount",count);
    ini.SaveFile(mFilename.toLocal8Bit());
    return true;
}

PProjectUnit Project::findUnitByFilename(const QString &filename)
{
    foreach(PProjectUnit unit, mUnits) {
        if (QString::compare(unit->fileName(),filename, PATH_SENSITIVITY)==0)
            return unit;
    }
    return PProjectUnit();
}

void Project::associateEditor(Editor *editor)
{
    if (!editor)
        return;
    PProjectUnit unit = findUnitByFilename(editor->filename());
    associateEditorToUnit(editor,unit);
}

void Project::associateEditorToUnit(Editor *editor, PProjectUnit unit)
{
    if (!unit) {
        if (editor)
            editor->setInProject(false);
        return;
    }
    if (editor) {
        unit->setEncoding(editor->encodingOption());
        editor->setInProject(true);
    }
}

//bool Project::setCompileOption(const QString &key, int valIndex)
//{
//    Settings::PCompilerSet pSet = pSettings->compilerSets().getSet(mOptions.compilerSet);
//    if (!pSet)
//        return false;
//    PCompilerOption op = CompilerInfoManager::getCompilerOption(
//                pSet->compilerType(), key);
//    if (!op)
//        return false;
//    if (op->choices.isEmpty()) {
//        if (valIndex>0)
//            mOptions.compilerOptions.insert(key,COMPILER_OPTION_ON);
//        else
//            mOptions.compilerOptions.insert(key,"");
//    } else {
//        if (valIndex>0 && valIndex <= op->choices.length()) {
//            mOptions.compilerOptions.insert(key,op->choices[valIndex-1].second);
//        } else {
//            mOptions.compilerOptions.insert(key,"");
//        }
//    }
//    return true;
//}

bool Project::setCompileOption(const QString &key, const QString &value)
{
    Settings::PCompilerSet pSet = pSettings->compilerSets().getSet(mOptions.compilerSet);
    if (!pSet)
        return false;
    PCompilerOption op = CompilerInfoManager::getCompilerOption(
                pSet->compilerType(), key);
    if (!op)
        return false;
    mOptions.compilerOptions.insert(key,value);
    return true;
}

QString Project::getCompileOption(const QString &key) const
{
    return mOptions.compilerOptions.value(key,"");
}

void Project::updateFolders()
{
    mFolders.clear();
    updateFolderNode(mRootNode);
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

PProjectModelNode Project::pointerToNode(ProjectModelNode *p, PProjectModelNode parent)
{
    if (!p)
        return PProjectModelNode();
    if (!parent) {
        parent = mRootNode;
    }
    if (p==mRootNode.get())
        return mRootNode;
    foreach (const PProjectModelNode& node , parent->children) {
        if (node.get()==p)
            return node;
        PProjectModelNode result = pointerToNode(p,node);
        if (result)
            return result;
    }
    return PProjectModelNode();
}

void Project::setCompilerSet(int compilerSetIndex)
{
    if (mOptions.compilerSet != compilerSetIndex) {
        mOptions.compilerSet = compilerSetIndex;
        updateCompilerSetType();
        setModified(true);
    }
}

bool Project::assignTemplate(const std::shared_ptr<ProjectTemplate> aTemplate, bool useCpp)
{
    if (!aTemplate) {
        return true;
    }
    mOptions = aTemplate->options();
    mOptions.compilerSet = pSettings->compilerSets().defaultIndex();
    mOptions.isCpp = useCpp;
    updateCompilerSetType();
    mOptions.icon = aTemplate->icon();

    // Copy icon to project directory
    if (!mOptions.icon.isEmpty()) {
        QString originIcon = QFileInfo(aTemplate->fileName()).absoluteDir().absoluteFilePath(mOptions.icon);
        if (fileExists(originIcon)) {
            QString destIcon = QFileInfo(mFilename).absoluteDir().absoluteFilePath("app.ico");
            QFile::copy(originIcon,destIcon);
            mOptions.icon = destIcon;
        } else {
            mOptions.icon = "";
        }
    }
    // Add list of files
    if (aTemplate->version() > 0) {
        QDir dir(aTemplate->folder());
        for (int i=0;i<aTemplate->unitCount();i++) {
            // Pick file contents
            PTemplateUnit templateUnit = aTemplate->unit(i);
            if (!templateUnit->Source.isEmpty()) {
                QString target = templateUnit->Source;
                PProjectUnit unit;
                if (!templateUnit->Target.isEmpty())
                    target = templateUnit->Target;
                QFile::copy(
                            dir.absoluteFilePath(templateUnit->Source),
                            includeTrailingPathDelimiter(this->directory())+target);
                unit = newUnit(mRootNode, target);
            } else {
                QString s;
                PProjectUnit unit;
                if (mOptions.isCpp) {
                    s = templateUnit->CppText;
                    unit = newUnit(mRootNode, templateUnit->CppName);
                } else {
                    s = templateUnit->CText;
                    unit = newUnit(mRootNode,templateUnit->CName);
                }

                Editor * editor = mEditorList->newEditor(
                            unit->fileName(),
                            unit->encoding(),
                            true,
                            true);

                QString s2 = dir.absoluteFilePath(s);
                if (fileExists(s2) && !s.isEmpty()) {
                    try {
                        editor->loadFile(s2);
                    } catch(FileError& e) {
                        QMessageBox::critical(nullptr,
                                              tr("Error Load File"),
                                              e.reason());
                    }
                } else {
                    s.replace("#13#10","\r\n");
                    editor->insertString(s,false);
                }
                editor->save(true,false);
                editor->activate();
            }
        }
    }
    rebuildNodes();
    return true;
}

bool Project::saveAsTemplate(const QString &templateFolder,
                             const QString& name,
                             const QString& description,
                             const QString& category)
{
    QDir dir(templateFolder);
    if (!dir.mkpath(templateFolder)) {
        QMessageBox::critical(nullptr,
                              tr("Error"),
                              tr("Can't create folder %1 ").arg(templateFolder),
                              QMessageBox::Ok);
        return false;
    }

    QString fileName = dir.absoluteFilePath(TEMPLATE_INFO_FILE);
    PSimpleIni ini = std::make_shared<SimpleIni>();

    ini->SetLongValue("Template","Ver",3);
    // template info
    ini->SetValue("Template", "Name", name.toUtf8());
    ini->SetValue("Template", "Category", category.toUtf8());
    ini->SetValue("Template", "Description", description.toUtf8());
    if (fileExists(mOptions.icon)) {
        QString iconName = extractFileName(mOptions.icon);
        copyFile(mOptions.icon, dir.absoluteFilePath(iconName),true);
        if (dir.exists(iconName))
            ini->SetValue("Template", "Icon", iconName.toUtf8());
    }

    ini->SetLongValue("Project", "Type", static_cast<int>(mOptions.type));
    if (!mOptions.objFiles.isEmpty())
        ini->SetValue("Project", "ObjFiles", mOptions.objFiles.join(";").toUtf8());
    if (!mOptions.includeDirs.isEmpty())
        ini->SetValue("Project", "Includes", mOptions.includeDirs.join(";").toUtf8());
    if (!mOptions.resourceIncludes.isEmpty())
        ini->SetValue("Project", "ResourceIncludes", mOptions.resourceIncludes.join(";").toUtf8());
    if (!mOptions.binDirs.isEmpty())
        ini->SetValue("Project", "Bins", mOptions.binDirs.join(";").toUtf8());
    if (!mOptions.libDirs.isEmpty())
        ini->SetValue("Project", "Libs", mOptions.libDirs.join(";").toUtf8());
    if (!mOptions.compilerCmd.isEmpty())
        ini->SetValue("Project", "Compiler", mOptions.compilerCmd.toUtf8());
    if (!mOptions.cppCompilerCmd.isEmpty())
        ini->SetValue("Project", "CppCompiler", mOptions.cppCompilerCmd.toUtf8());
    if (!mOptions.linkerCmd.isEmpty())
        ini->SetValue("Project", "Linker",mOptions.linkerCmd.toUtf8());
    ini->SetBoolValue("Project", "IsCpp", mOptions.isCpp);
    if (mOptions.includeVersionInfo)
        ini->SetBoolValue("Project", "IncludeVersionInfo", true);
    if (mOptions.supportXPThemes)
        ini->SetBoolValue("Project", "SupportXPThemes", true);
    if (!mOptions.exeOutput.isEmpty())
        ini->SetValue("Project", "ExeOutput", mOptions.exeOutput.toUtf8());
    if (!mOptions.objectOutput.isEmpty())
        ini->SetValue("Project", "ObjectOutput", mOptions.objectOutput.toUtf8());
    if (!mOptions.logOutput.isEmpty())
        ini->SetValue("Project", "LogOutput", mOptions.logOutput.toUtf8());
    if (mOptions.execEncoding!=ENCODING_SYSTEM_DEFAULT)
        ini->SetValue("Project","ExecEncoding", mOptions.execEncoding);

//    if (!mOptions.staticLink)
//        ini->SetBoolValue("Project", "StaticLink",false);
    if (!mOptions.addCharset)
        ini->SetBoolValue("Project", "AddCharset",false);
    if (mOptions.encoding!=ENCODING_AUTO_DETECT)
        ini->SetValue("Project","Encoding",mOptions.encoding.toUtf8());
    if (mOptions.modelType!=ProjectModelType::FileSystem)
        ini->SetLongValue("Project", "ModelType", (int)mOptions.modelType);

    for (int i=0;i<mUnits.count();i++) {
        const PProjectUnit& unit=mUnits[i];
        QString unitName = extractFileName(unit->fileName());
        QByteArray section = toByteArray(QString("Unit%1").arg(i));
        if (!copyFile(unit->fileName(), dir.absoluteFilePath(unitName),true)) {
            QMessageBox::warning(nullptr,
                                  tr("Warning"),
                                  tr("Can't save file %1").arg(dir.absoluteFilePath(unitName)),
                                  QMessageBox::Ok);
        }
        switch(getFileType(unit->fileName())) {
        case FileType::CSource:
            ini->SetValue(section,"C", unitName.toUtf8());
            ini->SetValue(section,"CName", unitName.toUtf8());
            break;
        case FileType::CppSource:
            ini->SetValue(section,"Cpp", unitName.toUtf8());
            ini->SetValue(section,"CppName", unitName.toUtf8());
            break;
        case FileType::CHeader:
        case FileType::CppHeader:
            ini->SetValue(section,"C", unitName.toUtf8());
            ini->SetValue(section,"CName", unitName.toUtf8());
            ini->SetValue(section,"Cpp", unitName.toUtf8());
            ini->SetValue(section,"CppName", unitName.toUtf8());
            break;
        default:
            ini->SetValue(section,"Source", unitName.toUtf8());
            ini->SetValue(section,"Target", unitName.toUtf8());
        }
    }
    ini->SetLongValue("Project","UnitCount",mUnits.count());
    if (ini->SaveFile(fileName.toLocal8Bit())!=SI_OK) {
        QMessageBox::critical(nullptr,
                              tr("Error"),
                              tr("Can't save file %1").arg(fileName),
                              QMessageBox::Ok);
        return false;
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
    ini.SetLongValue("Project","Ver", 3); // Is 3 as of Red Panda C++.0
    ini.SetValue("Project","ObjFiles", toByteArray(mOptions.objFiles.join(";")));
    ini.SetValue("Project","Includes", toByteArray(mOptions.includeDirs.join(";")));
    ini.SetValue("Project","Libs", toByteArray(mOptions.libDirs.join(";")));
    ini.SetValue("Project","Bins", toByteArray(mOptions.binDirs.join(";")));
    ini.SetValue("Project","PrivateResource", toByteArray(mOptions.privateResource));
    ini.SetValue("Project","ResourceIncludes", toByteArray(mOptions.resourceIncludes.join(";")));
    ini.SetValue("Project","MakeIncludes", toByteArray(mOptions.makeIncludes.join(";")));
    ini.SetValue("Project","Compiler", toByteArray(mOptions.compilerCmd));
    ini.SetValue("Project","CppCompiler", toByteArray(mOptions.cppCompilerCmd));
    ini.SetValue("Project","Linker", toByteArray(mOptions.linkerCmd));
    ini.SetLongValue("Project","IsCpp", mOptions.isCpp);
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
    ini.Delete("Project","CompilerSettings"); // remove old compiler settings
    ini.Delete("CompilerSettings",nullptr); // remove old compiler settings
    foreach (const QString& key, mOptions.compilerOptions.keys()) {
        ini.SetValue("CompilerSettings",toByteArray(key),toByteArray(mOptions.compilerOptions.value(key)));
    }
    ini.SetLongValue("Project","StaticLink", mOptions.staticLink);
    ini.SetLongValue("Project","AddCharset", mOptions.addCharset);
    ini.SetValue("Project","ExecEncoding", mOptions.execEncoding);
    ini.SetValue("Project","Encoding",toByteArray(mOptions.encoding));
    ini.SetLongValue("Project","ModelType", (int)mOptions.modelType);
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

PProjectUnit Project::addUnit(const QString &inFileName, PProjectModelNode parentNode, bool rebuild)
{
    PProjectUnit newUnit;
    // Don't add if it already exists
    if (fileAlreadyExists(inFileName)) {
        QMessageBox::critical(nullptr,
                                 tr("File Exists"),
                                 tr("File '%1' is already in the project"),
                              QMessageBox::Ok);
        return newUnit;
    }
    newUnit = std::make_shared<ProjectUnit>(this);

    // Set all properties
    newUnit->setFileName(QDir(directory()).filePath(inFileName));
    newUnit->setNew(false);
    Editor * e= unitEditor(newUnit);
    if (e) {
        newUnit->setEncoding(e->fileEncoding());
        e->setInProject(true);
    } else {
        newUnit->setEncoding(options().encoding.toUtf8());
    }
    newUnit->setFolder(getFolderPath(parentNode));
    newUnit->setNode(makeNewFileNode(extractFileName(newUnit->fileName()), false, parentNode));
    newUnit->node()->unitIndex = mUnits.count();
    mUnits.append(newUnit);

  // Determine compilation flags
    switch(getFileType(inFileName)) {
    case FileType::CSource:
        newUnit->setCompile(true);
        newUnit->setCompileCpp(false);
        newUnit->setLink(true);
        break;
    case FileType::CppSource:
        newUnit->setCompile(true);
        newUnit->setCompileCpp(true);
        newUnit->setLink(true);
        break;
    case FileType::WindowsResourceSource:
        newUnit->setCompile(true);
        newUnit->setCompileCpp(mOptions.isCpp);
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
        rebuildNodes();
    }
    setModified(true);
    return newUnit;
}

QString Project::folder()
{
    return extractFileDir(filename());
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
    contents.append("/* THIS FILE WILL BE OVERWRITTEN BY Red Panda C++ */");
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
        stringsToFile(contents,rcFile);
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
        stringsToFile(content,executable() + ".Manifest");
    } else if (fileExists(executable() + ".Manifest"))
        QFile::remove(executable() + ".Manifest");

    // create private header file
    QString hFile = changeFileExt(rcFile, H_EXT);
    contents.clear();
    QString def = extractFileName(rcFile);
    def.replace(".","_");
    contents.append("/* THIS FILE WILL BE OVERWRITTEN BY Red Panda C++ */");
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
    stringsToFile(contents,hFile);
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
        sl = oldRes.split(';',
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            Qt::SkipEmptyParts
#else
            QString::SkipEmptyParts
#endif
                          );
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
                    nullptr,
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
    Editor * editor =unitEditor(unit);
    if (editor) {
        saveUnitLayout(editor,index);
        editor->setInProject(false);
        mEditorList->forceCloseEditor(editor);
    }
}

void Project::createFolderNodes()
{
    for (int idx=0;idx<mFolders.count();idx++) {
        PProjectModelNode node = mRootNode;
        QString s = mFolders[idx];
        int i = s.indexOf('/');
        while (i>=0) {
            PProjectModelNode findnode;
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

static void addFolderRecursively(QSet<QString>& folders, QString folder) {
    if (folder.isEmpty())
        return;
    folders.insert(excludeTrailingPathDelimiter(folder));
    QString parentFolder = QFileInfo(folder).absolutePath();
    if (parentFolder==folder)
        return;
    addFolderRecursively(folders, parentFolder);
}

void Project::createFileSystemFolderNodes()
{
    QSet<QString> headerFolders;
    QSet<QString> sourceFolders;
    QSet<QString> otherFolders;
    for (int idx=0;idx<mUnits.count();idx++) {
        QFileInfo fileInfo(mUnits[idx]->fileName());
        if (isHFile(fileInfo.fileName())) {
            addFolderRecursively(headerFolders,fileInfo.absolutePath());
        } else if (isCFile(fileInfo.fileName())) {
            addFolderRecursively(sourceFolders,fileInfo.absolutePath());
        } else {
            addFolderRecursively(otherFolders,fileInfo.absolutePath());
        }
    }
    PProjectModelNode node = makeNewFileNode(tr("Headers"),true,mRootNode);
    node->folderNodeType = ProjectSpecialFolderNode::HEADERS;
    node->priority = 1000;
    createFileSystemFolderNode(ProjectSpecialFolderNode::HEADERS,folder(),node, headerFolders);
    mFolderNodes.append(node);
    mSpecialNodes.insert(ProjectSpecialFolderNode::HEADERS,node);

    node = makeNewFileNode(tr("Sources"),true,mRootNode);
    node->folderNodeType = ProjectSpecialFolderNode::SOURCES;
    node->priority = 900;
    createFileSystemFolderNode(ProjectSpecialFolderNode::SOURCES,folder(),node, sourceFolders);
    mFolderNodes.append(node);
    mSpecialNodes.insert(ProjectSpecialFolderNode::SOURCES,node);

    node = makeNewFileNode(tr("Others"),true,mRootNode);
    node->folderNodeType = ProjectSpecialFolderNode::OTHERS;
    node->priority = 800;
    createFileSystemFolderNode(ProjectSpecialFolderNode::OTHERS,folder(),node, otherFolders);
    mFolderNodes.append(node);
    mSpecialNodes.insert(ProjectSpecialFolderNode::OTHERS,node);
}

void Project::createFileSystemFolderNode(
        ProjectSpecialFolderNode folderType,
        const QString &folderName,
        PProjectModelNode parent,
        const QSet<QString>& validFolders)
{
    QDirIterator iter(folderName);
    while (iter.hasNext()) {
        iter.next();
        QFileInfo fileInfo = iter.fileInfo();
        if (fileInfo.isHidden() || fileInfo.fileName().startsWith('.'))
            continue;
        if (fileInfo.isDir() && validFolders.contains(fileInfo.absoluteFilePath())) {
            PProjectModelNode node = makeNewFileNode(fileInfo.fileName(),true,parent);
            mFileSystemFolderNodes.insert(QString("%1/%2").arg((int)folderType).arg(fileInfo.absoluteFilePath()),node);
            createFileSystemFolderNode(folderType,fileInfo.absoluteFilePath(), node, validFolders);
        }
    }
}

void Project::doAutoOpen()
{
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

PProjectModelNode Project::findFolderNode(const QString &folderPath, ProjectSpecialFolderNode nodeType)
{
    PProjectModelNode node = mFileSystemFolderNodes.value(QString("%1/%2").arg((int)nodeType).arg(folderPath),
                                                          PProjectModelNode());
    if (node)
        return node;
    PProjectModelNode parentNode = mSpecialNodes.value(nodeType,PProjectModelNode());
    if (parentNode)
        return parentNode;
    return mRootNode;
}

PProjectModelNode Project::folderNodeFromName(const QString &name)
{
    int index = mFolders.indexOf(name);
    if (index>=0) {
        return mFolderNodes[index];
    }
    return mRootNode;
}

QString Project::getFolderPath(PProjectModelNode node)
{
    QString result;
    if (!node)
        return result;

    if (node->unitIndex>=0) // not a folder
        return result;

    PProjectModelNode p = node;
    while (p && p->unitIndex==-1 && p!=mRootNode) {
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

PProjectModelNode Project::getParentFolderNode(const QString &filename)
{
    QFileInfo fileInfo(filename);
    ProjectSpecialFolderNode folderNodeType;
    if (isHFile(fileInfo.fileName())) {
        folderNodeType = ProjectSpecialFolderNode::HEADERS;
    } else if (isCFile(fileInfo.fileName())) {
        folderNodeType = ProjectSpecialFolderNode::SOURCES;
    } else {
        folderNodeType = ProjectSpecialFolderNode::OTHERS;
    }
    return findFolderNode(fileInfo.absolutePath(),folderNodeType);
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

void Project::loadLayout()
{
    SimpleIni layIni;
    SI_Error error = layIni.LoadFile(changeFileExt(filename(), "layout").toLocal8Bit());
    if (error!=SI_OK)
        return;
    int topLeft = layIni.GetLongValue("Editors","Focused",1);
    //TopRight := layIni.ReadInteger('Editors', 'FocusedRight', -1);
    QString temp =layIni.GetValue("Editors","Order", "");
    QStringList sl = temp.split(",",
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            Qt::SkipEmptyParts
#else
            QString::SkipEmptyParts
#endif
                                );

    foreach (const QString& s,sl) {
        bool ok;
        int currIdx = s.toInt(&ok);
        if (ok) {
            openUnit(currIdx);
        }
    }
    if (topLeft>=0 && topLeft<mUnits.count()) {
        Editor * editor = unitEditor(mUnits[topLeft]);
        if (editor)
            editor->activate();
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
        if (mOptions.version < 3) {
            mOptions.version = 3;
            QMessageBox::information(nullptr,
                                     tr("Settings need update"),
                                     tr("The compiler settings format of Red Panda C++ has changed.")
                                     +"<BR /><BR />"
                                     +tr("Please update your settings at Project >> Project Options >> Compiler and save your project."),
                                     QMessageBox::Ok);
        }

        mOptions.type = static_cast<ProjectType>(ini.GetLongValue("Project", "type", 0));
        mOptions.compilerCmd = fromByteArray(ini.GetValue("Project", "Compiler", ""));
        mOptions.cppCompilerCmd = fromByteArray(ini.GetValue("Project", "CppCompiler", ""));
        mOptions.linkerCmd = fromByteArray(ini.GetValue("Project", "Linker", ""));
        mOptions.objFiles = fromByteArray(ini.GetValue("Project", "ObjFiles", "")).split(";",
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
         Qt::SkipEmptyParts
#else
         QString::SkipEmptyParts
#endif
        );
        mOptions.binDirs = fromByteArray(ini.GetValue("Project", "Bins", "")).split(";",
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            Qt::SkipEmptyParts
#else
            QString::SkipEmptyParts
#endif
        );
        mOptions.libDirs = fromByteArray(ini.GetValue("Project", "Libs", "")).split(";",
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            Qt::SkipEmptyParts
#else
            QString::SkipEmptyParts
#endif
        );
        mOptions.includeDirs = fromByteArray(ini.GetValue("Project", "Includes", "")).split(";",
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            Qt::SkipEmptyParts
#else
            QString::SkipEmptyParts
#endif
        );
        mOptions.privateResource = fromByteArray(ini.GetValue("Project", "PrivateResource", ""));
        mOptions.resourceIncludes = fromByteArray(ini.GetValue("Project", "ResourceIncludes", "")).split(";",
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
         Qt::SkipEmptyParts
#else
         QString::SkipEmptyParts
#endif
        );
        mOptions.makeIncludes = fromByteArray(ini.GetValue("Project", "MakeIncludes", "")).split(";",
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
         Qt::SkipEmptyParts
#else
         QString::SkipEmptyParts
#endif
        );
        mOptions.isCpp = ini.GetBoolValue("Project", "IsCpp", false);
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
        mFolders = fromByteArray(ini.GetValue("Project", "Folders", "")).split(";",
        #if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
                   Qt::SkipEmptyParts
        #else
                   QString::SkipEmptyParts
        #endif
               );
        mOptions.includeVersionInfo = ini.GetBoolValue("Project", "IncludeVersionInfo", false);
        mOptions.supportXPThemes = ini.GetBoolValue("Project", "SupportXPThemes", false);
        mOptions.compilerSet = ini.GetLongValue("Project", "CompilerSet", pSettings->compilerSets().defaultIndex());
        mOptions.modelType = (ProjectModelType)ini.GetLongValue("Project", "ModelType", (int)ProjectModelType::Custom);

        if (mOptions.compilerSet >= (int)pSettings->compilerSets().size()
                || mOptions.compilerSet < 0) { // TODO: change from indices to names
            QMessageBox::critical(
                        nullptr,
                        tr("Compiler not found"),
                        tr("The compiler set you have selected for this project, no longer exists.")
                        +"<BR />"
                        +tr("It will be substituted by the global compiler set."),
                        QMessageBox::Ok
                                  );
            setCompilerSet(pSettings->compilerSets().defaultIndex());
        }

        Settings::PCompilerSet pSet = pSettings->compilerSets().getSet(mOptions.compilerSet);
        if (pSet) {
            QByteArray oldCompilerOptions = ini.GetValue("Project", "CompilerSettings", "");
            if (!oldCompilerOptions.isEmpty()) {
                //version 2 compatibility
                // test if it is created by old dev-c++
                SimpleIni::TNamesDepend oKeys;
                ini.GetAllKeys("Project", oKeys);
                bool isNewDev=false;
                for(const SimpleIni::Entry& entry:oKeys) {
                    QString key(entry.pItem);
                    if (key=="UsePrecompiledHeader"
                            || key == "CompilerSetType"
                            || key == "StaticLink"
                            || key == "AddCharset"
                            || key == "ExecEncoding"
                            || key == "Encoding"
                            || key == "UseUTF8") {
                        isNewDev = true;
                        break;
                    }
                }
                if (!isNewDev && oldCompilerOptions.length()>=25) {
                    char t = oldCompilerOptions[18];
                    oldCompilerOptions[18]=oldCompilerOptions[21];
                    oldCompilerOptions[21]=t;
                }
                for (int i=0;i<oldCompilerOptions.length();i++) {
                    QString key = pSettings->compilerSets().getKeyFromCompilerCompatibleIndex(i);
                    PCompilerOption pOption = CompilerInfoManager::getCompilerOption(
                                pSet->compilerType(), key);
                    if (pOption) {
                        int val = Settings::CompilerSet::charToValue(oldCompilerOptions[i]);
                        if (pOption->choices.isEmpty()) {
                            if (val>0)
                                mOptions.compilerOptions.insert(key,COMPILER_OPTION_ON);
                            else
                                mOptions.compilerOptions.insert(key,"");
                        } else {
                            if (val>0 && val <= pOption->choices.length())
                                mOptions.compilerOptions.insert(key,pOption->choices[val-1].second);
                            else
                                mOptions.compilerOptions.insert(key,"");
                        }
                    }
                }
            } else {
                //version 3
                SimpleIni::TNamesDepend oKeys;
                ini.GetAllKeys("CompilerSettings", oKeys);
                for(const SimpleIni::Entry& entry:oKeys) {
                    QString key(entry.pItem);
                    mOptions.compilerOptions.insert(
                                    key,
                                    ini.GetValue("CompilerSettings", entry.pItem, ""));
                }
            }
        }

        mOptions.staticLink = ini.GetBoolValue("Project", "StaticLink", true);
        mOptions.execEncoding = ini.GetValue("Project","ExecEncoding", ENCODING_SYSTEM_DEFAULT);
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
          toByteArray(tr("Developed using the Red Panda C++ IDE"))));
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
        mOptions.version = 3;
        if (!ini.GetBoolValue("VersionInfo", "NoConsole", true))
            mOptions.type = ProjectType::Console;
        else if (ini.GetBoolValue("VersionInfo", "IsDLL", false))
            mOptions.type = ProjectType::DynamicLib;
        else
            mOptions.type = ProjectType::GUI;

        mOptions.privateResource = fromByteArray(ini.GetValue("Project", "PrivateResource", ""));
        mOptions.resourceIncludes = fromByteArray(ini.GetValue("Project", "ResourceIncludes", "")).split(";",
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
         Qt::SkipEmptyParts
#else
         QString::SkipEmptyParts
#endif
        );
        mOptions.objFiles = fromByteArray(ini.GetValue("Project", "ObjFiles", "")).split(";",
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
         Qt::SkipEmptyParts
#else
         QString::SkipEmptyParts
#endif
        );
        mOptions.includeDirs = fromByteArray(ini.GetValue("Project", "IncludeDirs", "")).split(";",
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
           Qt::SkipEmptyParts
#else
           QString::SkipEmptyParts
#endif
        );
        mOptions.compilerCmd = fromByteArray(ini.GetValue("Project", "CompilerOptions", ""));
        mOptions.isCpp = ini.GetBoolValue("Project", "Use_GPP", false);
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
    SimpleIni layIni;
    SI_Error error;
    error = layIni.LoadFile(changeFileExt(filename(), "layout").toLocal8Bit());
    if (error != SI_Error::SI_OK)
        return;
    QByteArray groupName = (QString("Editor_%1").arg(index)).toUtf8();
    e->setCaretY(layIni.GetLongValue(groupName,"CursorRow",1));
    e->setCaretX(layIni.GetLongValue(groupName,"CursorCol",1));
    e->setTopLine(layIni.GetLongValue(groupName,"TopLine",1));
    e->setLeftChar(layIni.GetLongValue(groupName,"LeftChar",1));
}

PCppParser Project::cppParser()
{
    return mParser;
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

void Project::removeFolderRecurse(PProjectModelNode node)
{
    if (!node)
        return ;
    // Recursively remove folders
    for (int i=node->children.count()-1;i>=0;i++) {
        PProjectModelNode childNode = node->children[i];
        // Remove folder inside folder
        if (childNode->unitIndex<0 && childNode->level>0) {
            removeFolderRecurse(childNode);
        // Or remove editors at this level
        } else if (childNode->unitIndex >= 0 && childNode->level > 0) {
            // Remove editor in folder from project
            int editorIndex = childNode->unitIndex;
            if (!removeUnit(editorIndex,true))
                return;
        }
    }

    PProjectModelNode parent = node->parent.lock();
    if (parent) {
        parent->children.removeAll(node);
    }
}

void Project::updateFolderNode(PProjectModelNode node)
{
    for (int i=0;i<node->children.count();i++){
        PProjectModelNode child = node->children[i];
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
        mOptions.compilerOptions = defaultSet->compileOptions();
    } else {
        mOptions.compilerSetType=CST_DEBUG;
        mOptions.staticLink = false;
    }
}

QFileSystemWatcher *Project::fileSystemWatcher() const
{
    return mFileSystemWatcher;
}

QString Project::fileSystemNodeFolderPath(const PProjectModelNode &node)
{
    QString result;
    if (node != mRootNode) {
        PProjectModelNode pNode = node;
        while (pNode && pNode->folderNodeType == ProjectSpecialFolderNode::NonSpecial) {
            result = node->text + "/" +result;
            pNode = pNode->parent.lock();
        }
    }
    result = folder() + "/" + result;
    return result;
}

QStringList Project::binDirs()
{
    QStringList lst = options().binDirs;
    Settings::PCompilerSet compilerSet = pSettings->compilerSets().getSet(options().compilerSet);
    if (compilerSet) {
        lst.append(compilerSet->binDirs());
    }
    return lst;
}

EditorList *Project::editorList() const
{
    return mEditorList;
}

const QList<PProjectUnit> &Project::units() const
{
    return mUnits;
}

ProjectModelType Project::modelType() const
{
    return mOptions.modelType;
}

void Project::setModelType(ProjectModelType type)
{
    if (type!=mOptions.modelType) {
        mOptions.modelType = type;
        rebuildNodes();
    }
}

ProjectOptions &Project::options()
{
    return mOptions;
}

ProjectModel *Project::model()
{
    return &mModel;
}

const PProjectModelNode &Project::rootNode() const
{
    return mRootNode;
}

const QString &Project::name() const
{
    return mName;
}

void Project::setName(const QString &newName)
{
    if (newName != mName) {
        mName = newName;
        mRootNode->text = newName;
        setModified(true);
    }
}

const QString &Project::filename() const
{
    return mFilename;
}

ProjectUnit::ProjectUnit(Project* parent)
{
    mNode = nullptr;
    mParent = parent;
    mFileMissing = false;
}

Project *ProjectUnit::parent() const
{
    return mParent;
}

void ProjectUnit::setParent(Project* newParent)
{
    mParent = newParent;
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
    if (mPriority!=newPriority) {
        mPriority = newPriority;
        if (mNode)
            mNode->priority = mPriority;
    }
}

const QByteArray &ProjectUnit::encoding() const
{
    return mEncoding;
}

void ProjectUnit::setEncoding(const QByteArray &newEncoding)
{
    if (mEncoding != newEncoding) {
        Editor * editor=mParent->unitEditor(this);
        if (editor) {
            editor->setEncodingOption(newEncoding);
        }
        mEncoding = newEncoding;
    }
}

bool ProjectUnit::modified() const
{
    Editor * editor=mParent->unitEditor(this);
    if (editor) {
        return editor->modified();
    } else {
        return false;
    }
}

void ProjectUnit::setModified(bool value)
{
    Editor * editor=mParent->unitEditor(this);
    // Mark the change in the coupled editor
    if (editor) {
        return editor->setModified(value);
    }

    // If modified is set to true, mark project as modified too
    if (value) {
        mParent->setModified(true);
    }
}

bool ProjectUnit::save()
{
    bool previous=mParent->fileSystemWatcher()->blockSignals(true);
    auto action = finally([&previous,this](){
        mParent->fileSystemWatcher()->blockSignals(previous);
    });
    bool result=true;
    Editor * editor=mParent->unitEditor(this);
    if (!editor && !fileExists(mFileName)) {
        // file is neither open, nor saved
        QStringList temp;
        stringsToFile(temp,mFileName);
    } else if (editor && editor->modified()) {
        result = editor->save();
    }
    if (mNode) {
        mNode->text = extractFileName(mFileName);
    }
    return result;
}

PProjectModelNode &ProjectUnit::node()
{
    return mNode;
}

void ProjectUnit::setNode(const PProjectModelNode &newNode)
{
    mNode = newNode;
}

bool ProjectUnit::FileMissing() const
{
    return mFileMissing;
}

void ProjectUnit::setFileMissing(bool newDontSave)
{
    mFileMissing = newDontSave;
}

ProjectModel::ProjectModel(Project *project, QObject *parent):
    QAbstractItemModel(parent),
    mProject(project)
{
    mUpdateCount = 0;
    mIconProvider = new CustomFileIconProvider();
}

ProjectModel::~ProjectModel()
{
    delete mIconProvider;
}

void ProjectModel::beginUpdate()
{
    if (mUpdateCount==0) {
        beginResetModel();
    }
    mUpdateCount++;
}

void ProjectModel::endUpdate()
{
    mUpdateCount--;
    if (mUpdateCount==0) {
        mIconProvider->setRootFolder(mProject->folder());
        endResetModel();
    }
}

CustomFileIconProvider *ProjectModel::iconProvider() const
{
    return mIconProvider;
}

Project *ProjectModel::project() const
{
    return mProject;
}

QModelIndex ProjectModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row,column,mProject->rootNode().get());
    }
    ProjectModelNode* parentNode = static_cast<ProjectModelNode*>(parent.internalPointer());
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
    ProjectModelNode * node = static_cast<ProjectModelNode*>(child.internalPointer());
    if (!node)
        return QModelIndex();
    return getParentIndex(node);
}

int ProjectModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 1;
    ProjectModelNode* p = static_cast<ProjectModelNode*>(parent.internalPointer());
    if (p) {
        return p->children.count();
    } else {
        return mProject->rootNode()->children.count();
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
    ProjectModelNode* p = static_cast<ProjectModelNode*>(index.internalPointer());
    if (!p)
        return QVariant();
    if (role == Qt::DisplayRole) {
        if (p == mProject->rootNode().get()) {
            QString branch;
            if (mIconProvider->VCSRepository()->hasRepository(branch))
                return QString("%1 [%2]").arg(p->text,branch);
        }
        return p->text;
    } else if (role==Qt::EditRole) {
        return p->text;
    } else if (role == Qt::DecorationRole) {
        QIcon icon;
        if (p->unitIndex>=0) {
            icon = mIconProvider->icon(mProject->units()[p->unitIndex]->fileName());
        } else {
            if (p == mProject->rootNode().get()) {
                QString branch;
                if (mIconProvider->VCSRepository()->hasRepository(branch))
                    icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_GIT);
            } else {
                switch(p->folderNodeType) {
                case ProjectSpecialFolderNode::HEADERS:
                    icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_HEADERS_FOLDER);
                    break;
                case ProjectSpecialFolderNode::SOURCES:
                    icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_SOURCES_FOLDER);
                    break;
                default:
                    icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_FOLDER);
                }
            }
            if (icon.isNull())
                icon = mIconProvider->icon(QFileIconProvider::Folder);
        }
        return icon;
    }
    return QVariant();
}

Qt::ItemFlags ProjectModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    ProjectModelNode* p = static_cast<ProjectModelNode*>(index.internalPointer());
    if (!p)
        return Qt::NoItemFlags;
    if (p==mProject->rootNode().get())
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
    if (mProject && mProject->modelType() == ProjectModelType::FileSystem) {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (p->unitIndex>=0)
            flags.setFlag(Qt::ItemIsEditable);
        return flags;
    } else {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
        if (p->unitIndex<0) {
            flags.setFlag(Qt::ItemIsDropEnabled);
            flags.setFlag(Qt::ItemIsDragEnabled,false);
        }
        return flags;
    }
}

bool ProjectModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    ProjectModelNode* p = static_cast<ProjectModelNode*>(index.internalPointer());
    PProjectModelNode node = mProject->pointerToNode(p);
    if (!node)
        return false;
    if (role == Qt::EditRole) {
        if (node == mProject->rootNode()) {
            QString newName = value.toString().trimmed();
            if (newName.isEmpty())
                return false;
            mProject->setName(newName);
            emit dataChanged(index,index);
            return true;
        }
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
                if (QMessageBox::question(nullptr,
                                          tr("File exists"),
                                          tr("File '%1' already exists. Delete it now?")
                                          .arg(newName),
                                          QMessageBox::Yes | QMessageBox::No,
                                          QMessageBox::No) == QMessageBox::Yes) {
                    // Close the target file...
                    Editor * e=mProject->editorList()->getOpenedEditorByFilename(newName);
                    if (e)
                        mProject->editorList()->closeEditor(e);

                    // Remove it from the current project...
                    int projindex = mProject->indexInUnits(newName);
                    if (projindex>=0) {
                        mProject->removeUnit(projindex,false);
                    }

                    // All references to the file are removed. Delete the file from disk
                    if (!QFile::remove(newName)) {
                        QMessageBox::critical(nullptr,
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
            //save old file, if it is openned;
            // remove old file from monitor list
            mProject->fileSystemWatcher()->removePath(oldName);

            if (!QFile::rename(oldName,newName)) {
                QMessageBox::critical(nullptr,
                                      tr("Rename failed"),
                                      tr("Failed to rename file '%1' to '%2'")
                                      .arg(oldName,newName),
                                      QMessageBox::Ok);
                return false;
            }
            mProject->saveUnitAs(idx,newName);

            // Add new filename to file minitor
            mProject->fileSystemWatcher()->addPath(newName);

            //suffix changed
            if (mProject && mProject->modelType() == ProjectModelType::FileSystem
                    && QFileInfo(oldName).suffix()!=QFileInfo(newName).suffix()) {
                mProject->rebuildNodes();
            } else
                emit dataChanged(index,index);
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
            emit dataChanged(index,index);
            return true;
        }

    }
    return false;
}

QModelIndex ProjectModel::getParentIndex(ProjectModelNode * node) const
{
    PProjectModelNode parent = node->parent.lock();
    if (!parent) // root node
        return QModelIndex();
    PProjectModelNode grand = parent->parent.lock();
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
    ProjectModelNode* p= static_cast<ProjectModelNode*>(idx.internalPointer());
    PProjectModelNode node = mProject->pointerToNode(p);
    if (node->unitIndex>=0)
        return false;
    QByteArray encoded = data->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    while (!stream.atEnd()) {
        qint32 r, c;
        quintptr v;
        stream >> r >> c >> v;
        ProjectModelNode* droppedPointer= (ProjectModelNode*)(v);
        PProjectModelNode droppedNode = mProject->pointerToNode(droppedPointer);
        PProjectModelNode oldParent = droppedNode->parent.lock();
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
    ProjectModelNode* p= static_cast<ProjectModelNode*>(parent.internalPointer());
    PProjectModelNode node = mProject->pointerToNode(p);

    QByteArray encoded = data->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    QVector<int> rows,cols;
    QVector<intptr_t> pointers;
    while (!stream.atEnd()) {
        qint32 r, c;
        quintptr v;
        stream >> r >> c >> v;
        rows.append(r);
        cols.append(c);
        pointers.append(v);
    }
    for (int i=pointers.count()-1;i>=0;i--) {
        int r = rows[i];
        intptr_t v = pointers[i];
        ProjectModelNode* droppedPointer= (ProjectModelNode*)(v);
        PProjectModelNode droppedNode = mProject->pointerToNode(droppedPointer);
        PProjectModelNode oldParent = droppedNode->parent.lock();
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
        stream << (qint32)((*it).row()) << (qint32)((*it).column()) << (quintptr)((*it).internalPointer());
        ProjectModelNode* p = static_cast<ProjectModelNode*>((*it).internalPointer());
        if (p && p->unitIndex>=0) {
            urls.append(QUrl::fromLocalFile(mProject->units()[p->unitIndex]->fileName()));
        }
    }
    if (!urls.isEmpty())
        data->setUrls(urls);
    data->setData(format, encoded);
    return data;
}

ProjectModelSortFilterProxy::ProjectModelSortFilterProxy(QObject *parent):
    QSortFilterProxyModel(parent)
{

}

bool ProjectModelSortFilterProxy::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    if (!sourceModel())
        return false;
    ProjectModelNode* pLeft=nullptr;
    if (source_left.isValid())
        pLeft = static_cast<ProjectModelNode*>(source_left.internalPointer());
    ProjectModelNode* pRight=nullptr;
    if (source_right.isValid())
        pRight = static_cast<ProjectModelNode*>(source_right.internalPointer());
    if (!pLeft)
        return true;
    if (!pRight)
        return false;
    if (pLeft->unitIndex<0 && pRight->unitIndex>=0)
        return true;
    if (pLeft->unitIndex>=0 && pRight->unitIndex<0)
        return false;
    if (pLeft->priority!=pRight->priority)
        return pLeft->priority>pRight->priority;
    return QString::compare(pLeft->text, pRight->text)<0;
}
