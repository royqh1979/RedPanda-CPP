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
#ifndef TEST_TODOPARSER_H
#define TEST_TODOPARSER_H

#include <QObject>

class TodoThread;

class TestTodoParser : public QObject
{
    Q_OBJECT
private:
    static QStringList parseTodoFile(TodoThread &thread, const QString &fileName);
private slots:
    void test_inline_todo_preserves_parenthesis_state();
    void test_parse_miniaudio_preserves_parenthesis_state();
};

#endif // TEST_TODOPARSER_H
