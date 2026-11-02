/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
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

// Minimal multiview demo: two QSynEdit views share one Document.
// Editing in either view updates the other; undo/redo history is shared
// (Document-owned); caret/selection/scroll/font are per-view.

#include <QApplication>
#include <QFontDatabase>
#include <QMainWindow>
#include <QSplitter>
#include "qsynedit/qsynedit.h"
#include "qsynedit/syntaxer/cpp.h"
#include "qt_utils/charsetinfo.h"

using namespace QSynedit;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle(QStringLiteral("QSynedit multiview demo (two views, one document)"));

    auto *splitter = new QSplitter(&window);

    auto *edit1 = new QSynEdit(splitter);
    edit1->setSyntaxer(std::make_shared<CppSyntaxer>());

    auto *edit2 = new QSynEdit(splitter);
    // attach the second view to the same shared document
    edit2->setDocument(edit1->document());

    // font metrics are per-view: the two views may use different fonts/sizes
    QFont font1 = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font1.setPointSize(10);
    edit1->setFont(font1);
    QFont font2 = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font2.setPointSize(14);
    edit2->setFont(font2);

    splitter->addWidget(edit1);
    splitter->addWidget(edit2);
    window.setCentralWidget(splitter);
    window.resize(1100, 650);

    if (argc > 1) {
        QByteArray realEncoding;
        edit1->loadFromFile(QString::fromLocal8Bit(argv[1]), ENCODING_AUTO_DETECT, realEncoding);
    } else {
        edit1->document()->setText(QStringLiteral(
            "// Two QSynEdit views sharing one Document.\n"
            "// - type in either view: both update\n"
            "// - undo/redo (Ctrl+Z / Ctrl+Y) is shared\n"
            "// - caret, selection, scrolling and font are per-view\n"
            "\n"
            "#include <iostream>\n"
            "\n"
            "int main()\n"
            "{\n"
            "    std::cout << \"hello\" << std::endl;\n"
            "    return 0;\n"
            "}\n"));
    }

    window.show();
    return app.exec();
}
