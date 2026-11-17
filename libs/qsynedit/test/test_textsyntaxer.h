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
#ifndef TEST_TEXTSYNTAXER_H
#define TEST_TEXTSYNTAXER_H

#include <QObject>

class TestTextSyntaxer : public QObject
{
    Q_OBJECT
private slots:
    void test_tokens_text_positions_and_attributes();
    void test_empty_lines_and_state();
    void test_create_instance_copies_settings();
    void test_profile_file();
};

#endif // TEST_TEXTSYNTAXER_H
