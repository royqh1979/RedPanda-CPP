#include "vcssettings.h"
#include "../systemconsts.h"
#include <QDir>
#include <QProcessEnvironment>
#include <QSet>

VCSSettings::VCSSettings(SettingsPersistor *persistor):
    BaseSettings{persistor,SETTING_VCS},
    mGitOk(false)
{
}

void VCSSettings::doSave()
{
    saveValue("git_path",mGitPath);
}

void VCSSettings::doLoad()
{
    setGitPath(stringValue("git_path", ""));
}

const QString &VCSSettings::gitPath() const
{
    return mGitPath;
}

void VCSSettings::setGitPath(const QString &newGitPath)
{
    if (mGitPath!=newGitPath) {
        mGitPath = newGitPath;
        validateGit();
    }
}

void VCSSettings::validateGit()
{
    mGitOk = false;
    QFileInfo fileInfo(mGitPath);
    if (!fileInfo.exists()) {
        return;
    }
    mGitOk=true;
//    QStringList args;
//    args.append("--version");
//    QString output = runAndGetOutput(
//                fileInfo.fileName(),
//                fileInfo.absolutePath(),
//                args);
//    mGitOk = output.startsWith("git version");
}

bool VCSSettings::gitOk() const
{
    return mGitOk;
}

void VCSSettings::detectGitInPath()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList pathList = path.split(PATH_SEPARATOR);
    QSet<QString> searched;
    foreach (const QString& s, pathList){
        if (searched.contains(s))
            continue;;
        searched.insert(s);
        QDir dir(s);
        if (dir.exists(GIT_PROGRAM)) {
            QString oldPath = mGitPath;
            setGitPath(dir.filePath(GIT_PROGRAM));
            validateGit();
            if (mGitOk) {
                save();
                return;
            } else {
                mGitPath = oldPath;
            }
        }

    }
}
