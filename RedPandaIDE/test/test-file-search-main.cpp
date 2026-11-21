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
#include "test_filesearchengine.h"
#include "test_searchresultreplacer.h"
#include "test_searchresultmodel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString suite = "all";
    QVector<char *> qtArgs{argv[0]};
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--suite" && i + 1 < argc)
            suite = QString::fromLocal8Bit(argv[++i]);
        else if (arg.startsWith("--suite="))
            suite = arg.mid(QString("--suite=").size());
        else
            qtArgs.push_back(argv[i]);
    }
    argc = qtArgs.size();
    argv = qtArgs.data();

    int status = 0;
    {
        TestFileSearchEngine test;
        if (suite == "all" || suite == "TestFileSearchEngine")
            status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestSearchResultReplacer test;
        if (suite == "all" || suite == "TestSearchResultReplacer")
            status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestSearchResultModel test;
        if (suite == "all" || suite == "TestSearchResultModel")
            status |= QTest::qExec(&test, argc, argv);
    }
    return status;
}
