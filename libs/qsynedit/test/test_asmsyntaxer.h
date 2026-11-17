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
#ifndef TEST_ASMSYNTAXER_H
#define TEST_ASMSYNTAXER_H

#include <QObject>

class TestASMSyntaxer : public QObject
{
    Q_OBJECT
private slots:
    void test_gas_identifiers_preserve_original_text();
    void test_nasm_directives_preprocessor_and_numbers();
    void test_comments_strings_and_symbols();
    void test_trailing_spaces();
    void test_gas_directive_mode_and_prefix();
    void test_gas_state_restoration_and_clone();
    void test_profile_file();
};

#endif // TEST_ASMSYNTAXER_H
