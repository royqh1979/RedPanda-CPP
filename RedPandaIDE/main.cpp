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
#include "mainwindow.h"
#include "settings.h"
#include "systemconsts.h"
#include "utils.h"
#include <QApplication>
#include <QDir>
#include <QTranslator>
#include <QStandardPaths>
#include <QMessageBox>
#include <QStringList>
#include <QAbstractNativeEventFilter>
#include <QDesktopWidget>
#include <QDir>
#include <QScreen>
#include "common.h"
#include "colorscheme.h"
#include "iconsmanager.h"
#include "autolinkmanager.h"
#include "platform.h"
#include "parser/parserutils.h"
#include "editorlist.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_WIN
class WindowLogoutEventFilter : public QAbstractNativeEventFilter {

    // QAbstractNativeEventFilter interface
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
};

bool WindowLogoutEventFilter::nativeEventFilter(const QByteArray & /*eventType*/, void *message, long *result){
    MSG * pMsg = static_cast<MSG *>(message);
    switch(pMsg->message) {
    case WM_QUERYENDSESSION:
        if (pMsg->lParam == 0
                || (pMsg->lParam & ENDSESSION_LOGOFF)) {
            if (!pMainWindow->close()) {
                *result = 0;
            } else {
                *result = 1;
            }
            return true;
        }
        break;
    case WM_DPICHANGED:
        setScreenDPI(HIWORD(pMsg->wParam));
        pMainWindow->updateDPI();
        break;
    }
    return false;
}
#endif

QString getSettingFilename(const QString& filepath = QString()) {
    QString filename;
    if (filepath.isEmpty()) {
        if (isGreenEdition()) {
            filename = includeTrailingPathDelimiter(QApplication::applicationDirPath()) +
                    "config/"  + APP_SETTSINGS_FILENAME;
        } else {
            filename =includeTrailingPathDelimiter(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0])
                 + APP_SETTSINGS_FILENAME;
        }
    } else {
        filename = filepath;
    }

    QFileInfo fileInfo(filename);
    QDir dir(fileInfo.absoluteDir());
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            QMessageBox::critical(nullptr, QObject::tr("Error"),
                QString(QObject::tr("Can't create configuration folder %1")).arg(dir.absolutePath()));
            return "";
        }
    }

    if (fileInfo.exists() && !fileInfo.isWritable()) {
        QMessageBox::critical(nullptr, QObject::tr("Error"),
            QString(QObject::tr("Can't write to configuration file %1")).arg(filename));

        return "";
    }
    return filename;
}

int main(int argc, char *argv[])
{
    //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    //Translation must be loaded first
    QTranslator trans,transQt;
    QString settingFilename = getSettingFilename();
    if (!isGreenEdition()) {
        QDir::setCurrent(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0]);
    }
    if (settingFilename.isEmpty())
        return -1;
    {
        QSettings languageSetting(settingFilename,QSettings::IniFormat);
        languageSetting.beginGroup(SETTING_ENVIRONMENT);
        QString language = languageSetting.value("language",QLocale::system().name()).toString();

        if (trans.load("RedPandaIDE_"+language,":/i18n/")) {
            app.installTranslator(&trans);
        }
        if (transQt.load("qt_"+language,":/translations")) {
            app.installTranslator(&transQt);
        }
    }

    qRegisterMetaType<PCompileIssue>("PCompileIssue");
    qRegisterMetaType<PCompileIssue>("PCompileIssue&");
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<QHash<int,QString>>("QHash<int,QString>");

    initParser();

    try {

        SystemConsts systemConsts;
        pSystemConsts = &systemConsts;
        pCharsetInfoManager = new CharsetInfoManager();
        auto charsetInfoManager = std::unique_ptr<CharsetInfoManager>(pCharsetInfoManager);
        //load settings
        pSettings = new Settings(settingFilename);
        auto settings = std::unique_ptr<Settings>(pSettings);

        //Color scheme settings must be loaded after translation
        pColorManager = new ColorManager();
        pIconsManager = new IconsManager();
        pAutolinkManager = new AutolinkManager();
        try {
            pAutolinkManager->load();
        } catch (FileError e) {
            QMessageBox::critical(nullptr,
                                  QObject::tr("Can't load autolink settings"),
                                  e.reason(),
                                  QMessageBox::Ok);
        }

        //set default open folder
        QDir::setCurrent(pSettings->environment().defaultOpenFolder());

        MainWindow mainWindow;
        pMainWindow = &mainWindow;
        if (app.arguments().count()>1) {
            QStringList filesToOpen = app.arguments();
            filesToOpen.pop_front();
            pMainWindow->openFiles(filesToOpen);
        } else {
            if (pSettings->editor().autoLoadLastFiles())
                pMainWindow->loadLastOpens();
            if (pMainWindow->editorList()->pageCount()==0) {
                pMainWindow->newEditor();
            }
        }
#if QT_VERSION_MAJOR==5 && QT_VERSION_MINOR < 15
        setScreenDPI(qApp->primaryScreen()->logicalDotsPerInch());
#else
        if (mainWindow.screen())
            setScreenDPI(mainWindow.screen()->logicalDotsPerInch());
#endif
        mainWindow.show();
#ifdef Q_OS_WIN
        WindowLogoutEventFilter filter;
        app.installNativeEventFilter(&filter);
#endif
        int retCode = app.exec();
        QString configDir = pSettings->dirs().config();
        // save settings
        // settings->compilerSets().saveSets();
        if (mainWindow.shouldRemoveAllSettings()) {
            settings.release();
            delete pSettings;
            QDir dir(configDir);
            dir.removeRecursively();
        }
        return retCode;
    }  catch (BaseError e) {
        QMessageBox::critical(nullptr,QApplication::tr("Error"),e.reason());
        return -1;
    }
}
