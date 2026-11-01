#include <QTest>
#include <QFont>
#include <QFontInfo>
#include <QFontMetrics>

#include "test_qsynedit_infra.h"
#include "test_utils.h"

namespace QSynedit {

void TestQSyneditInfra::test_fonts()
{
    // the bundled mono font must really be installed, not silently
    // substituted by an environment font
    QFont monoFont(defaultMonoFont());
    monoFont.setPointSize(14);
    QCOMPARE(QFontInfo(monoFont).family(), defaultMonoFont());

    QFontMetrics monoMetrics(monoFont);
    // it is really monospaced for ASCII
    int halfWidth = monoMetrics.horizontalAdvance('A');
    QCOMPARE(monoMetrics.horizontalAdvance('i'), halfWidth);
    QCOMPARE(monoMetrics.horizontalAdvance('m'), halfWidth);
    // CJK and emoji are full-width (allowing +-1px of engine rounding)
    int cjkWidth = monoMetrics.horizontalAdvance(QString("中"));
    QVERIFY(qAbs(cjkWidth - 2*halfWidth) <= 1);
    int emojiWidth = monoMetrics.horizontalAdvance(QString::fromUcs4(U"\U0001F334"));
    QVERIFY(qAbs(emojiWidth - 2*halfWidth) <= 1);
    // ZWJ is zero-width
    QCOMPARE(monoMetrics.horizontalAdvance(QChar(0x200D)), 0);

    // the proportional font is installed...
    QFont propFont(testProportionalFont());
    propFont.setPointSize(14);
    QCOMPARE(QFontInfo(propFont).family(), testProportionalFont());
    // ...and really proportional
    QFontMetrics propMetrics(propFont);
    QVERIFY(propMetrics.horizontalAdvance('i') != propMetrics.horizontalAdvance('m'));
}

}
