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
#include "test_asmsyntaxer.h"
#include <QTest>
#include "test_utils.h"
#include "qsynedit/syntaxer/gas.h"
#include "qsynedit/syntaxer/nasm.h"

void TestASMSyntaxer::test_gas_identifiers_preserve_original_text()
{
    QSynedit::GASSyntaxer syntaxer;
    QList<TokenInfoList> lines = parseLines(&syntaxer, {
        "MOV EAX, %EBX",
        "label: mov byte ptr [rax], 0x2A",
        ".SECTION .text"
    });

    QCOMPARE(lines[0][0]->token, "MOV");
    QCOMPARE(lines[0][2]->token, "EAX");
    QCOMPARE(lines[0][5]->token, "%EBX");
    QCOMPARE(filterTokens(lines, syntaxer.keywordAttribute()),
             QStringList({"MOV", "mov"}));
    QCOMPARE(filterTokens(lines, syntaxer.registerAttribute()),
             QStringList({"%EBX"}));
    QCOMPARE(lines[0][2]->attribute, syntaxer.identifierAttribute());
    QCOMPARE(lines[1][10]->attribute, syntaxer.identifierAttribute());
    QCOMPARE(filterTokens(lines, syntaxer.labelAttribute()),
             QStringList({"label"}));
    QCOMPARE(filterTokens(lines, syntaxer.directiveAttribute()),
             QStringList({"byte", "ptr", ".SECTION", ".text"}));
    QCOMPARE(filterTokens(lines, syntaxer.hexAttribute()),
             QStringList({"0x2A"}));
}

void TestASMSyntaxer::test_nasm_directives_preprocessor_and_numbers()
{
    QSynedit::NASMSyntaxer syntaxer;
    QList<TokenInfoList> lines = parseLines(&syntaxer, {
        "%DEFINE VALUE 0x2A",
        "SECTION .data",
        "value: DB 0ffH, 0xB, 0ABH, 12D, 10B, 77Q"
    });

    QCOMPARE(lines[0][0]->token, "%DEFINE");
    QCOMPARE(filterTokens(lines, syntaxer.preprocessDirectiveAttribute()),
             QStringList({"%DEFINE"}));
    QCOMPARE(filterTokens(lines, syntaxer.directiveAttribute()),
             QStringList({"SECTION", "DB"}));
    QCOMPARE(filterTokens(lines, syntaxer.labelAttribute()),
             QStringList({"value"}));
    QCOMPARE(filterTokens(lines, syntaxer.hexAttribute()),
             QStringList({"0x2A", "0ffH", "0xB", "0ABH"}));
    QCOMPARE(filterTokens(lines, syntaxer.numberAttribute()).join("|"),
             "12D");
    QCOMPARE(filterTokens(lines, syntaxer.octAttribute()),
             QStringList({"10B", "77Q"}));
}

void TestASMSyntaxer::test_comments_strings_and_symbols()
{
    QSynedit::GASSyntaxer gas;
    TokenInfoList gasTokens = parseLine(&gas, "MOV \"EAX\" 'x' >= <= # MOV");
    QCOMPARE(filterTokens({gasTokens}, gas.stringAttribute()).join("|"),
             "\"EAX\"|'x'");
    QCOMPARE(filterTokens({gasTokens}, gas.commentAttribute()),
             QStringList({"# MOV"}));
    QCOMPARE(filterTokens({gasTokens}, gas.symbolAttribute()),
             QStringList({">=", "<="}));

    QSynedit::NASMSyntaxer nasm;
    TokenInfoList nasmTokens = parseLine(&nasm, "MOV EAX ; comment");
    QCOMPARE(filterTokens({nasmTokens}, nasm.commentAttribute()),
             QStringList({"; comment"}));
}

void TestASMSyntaxer::test_trailing_spaces()
{
    QSynedit::GASSyntaxer syntaxer;
    TokenInfoList tokens = parseLine(&syntaxer, "mov eax, \t ");
    QCOMPARE(tokens.last()->token, " \t ");
    QVERIFY(tokens.last()->state->hasTrailingSpaces);
}

void TestASMSyntaxer::test_gas_directive_mode_and_prefix()
{
    QSynedit::GASSyntaxer syntaxer;
    QList<TokenInfoList> lines = parseLines(&syntaxer, {
        "mov eax, ebx",
        "mov %eax, %ebx",
        ".intel_syntax noprefix",
        "mov eax, ebx",
        "mov %eax, %ebx",
        ".att_syntax prefix",
        "mov %eax, %ebx"
    });
    auto registerTokens = [&](int line) {
        return filterTokens({lines[line]}, syntaxer.registerAttribute());
    };
    QVERIFY(registerTokens(0).isEmpty());
    QCOMPARE(registerTokens(1), QStringList({"%eax", "%ebx"}));
    QCOMPARE(registerTokens(3), QStringList({"eax", "ebx"}));
    QVERIFY(registerTokens(4).isEmpty());
    QCOMPARE(registerTokens(6), QStringList({"%eax", "%ebx"}));

    auto state = std::dynamic_pointer_cast<QSynedit::GASSyntaxer::GASSyntaxState>(
                syntaxer.getState());
    QVERIFY(state);
    QCOMPARE(state->syntaxMode, QSynedit::GASSyntaxer::SyntaxMode::ATT);
    QVERIFY(state->prefixRegisterNames);
}

void TestASMSyntaxer::test_gas_state_restoration_and_clone()
{
    QSynedit::GASSyntaxer syntaxer;
    parseLines(&syntaxer, {".intel_syntax noprefix"});
    QSynedit::PSyntaxState state = syntaxer.getState();

    QSynedit::GASSyntaxer restored;
    restored.setState(state);
    restored.setLine(1, "mov eax, ebx", 1);
    QStringList registers;
    while (!restored.eol()) {
        if (restored.getTokenAttribute() == restored.registerAttribute())
            registers.append(restored.getToken());
        restored.next();
    }
    QCOMPARE(registers, QStringList({"eax", "ebx"}));

    auto clone = std::dynamic_pointer_cast<QSynedit::GASSyntaxer>(syntaxer.createInstance());
    QVERIFY(clone);
    QCOMPARE(clone->syntaxMode(), QSynedit::GASSyntaxer::SyntaxMode::ATT);
    QVERIFY(clone->prefixRegisterNames());

    syntaxer.resetState();
    QCOMPARE(syntaxer.syntaxMode(), QSynedit::GASSyntaxer::SyntaxMode::ATT);
    QVERIFY(syntaxer.prefixRegisterNames());
}

void TestASMSyntaxer::test_profile_file()
{
    QString fileName = qEnvironmentVariable("QSYNEDIT_PROFILE_FILE");
    if (fileName.isEmpty())
        QSKIP("Set QSYNEDIT_PROFILE_FILE to run syntaxer profiling");
    QSynedit::GASSyntaxer syntaxer;
    profileSyntaxerFile(&syntaxer, fileName, "gas");
}
