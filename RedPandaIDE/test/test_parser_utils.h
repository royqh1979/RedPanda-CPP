/*
 * Copyright (C) 2026 Roy Qu (royqh1979@gmail.com)
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
#ifndef TEST_PARSER_UTILS_H
#define TEST_PARSER_UTILS_H

#if defined(REDPANDA_PARSER_STDLIB_TEST) || defined(REDPANDA_PARSER_PUBLIC_API_BASELINE_TEST)
#include <QDir>
#include <QProcess>
#include <QTemporaryFile>
#include "src/parser/cppparser.h"

inline bool configureStandardLibraryParser(const PCppParser &parser, QString &bitsStdcpp,
                                           ParserLanguage language = ParserLanguage::CPlusPlus)
{
    bool isC = language == ParserLanguage::C;
    parser->setLanguage(language);
    QString root = QStringLiteral(REDPANDA_PARSER_STDLIB_MINGW_ROOT);
    QString compiler = root + (isC ? "/bin/gcc.exe" : "/bin/g++.exe");
    QTemporaryFile source(QDir::temp().filePath(
                              isC ? "redpanda-stdlib-XXXXXX.c" : "redpanda-stdlib-XXXXXX.cpp"));
    if (!source.open()) {
        qWarning() << "Cannot create a temporary source for compiler defines";
        return false;
    }
    source.write("\n");
    source.close();

    QString standard = qEnvironmentVariable("REDPANDA_PARSER_TEST_CXX_STANDARD");
    if (standard.isEmpty())
        standard = isC ? "c11" : "c++17";

    QProcess process;
    process.start(compiler, {"-std=" + standard, "-dM", "-E", source.fileName()});
    if (!process.waitForStarted() || !process.waitForFinished(30000)
            || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qWarning() << "Cannot obtain compiler defines:" << process.errorString()
                   << process.readAllStandardError();
        return false;
    }
    const QStringList defines = QString::fromUtf8(process.readAllStandardOutput())
            .split('\n', Qt::SkipEmptyParts);
    for (const QString &define : defines)
        parser->addHardDefineByLine(define);

    process.start(compiler, {isC ? "-xc" : "-xc++", "-E", "-v", source.fileName()});
    if (!process.waitForStarted() || !process.waitForFinished(30000)
            || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qWarning() << "Cannot obtain compiler include paths:" << process.errorString()
                   << process.readAllStandardError();
        return false;
    }
    QByteArray output = process.readAllStandardError();
    int searchStart = output.indexOf("#include <...> search starts here:");
    int searchEnd = output.indexOf("End of search list.");
    if (searchStart < 0 || searchEnd < 0 || searchEnd <= searchStart) {
        qWarning() << "Cannot parse compiler include paths";
        return false;
    }
    searchStart += QByteArray("#include <...> search starts here:").length();
    QStringList includePaths;
    const QList<QByteArray> lines = output.mid(searchStart, searchEnd - searchStart).split('\n');
    for (const QByteArray &line : lines) {
        QByteArray path = line.trimmed();
        if (!path.isEmpty() && QDir(path).exists()) {
            QString includePath = QDir(path).canonicalPath();
            includePaths.append(includePath);
            parser->addIncludePath(includePath);
        }
    }
    if (isC)
        return true;
    for (const QString &includePath : includePaths) {
        QString candidate = includePath + "/bits/stdc++.h";
        if (QFileInfo::exists(candidate)) {
            bitsStdcpp = QFileInfo(candidate).canonicalFilePath();
            break;
        }
        candidate = includePath + "/x86_64-w64-mingw32/bits/stdc++.h";
        if (QFileInfo::exists(candidate)) {
            bitsStdcpp = QFileInfo(candidate).canonicalFilePath();
            break;
        }
    }
    if (bitsStdcpp.isEmpty()) {
        QString fallbackDir = QStringLiteral(REDPANDA_PARSER_STDLIB_RESOURCE_DIR);
        QString candidate = fallbackDir + "/bits/stdc++.h";
        if (QFileInfo::exists(candidate)) {
            parser->addIncludePath(fallbackDir);
            bitsStdcpp = QFileInfo(candidate).canonicalFilePath();
        }
    }
    if (bitsStdcpp.isEmpty()) {
        qWarning() << "Cannot find bits/stdc++.h in compiler include paths or test resources";
        return false;
    }
    return true;
}
#endif

#endif // TEST_PARSER_UTILS_H
