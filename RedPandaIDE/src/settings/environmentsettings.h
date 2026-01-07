/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
