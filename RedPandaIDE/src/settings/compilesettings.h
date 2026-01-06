#ifndef COMPILE_SETTINGS_H
#define COMPILE_SETTINGS_H
#include "basesettings.h"

#define SETTING_COMPILE "Compile"

class CompileSettings: public BaseSettings {
public:
    explicit CompileSettings(SettingsPersistor *persistor);
    const QString &NASMPath() const;
    void setNASMPath(const QString &newNASMPath);
    bool NASMLinkCStandardLib() const;
    void setNASMLinkCStandardLib(bool newLinkCStandardLib);

    bool GASLinkCStandardLib() const;
    void setGASLinkCStandardLib(bool newGASLinkCStandardLib);

private:
    QString mNASMPath;
    bool mNASMLinkCStandardLib;
    bool mGASLinkCStandardLib;
    // _Base interface
protected:
    void doSave() override;
    void doLoad() override;

};


#endif
//COMPILE_SETTINGS_H
