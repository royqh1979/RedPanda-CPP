#include "auto_test.h"

// 类测试
class MathTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void testAddition() {
        QCOMPARE(1 + 1, 2);
    }

    void testMultiplication() {
        QCOMPARE(2 * 3, 6);
    }

    void testMultiplication2() {
        QCOMPARE(2 * 3, 6);
    }
};
DECLARE_TEST(MathTest)

// 分组测试
TEST_GROUP_BEGIN(NetworkTests)
void testPing() {
    QVERIFY(true);
}
void testDownload() {
    QVERIFY(true);
}
TEST_GROUP_END(NetworkTests)

// 主函数
TEST_MAIN

#include "test_main.moc"
