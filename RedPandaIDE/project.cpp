#include "project.h"
#include "editor.h"
#include "mainwindow.h"
#include "utils.h"
#include "systemconsts.h"

#include <QDir>
#include <QFileInfo>

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
    if fOptions.UseCustomMakefile then
      Result := fOptions.CustomMakefile
    else
      Result := Directory + DEV_MAKE_FILE;
}

const std::weak_ptr<Project> &ProjectUnit::parent() const
{
    return mParent;
}

void ProjectUnit::setParent(const std::weak_ptr<Project> &newParent)
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

bool ProjectUnit::modified()
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
        std::shared_ptr<Project> parent = mParent.lock();
        if (parent) {
            parent->setModified(true);
        }
    }
}

bool ProjectUnit::save()
{
    bool previous=pMainWindow->fileSystemWatcher()->blockSignals(true);
    auto action = finally([&previous](){
        pMainWindow->fileSystemWatcher()->blockSignals(previous);
    });
    bool result=true;
    if (!mEditor && !QFile(mFileName).exists()) {
        // file is neither open, nor saved
        QStringList temp;
        StringsToFile(temp,mFileName);
    } else if (mEditor and modified()) {
        result = mEditor->save();
    }
    return result;
}
