#ifndef TEST_CHARPOS_H
#define TEST_CHARPOS_H
#include <QObject>

namespace QSynedit {
class TestCharPos : public QObject
{
    Q_OBJECT

private slots:
    void test_default_constructor();
    void test_constructor();
    void test_copy_constructor();
    void test_isInvalid();
    void test_compare();
};
}
#endif
