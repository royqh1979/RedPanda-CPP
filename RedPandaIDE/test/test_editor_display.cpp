#include <QTest>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QCursor>
#include <QMimeData>
#include "test_editor_display.h"
#include "src/editor/editor.h"
#include "src/reformatter/basereformatter.h"

namespace {

class StaticReformatter : public BaseReformatter
{
public:
    StaticReformatter(const QString &content): mContent(content) { }

    QString refomat(const QString &, QString &, bool &isOk) override
    {
        isOk = true;
        return mContent;
    }

private:
    QString mContent;
};

}

void TestEditorDisplay::test_fold_marks_paint()
{
    mEditor->loadContent(QStringList{
        "int main() {",
        "    int a = 1;",
        "    if (a) {",
        "        a++;",
        "    }",
        "}",
    });
    mEditor->resize(600, 400);
    QImage image = mEditor->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    int h = mEditor->textHeight();
    // the fold button band (right edge of the gutter) at the row of line 0
    // ("int main() {") must contain the fold square/line pixels
    int x0 = mEditor->gutterWidth() - mEditor->gutter().rightOffset();
    QSet<QRgb> colors;
    for (int x = x0; x < x0 + mEditor->gutter().rightOffset() - 2; x++)
        for (int y = 2; y < h - 2; y++)
            colors.insert(image.pixel(x, y));
    QVERIFY(colors.count() >= 2);
}

void TestEditorDisplay::test_reformat_restores_view_state()
{
    QStringList oldLines;
    for (int i = 0; i < 200; i++)
        oldLines << QString("int old_line_%1 = %1;").arg(i);
    mEditor->loadContent(oldLines);
    mEditor->resize(600, 400);
    QSynedit::CharPos savedCaret{5, 55};
    mEditor->setCaretPos(savedCaret);
    mEditor->setTopLineSegment(50, 0);

    QStringList newLines;
    for (int i = 0; i < 160; i++)
        newLines << QString("int reformatted_line_%1 = %1;").arg(i);
    QString newContent = newLines.join('\n');
    mEditor->setGetReformatterFunc([newContent](Editor *) {
        return std::make_unique<StaticReformatter>(newContent);
    });
    mEditor->reformat(true);

    QCOMPARE(mEditor->caretPos(), savedCaret);
    QCOMPARE(mEditor->topLine(), 50);
    QCOMPARE(mEditor->topSegment(), 0);
}

void TestEditorDisplay::test_dnd_undo_restores_caret()
{
    QString sourcePath = QFINDTESTDATA("../../libs/qsynedit/test/resources/editormanager.cpp");
    QVERIFY(!sourcePath.isEmpty());
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::ReadOnly));
    mEditor->loadContent(QString());
    mEditor->setSelText(QString::fromUtf8(sourceFile.readAll()));
    QStringList originalContent = mEditor->content();
    QSynedit::EditorOptions options = mEditor->getOptions();
    options.setFlag(QSynedit::EditorOption::DragDropEditing, true);
    mEditor->setOptions(options);
    mEditor->resize(600, 400);

    int beginLine = 1145;
    int endLine = 1147;
    QSynedit::CharPos begin{mEditor->lineText(beginLine).indexOf("disconnect"), beginLine};
    QSynedit::CharPos end{mEditor->lineText(endLine).indexOf('}') + 1, endLine};
    auto dragAndUndo = [&](int targetLine, int targetChar, int topLine,
                           QSynedit::CharPos caret, QSynedit::CharPos selectionAnchor) {
        mEditor->setCaretAndSelection(caret, selectionAnchor, caret);
        QVERIFY(mEditor->selText().startsWith("disconnect"));
        QCOMPARE(mEditor->caretPos(), caret);
        QSynedit::CharPos caretBeforeDrag = mEditor->caretPos();
        mEditor->setTopLineSegment(1133, 0);
        QSynedit::CharPos dragClick{begin.ch + 4, begin.line};
        QPoint clickPoint = mEditor->displayCoordToPixels(
                    mEditor->bufferToDisplayPos(dragClick)) + QPoint(2, 2);
        QMouseEvent pressEvent(QEvent::MouseButtonPress, clickPoint, Qt::LeftButton,
                               Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(mEditor->viewport(), &pressEvent);
        QMimeData mime;
        mime.setText(mEditor->selText());
        QDragEnterEvent enterEvent(QPoint(100, 100), Qt::MoveAction, &mime,
                                   Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(mEditor->viewport(), &enterEvent);
        QVERIFY(enterEvent.isAccepted());
        mEditor->setTopLineSegment(topLine, 0);
        QPoint pt = mEditor->displayCoordToPixels(
                    mEditor->bufferToDisplayPos(QSynedit::CharPos{targetChar, targetLine}))
                + QPoint(0, 2);
        QCursor::setPos(mEditor->viewport()->mapToGlobal(pt));
        QDragMoveEvent moveEvent(pt, Qt::MoveAction, &mime,
                                 Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(mEditor->viewport(), &moveEvent);
        QDropEvent dropEvent(pt, Qt::MoveAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(mEditor->viewport(), &dropEvent);
        QVERIFY(dropEvent.isAccepted());
        QVERIFY(mEditor->content() != originalContent);
        mEditor->undo();
        QCoreApplication::processEvents();

        QCOMPARE(mEditor->content(), originalContent);
        QCOMPARE(mEditor->selBegin(), std::min(caret, selectionAnchor));
        QCOMPARE(mEditor->selEnd(), std::max(caret, selectionAnchor));
        QCOMPARE(mEditor->caretPos(), caretBeforeDrag);
        if (caret == begin) {
            QVERIFY(mEditor->lineText(beginLine).left(mEditor->caretCh()).trimmed().isEmpty());
            QVERIFY(mEditor->lineText(beginLine).mid(mEditor->caretCh()).startsWith("disconnect"));
        }
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, clickPoint, Qt::LeftButton,
                                 Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(mEditor->viewport(), &releaseEvent);
    };

    int targetLine = 1139;
    int targetChar = mEditor->lineText(targetLine).indexOf("pMainWindow")
            + QString("pMainWindow").length();
    dragAndUndo(targetLine, targetChar, 1130, begin, end);
    dragAndUndo(1122, mEditor->lineText(1122).indexOf("for"), 1116, begin, end);

    QSynedit::CharPos semicolonEnd{mEditor->lineText(1146).indexOf(';') + 1, 1146};
    targetLine = 1137;
    targetChar = mEditor->lineText(targetLine).indexOf("connect");
    dragAndUndo(targetLine, targetChar, 1133, semicolonEnd, begin);
}
