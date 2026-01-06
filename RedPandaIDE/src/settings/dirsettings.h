#ifndef DIR_SETTINGS_H
#define DIR_SETTINGS_H
#include "basesettings.h"

#define SETTING_DIRS "Dirs"

class DirSettings: public BaseSettings {
public:
    enum class DataType {
        None,
        ColorScheme,
        IconSet,
        Theme,
        Template
    };
    explicit DirSettings(SettingsPersistor * persistor);
    QString appDir() const;
    QString appResourceDir() const;
    QString appLibexecDir() const;
    QString projectDir() const;
    QString data(DataType dataType = DataType::None) const;
    QString config(DataType dataType = DataType::None) const;
    QString executable() const;

    void setProjectDir(const QString &newProjectDir);

protected:
    void doSave() override;
    void doLoad() override;
private:
    QString mProjectDir;
};


#endif
//DIR_SETTINGS_H
