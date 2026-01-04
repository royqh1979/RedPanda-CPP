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
#ifndef MAIN_H
#define MAIN_H
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
#include <QDir>
#include <QScreen>
#include <QLockFile>
#include <QFontDatabase>
#include <QLibraryInfo>
#include <QComboBox>
#include "common.h"
#include "colorscheme.h"
#include "iconsmanager.h"
#include "autolinkmanager.h"
#include <qt_utils/charsetinfo.h>
#include "parser/parserutils.h"
#include "editormanager.h"
#include "widgets/choosethemedialog.h"
#include "thememanager.h"
#include "utils/font.h"
#include "problems/ojproblemset.h"

#ifdef Q_OS_WIN
#include <QTemporaryFile>
#include <windows.h>
#include <psapi.h>
#include <QSharedMemory>
#include <QBuffer>
#include <winuser.h>
#include <QFontDatabase>
#include "widgets/cpudialog.h"

class WindowLogoutEventFilter : public QAbstractNativeEventFilter {

    // QAbstractNativeEventFilter interface
public:
#if QT_VERSION_MAJOR >= 6
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
#else
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
#endif
};

#ifndef WM_DPICHANGED
# define WM_DPICHANGED 0x02e0
#endif

#define WM_APP_OPEN_FILE (WM_APP + 6736 /* “OPEN” on dial pad */)
static_assert(WM_APP_OPEN_FILE < 0xc000);
#endif


QString getSettingFilename(const QString& filepath, bool& firstRun);

class BlockWheelEventFiler : public QObject {
    Q_OBJECT
public:
    BlockWheelEventFiler(QObject* parent=nullptr);
    ~BlockWheelEventFiler();
    BlockWheelEventFiler(const BlockWheelEventFiler&) = delete;
    BlockWheelEventFiler& operator=(const BlockWheelEventFiler&) = delete;

    // QObject interface
public:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif
