#ifndef VCS_SETTINGS_H
#define VCS_SETTINGS_H
#include "basesettings.h"

#define SETTING_VCS "VCS"

class VCSSettings: public BaseSettings {
public:
    explicit VCSSettings(SettingsPersistor *persistor);
    const QString &gitPath() const;
    void setGitPath(const QString &newGitPath);
    bool gitOk() const;
    void detectGitInPath();
private:
    void validateGit();
private:
    QString mGitPath;
    bool mGitOk;
protected:
    void doSave() override;
    void doLoad() override;
};


#endif
//VCS_SETTINGS_H
