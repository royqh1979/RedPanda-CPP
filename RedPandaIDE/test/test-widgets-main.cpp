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
#include <QApplication>
#include <QTest>
#include "src/mainwindow.h"
#include "test_widgets.h"
#include "test_compilersettings.h"
#include "test_todoparser.h"

MainWindow *pMainWindow = nullptr;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString suite = "all";
    QVector<char *> qtArgs;
    qtArgs.push_back(argv[0]);
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--suite") {
            if (i + 1 >= argc) {
                qWarning("--suite requires a name");
                return 2;
            }
            suite = QString::fromLocal8Bit(argv[++i]);
        } else if (arg.startsWith("--suite=")) {
            suite = arg.mid(QString("--suite=").size());
        } else {
            qtArgs.push_back(argv[i]);
        }
    }
    argc = qtArgs.length();
    argv = qtArgs.data();

    int status = 0;
    TestWidgets test;
    if (suite == "all" || suite == test.metaObject()->className())
        status |= QTest::qExec(&test, argc, argv);
    TestCompilerSettings compilerSettingsTest;
    if (suite == "all" || suite == compilerSettingsTest.metaObject()->className())
        status |= QTest::qExec(&compilerSettingsTest, argc, argv);
    TestTodoParser todoParserTest;
    if (suite == "all" || suite == todoParserTest.metaObject()->className())
        status |= QTest::qExec(&todoParserTest, argc, argv);
    return status;
}
