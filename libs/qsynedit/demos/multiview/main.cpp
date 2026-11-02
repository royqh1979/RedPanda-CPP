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
//
// File menu: New / Open / Save / Save As / Close operate on the shared
// document, so both views always show the same file.

#include <QApplication>
#include <QEvent>
#include <QFileDialog>
#include <QFontDatabase>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QWheelEvent>
#include "qsynedit/qsynedit.h"
#include "qsynedit/syntaxer/cpp.h"
#include "qt_utils/charsetinfo.h"

using namespace QSynedit;

// Ctrl+wheel font zoom, one instance per view (font size is a per-view
// property). QSynEdit itself only handles plain/Shift wheel scrolling.
class WheelZoomFilter : public QObject {
public:
    using QObject::QObject;
protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::Wheel) {
            auto *wheelEvent = static_cast<QWheelEvent *>(event);
            if (wheelEvent->modifiers() & Qt::ControlModifier) {
                auto *edit = static_cast<QSynEdit *>(watched);
                if ((mAccumulatedDelta > 0 && wheelEvent->angleDelta().y() < 0)
                        || (mAccumulatedDelta < 0 && wheelEvent->angleDelta().y() > 0))
                    mAccumulatedDelta = 0;
                mAccumulatedDelta += wheelEvent->angleDelta().y();
                while (mAccumulatedDelta >= 120) {
                    mAccumulatedDelta -= 120;
                    edit->processCommand(EditCommand::ZoomIn);
                }
                while (mAccumulatedDelta <= -120) {
                    mAccumulatedDelta += 120;
                    edit->processCommand(EditCommand::ZoomOut);
                }
                return true;
            }
        }
        return false;
    }
private:
    int mAccumulatedDelta = 0;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;

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

    // the library default active-line color (Qt::blue) is unreadably dark
    const QColor activeLineColor = window.palette().color(QPalette::AlternateBase);
    edit1->setActiveLineColor(activeLineColor);
    edit2->setActiveLineColor(activeLineColor);

    // gutter with line numbers is on by default; enable code folding so the
    // fold buttons show up in the gutter (per-view setting)
    edit1->setUseCodeFolding(true);
    edit2->setUseCodeFolding(true);

    // Ctrl+wheel zooms the view under the mouse
    edit1->installEventFilter(new WheelZoomFilter(edit1));
    edit2->installEventFilter(new WheelZoomFilter(edit2));

    splitter->addWidget(edit1);
    splitter->addWidget(edit2);
    window.setCentralWidget(splitter);
    window.resize(1100, 650);

    // ---- file state (the shared document's file) ----
    QString currentFile;
    QByteArray encoding = ENCODING_UTF8;

    auto updateTitle = [&] {
        if (currentFile.isEmpty())
            window.setWindowTitle(QStringLiteral("untitled - QSynedit multiview demo"));
        else
            window.setWindowTitle(QStringLiteral("%1 - QSynedit multiview demo").arg(currentFile));
    };

    // the undo/redo stacks are owned by the shared document: clearing them
    // once clears the history for both views
    auto clearHistory = [&] {
        edit1->document()->undoList()->clear();
        edit1->document()->redoList()->clear();
    };

    auto newFile = [&] {
        edit1->document()->setText(QString());
        clearHistory();
        currentFile.clear();
        encoding = ENCODING_UTF8;
        updateTitle();
    };

    auto saveTo = [&](const QString &path) -> bool {
        QFile file(path);
        QByteArray realEncoding = encoding;
        try {
            edit1->saveToFile(file, encoding, realEncoding);
        } catch (const FileError &e) {
            QMessageBox::warning(&window, QStringLiteral("Save failed"), e.reason());
            return false;
        }
        currentFile = path;
        encoding = realEncoding;
        updateTitle();
        return true;
    };

    auto saveFile = [&] {
        if (currentFile.isEmpty()) {
            QString path = QFileDialog::getSaveFileName(&window, QStringLiteral("Save file"));
            if (path.isEmpty())
                return;
            saveTo(path);
        } else {
            saveTo(currentFile);
        }
    };

    QMenu *fileMenu = window.menuBar()->addMenu(QStringLiteral("&File"));
    QAction *action = fileMenu->addAction(QStringLiteral("&New"), &window, newFile);
    action->setShortcut(QKeySequence::New);
    action = fileMenu->addAction(QStringLiteral("&Open..."), &window, [&] {
        QString path = QFileDialog::getOpenFileName(&window, QStringLiteral("Open file"));
        if (path.isEmpty())
            return;
        QByteArray realEncoding;
        try {
            edit1->loadFromFile(path, ENCODING_AUTO_DETECT, realEncoding);
        } catch (const FileError &e) {
            QMessageBox::warning(&window, QStringLiteral("Open failed"), e.reason());
            return;
        }
        clearHistory();
        currentFile = path;
        encoding = realEncoding;
        updateTitle();
    });
    action->setShortcut(QKeySequence::Open);
    action = fileMenu->addAction(QStringLiteral("&Save"), &window, saveFile);
    action->setShortcut(QKeySequence::Save);
    action = fileMenu->addAction(QStringLiteral("Save &As..."), &window, [&] {
        QString path = QFileDialog::getSaveFileName(&window, QStringLiteral("Save file as"));
        if (!path.isEmpty())
            saveTo(path);
    });
    action->setShortcut(QKeySequence::SaveAs);
    action = fileMenu->addAction(QStringLiteral("&Close"), &window, newFile);
    action->setShortcut(QKeySequence::Close);

    if (argc > 1) {
        QByteArray realEncoding;
        try {
            edit1->loadFromFile(QString::fromLocal8Bit(argv[1]), ENCODING_AUTO_DETECT, realEncoding);
            clearHistory();
            currentFile = QString::fromLocal8Bit(argv[1]);
            encoding = realEncoding;
        } catch (const FileError &e) {
            QMessageBox::warning(&window, QStringLiteral("Open failed"), e.reason());
        }
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
    updateTitle();

    window.show();
    return app.exec();
}
