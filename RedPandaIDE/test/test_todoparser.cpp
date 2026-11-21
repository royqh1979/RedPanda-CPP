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
#include "test_todoparser.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include "src/todoparser.h"

namespace {
QStringList debugMessages;

void collectDebugMessage(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    if (type == QtDebugMsg)
        debugMessages.append(message);
}
}

QStringList TestTodoParser::parseTodoFile(TodoThread &thread, const QString &fileName)
{
    debugMessages.clear();
    QtMessageHandler oldHandler = qInstallMessageHandler(collectDebugMessage);
    thread.doParseFile(fileName);
    qInstallMessageHandler(oldHandler);
    return debugMessages;
}

void TestTodoParser::test_inline_todo_preserves_parenthesis_state()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString fileName = tmp.filePath("todo.cpp");
    QFile file(fileName);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("int f()\n"
               "{\n"
               "    /* TODO: Check the result. */\n"
               "    int value = foo(1);\n"
               "    return bar(value);\n"
               "}\n");
    file.close();

    TodoThread thread(fileName);
    QSignalSpy todoSpy(&thread, &TodoThread::todoFound);
    QStringList messages = parseTodoFile(thread, fileName);
    QCOMPARE(todoSpy.count(), 1);
    QVERIFY2(!messages.join('\n').contains("parenthesis level < 0"),
             qPrintable(messages.join('\n')));
}

void TestTodoParser::test_parse_miniaudio_preserves_parenthesis_state()
{
    QString fileName = QFINDTESTDATA("../../libs/qsynedit/test/resources/miniaudio.h");
    QVERIFY(!fileName.isEmpty());
    TodoThread thread(fileName);
    QSignalSpy todoSpy(&thread, &TodoThread::todoFound);
    QStringList messages = parseTodoFile(thread, fileName);

    QVERIFY(todoSpy.count() > 0);
    QVERIFY2(!messages.join('\n').contains("parenthesis level < 0"),
             qPrintable(messages.join('\n')));
}
