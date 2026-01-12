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
#ifndef UTILS_H
#define UTILS_H
#include <type_traits>
#include <utility>
#include <functional>
#include <QString>
#include <QRect>
#include <QStringList>
#include <memory>
#include <QThread>
#include <QProcessEnvironment>
#include <QTemporaryFile>
#define SI_NO_CONVERSION
#include "SimpleIni.h"
#include "qt_utils/utils.h"
#include "utils/types.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

using SimpleIni = CSimpleIniA;
using PSimpleIni = std::shared_ptr<SimpleIni>;

struct ProcessOutput
{
    QByteArray standardOutput;
    QByteArray standardError;
    QString errorMessage;
};

ProcessOutput runAndGetOutput(const QString& cmd, const QString& workingDir, const QStringList& arguments,
                           const QByteArray& inputContent = QByteArray(),
                           bool separateStderr = false,
                           bool inheritEnvironment = false,
                           const QProcessEnvironment& env = QProcessEnvironment() );

void openFileFolderInExplorer(const QString& path);

void executeFile(const QString& fileName,
                 const QStringList& params,
                 const QString& workingDir,
                 const QString& tempFile);

qulonglong stringToHex(const QString& str, bool &isOk);

QByteArray getHTTPBody(const QByteArray& content);

QString getSizeString(int size);

class QComboBox;
void setComboTextAndHistory(QComboBox *cb, const QString& newText, QStringList &historyList);
void updateComboHistory(QStringList &historyList, const QString &newKey);

QStringList getExecutableSearchPaths();

QStringList platformCommandForTerminalArgsPreview();

QString byteArrayToString(const QByteArray &content, bool isUTF8);
QByteArray stringToByteArray(const QString& content, bool isUTF8);

#ifdef _MSC_VER
#define __builtin_unreachable() (__assume(0))
#endif

#endif // UTILS_H
