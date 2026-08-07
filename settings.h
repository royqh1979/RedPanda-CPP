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
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <vector>
#include <memory>
#include <QColor>
#include <QString>
#include <QPair>
#include <QSize>

/**
 * use the following command to get gcc's default bin/library folders:
 * gcc -print-search-dirs
 */

#define SETTING_DIRS "Dirs"
#define SETTING_UI "UI"
#define SETTING_VIEW "View"
#define SETTING_HISTORY "History"

class Settings;

class Settings
{
private:

    class _Base {
    public:
        explicit _Base(Settings* settings, const QString& groupName);
        void beginGroup();
        void endGroup();
        void remove(const QString &key);
        void saveValue(const QString &key, const QVariant &value);
        void saveValue(const QString &key, const QSet<QString>& set);
        QVariant value(const QString &key, const QVariant& defaultValue);
        bool boolValue(const QString &key, bool defaultValue);
        QSize sizeValue(const QString &key, const QSize& size=QSize());
        int intValue(const QString &key, int defaultValue);
        double doubleValue(const QString& key, double defaultValue);
        unsigned int uintValue(const QString &key, unsigned int defaultValue);
        QStringList stringListValue(const QString &key, const QStringList& defaultValue=QStringList());
        QSet<QString> stringSetValue(const QString &key);
        QColor colorValue(const QString &key, const QColor& defaultValue);
        QString stringValue(const QString &key, const QString& defaultValue);
        void save();
        void load();
    protected:
        virtual void doSave() = 0;
        virtual void doLoad() = 0;
    protected:
        Settings* mSettings;
        QString mGroup;
    };

public:
    class Dirs: public _Base {
    public:
        explicit Dirs(Settings * settings);
        QString appDir() const;
        QString executable() const;

    protected:
        void doSave() override;
        void doLoad() override;
    };

    class UI: public _Base {
    public:
        explicit UI(Settings *settings);
        int settingsDialogWidth() const;
        void setSettingsDialogWidth(int newSettingsDialogWidth);

        int settingsDialogHeight() const;
        void setSettingsDialogHeight(int newSettingsDialogHeight);

        int settingsDialogSplitterPos() const;
        void setSettingsDialogSplitterPos(int newSettingsDialogSplitterPos);

        const QString &fontName() const;
        void setFontName(const QString &newFontName);

        int fontSize() const;
        void setFontSize(int newFontSize);

        bool showContentsPanel() const;
        void setShowContentsPanel(bool newShowContentsPanel);

        int mainWindowWidth() const;
        void setMainWindowWidth(int newMainWindowWidth);

        int mainWindowHeight() const;
        void setMainWindowHeight(int newMainWindowHeight);

        int mainWindowLeft() const;
        void setMainWindowLeft(int newMainWindowLeft);

        int mainWindowTop() const;
        void setMainWindowTop(int newMainWindowTop);

        int contentsPanelWidth() const;
        void setContentsPanelWidth(int newContentsPanelWidth);

    protected:
        void doSave() override;
        void doLoad() override;
    private:
        int mSettingsDialogWidth;
        int mSettingsDialogHeight;
        int mSettingsDialogSplitterPos;
        QString mFontName;
        int mFontSize;
        bool mShowContentsPanel;
        int mMainWindowWidth;
        int mMainWindowHeight;
        int mMainWindowLeft;
        int mMainWindowTop;
        int mContentsPanelWidth;
    };

    class View: public _Base {
    public:
        explicit View(Settings *settings);

        const QString &fitMode() const;
        void setFitMode(const QString &newFitMode);

        int slideShowDelayTime() const;
        void setSlideShowDelayTime(int newSlideShowDelayTime);

    protected:
        void doSave() override;
        void doLoad() override;
    private:
        QString mFitMode;
        int mSlideShowDelayTime;
    };

    class History: public _Base {
    public:
        explicit History(Settings *settings);

        QStringList recentDirs() const;
        void setRecentDirs(const QStringList &newRecentDirs);
        void addRecentDir(const QString &dir);
        void clearRecentDirs();

    protected:
        void doSave() override;
        void doLoad() override;
    private:
        QStringList mRecentDirs;
    };

public:
    explicit Settings(const QString& filename);
    explicit Settings(Settings&& settings) = delete;
    explicit Settings(const Settings& settings) = delete;
    ~Settings();

    Settings& operator= (const Settings& settings) = delete;
    Settings& operator= (const Settings&& settings) = delete;
    void beginGroup(const QString& group);
    void endGroup();
    void remove(const QString &key);
    void saveValue(const QString& group, const QString &key, const QVariant &value);
    void saveValue(const QString &key, const QVariant &value);
    QVariant value(const QString& group, const QString &key, const QVariant& defaultValue);
    QVariant value(const QString &key, const QVariant& defaultValue);
    void load();
    void save();
    QSettings::Status sync();

    Dirs& dirs();
    UI& ui();
    View& view();
    History& history();
    QString filename() const;

private:
    QString mFilename;
    QSettings mSettings;
    Dirs mDirs;
    class UI mUI;
    View mView;
    History mHistory;
};

extern Settings* pSettings;

#endif // SETTINGS_H
