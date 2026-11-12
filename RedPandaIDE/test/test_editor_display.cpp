#include <QTest>
#include "test_editor_display.h"
#include "src/editor.h"
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
    mEditor->setGetReformatterFunc([&newContent](Editor *) {
        return std::make_unique<StaticReformatter>(newContent);
    });
    mEditor->reformat(true);

    QCOMPARE(mEditor->caretPos(), savedCaret);
    QCOMPARE(mEditor->topLine(), 50);
    QCOMPARE(mEditor->topSegment(), 0);
}
