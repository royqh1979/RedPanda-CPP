#ifndef ENVIRONMENT_SETTINGS_H
#define ENVIRONMENT_SETTINGS_H
#include "basesettings.h"

#define SETTING_ENVIRONMENT "Environment"

class DirSettings;
class EnvironmentSettings: public BaseSettings {
public:
    struct TerminalItem {
        QString name;
        QString terminal;
        QString param;
    };

    explicit EnvironmentSettings(SettingsPersistor * persistor, DirSettings *dirSettings);
    QString theme() const;
    void setTheme(const QString &theme);

    QString interfaceFont() const;
    void setInterfaceFont(const QString &interfaceFont);

    int interfaceFontSize() const;
    void setInterfaceFontSize(int interfaceFontSize);

    QString language() const;
    void setLanguage(const QString &language);

    const QString &currentFolder() const;
    void setCurrentFolder(const QString &newCurrentFolder);

    const QString &defaultOpenFolder() const;
    void setDefaultOpenFolder(const QString &newDefaultOpenFolder);

    const QString &iconSet() const;
    void setIconSet(const QString &newIconSet);

    QString terminalPath() const;
    void setTerminalPath(const QString &terminalPath);

    QString AStylePath() const;
    void setAStylePath(const QString &aStylePath);

    QString terminalArgumentsPattern() const;
    void setTerminalArgumentsPattern(const QString &argsPattern);

    bool useCustomIconSet() const;
    void setUseCustomIconSet(bool newUseCustomIconSet);

    bool hideNonSupportFilesInFileView() const;
    void setHideNonSupportFilesInFileView(bool newHideNonSupportFilesInFileView);

    bool openFilesInSingleInstance() const;
    void setOpenFilesInSingleInstance(bool newOpenFilesInSingleInstance);

    double iconZoomFactor() const;
    void setIconZoomFactor(double newIconZoomFactor);

    QJsonArray availableTerminals() const;
    void setAvailableTerminals(const QJsonArray &availableTerminals);

    QString queryPredefinedTerminalArgumentsPattern(const QString &executable) const;

    bool useCustomTerminal() const;
    void setUseCustomTerminal(bool newUseCustomTerminal);

    QList<TerminalItem> loadTerminalList() const;

    static QMap<QString, QString> terminalArgsPatternMagicVariables();

    bool comboboxWheel() const;
    void setComboboxWheel(bool newComboboxWheel);

private:
    bool isTerminalValid();
    void checkAndSetTerminal();
private:
    DirSettings *mDirSettings;
    //Appearance
    QString mTheme;
    QString mInterfaceFont;
    int mInterfaceFontSize;
    double mIconZoomFactor;
    QString mLanguage;
    QString mCurrentFolder;
    QString mIconSet;
    bool mUseCustomIconSet;
    bool mUseCustomTheme;
    bool mComboboxWheel;

    QString mDefaultOpenFolder;
    QString mTerminalPath;
    QString mAStylePath;
    QString mTerminalArgumentsPattern;

    bool mUseCustomTerminal;
    bool mHideNonSupportFilesInFileView;
    bool mOpenFilesInSingleInstance;

    static const QMap<QString, QString> mTerminalArgsPatternMagicVariables;
    // _Base interface
protected:
    void doSave() override;
    void doLoad() override;
};

#endif
//ENVIRONMENT_SETTINGS_H
