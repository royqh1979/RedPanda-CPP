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
#ifndef TEST_WIDGETS_H
#define TEST_WIDGETS_H

#include <QObject>
#include <memory>
#include "src/parser/cppparser.h"

class ColorManager;
class Settings;

class TestWidgets : public QObject
{
    Q_OBJECT
public:
    explicit TestWidgets(QObject *parent = nullptr);
    ~TestWidgets() override;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void test_code_completion_inherited_access();
    void test_code_completion_complex_keyword();
    void test_code_completion_labels();
    void test_code_completion_function_without_definition();
    void test_code_completion_namespaces();
    void test_code_completion_inline_namespaces();
#ifdef REDPANDA_PARSER_STDLIB_TEST
    void test_code_completion_standard_library_string();
    void test_code_completion_standard_library_vector_subscript();
    void test_code_completion_standard_library_make_shared();
    void test_goto_standard_library_string();
    void test_class_browser_standard_library_header_context();
    void test_class_browser_standard_library_ostream_context();
    void test_standard_library_template_consumers();
#endif
    void test_code_completion_types();
    void test_code_completion_macros();
    void test_code_completion_normal_includes_macros();
    void test_code_completion_keywords_only();
    void test_code_completion_operator_overloads_are_excluded();
    void test_class_browser_reopened_namespace_single_node();
    void test_class_browser_project_union();
    void test_class_browser_current_file_limits_reopened_namespace();
    void test_class_browser_current_file_out_of_line_member();
    void test_class_browser_inline_namespace_aliases();
    void test_class_browser_direct_members();
    void test_unscoped_enum_completion_and_class_browser();
    void test_goto_class_declaration_and_definition();
    void test_goto_constructor_declaration_and_definition();
    void test_goto_statement_definitions();
    void test_goto_definition_prefers_current_file();
    void test_ctrl_hover_uses_pointing_hand_cursor();
    void test_function_tip_class();
    void test_rename_class_and_constructor();
    void test_rename_from_constructor();
    void test_editor_color_scheme_demo_parser_is_self_contained();

private:
    std::unique_ptr<Settings> mSettings;
    std::unique_ptr<ColorManager> mColorManager;
    PCppParser mParser;
    QString mMainFile;
    QStringList mMainContent;
};

#endif // TEST_WIDGETS_H
