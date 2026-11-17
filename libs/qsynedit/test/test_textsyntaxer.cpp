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
#include "test_textsyntaxer.h"
#include <QTest>
#include "test_utils.h"
#include "qsynedit/syntaxer/textfile.h"

void TestTextSyntaxer::test_tokens_text_positions_and_attributes()
{
    QSynedit::TextSyntaxer syntaxer;
    TokenInfoList tokens = parseLine(&syntaxer, "alpha beta,gamma\tδelta  ");
    QStringList expected{"alpha", " ", "beta", ",", "gamma", "\t", "δelta", "  "};
    QCOMPARE(tokens.count(), expected.count());
    int position = 0;
    for (int i = 0; i < tokens.count(); ++i) {
        QCOMPARE(tokens[i]->token, expected[i]);
        QCOMPARE(tokens[i]->startChar, position);
        position += expected[i].length();
        QCOMPARE(tokens[i]->attribute,
                 i == 1 || i == 5 || i == 7
                 ? syntaxer.whitespaceAttribute() : syntaxer.getAttribute("Text"));
    }
    QVERIFY(tokens.last()->state->hasTrailingSpaces);
}

void TestTextSyntaxer::test_empty_lines_and_state()
{
    QSynedit::TextSyntaxer syntaxer;
    QList<TokenInfoList> lines = parseLines(&syntaxer, {"", "text", "   "});
    QVERIFY(lines[0].isEmpty());
    QCOMPARE(lines[1].count(), 1);
    QCOMPARE(lines[1].first()->token, "text");
    QCOMPARE(lines[2].count(), 1);
    QCOMPARE(lines[2].first()->token, "   ");
    QVERIFY(lines[2].first()->state->hasTrailingSpaces);
    QVERIFY(!syntaxer.needsLineState());
    QVERIFY(!syntaxer.isCommentNotFinished(lines[1].first()->state));
    QVERIFY(!syntaxer.isStringNotFinished(lines[1].first()->state));
}

void TestTextSyntaxer::test_create_instance_copies_settings()
{
    QSynedit::TextSyntaxer syntaxer;
    syntaxer.getAttribute("Text")->setForeground(QColor(1, 2, 3));
    auto clone = std::dynamic_pointer_cast<QSynedit::TextSyntaxer>(syntaxer.createInstance());
    QVERIFY(clone);
    QCOMPARE(clone->getAttribute("Text")->foreground(), QColor(1, 2, 3));
    QVERIFY(clone->getAttribute("Text") != syntaxer.getAttribute("Text"));
    QVERIFY(!clone->needsLineState());
    QVERIFY(!clone->supportFolding());
}

void TestTextSyntaxer::test_profile_file()
{
    QString fileName = qEnvironmentVariable("QSYNEDIT_PROFILE_FILE");
    if (fileName.isEmpty())
        QSKIP("Set QSYNEDIT_PROFILE_FILE to run syntaxer profiling");
    QSynedit::TextSyntaxer syntaxer;
    profileSyntaxerFile(&syntaxer, fileName, "text");
}
