#include <QTest>
#include "test_editor_display.h"
#include "src/editor.h"

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
