#ifndef TEST_QSYNEDIT_INFRA_H
#define TEST_QSYNEDIT_INFRA_H
#include <QObject>

namespace QSynedit {

// Tests for the test infrastructure itself (bundled fonts, helpers...).
class TestQSyneditInfra : public QObject
{
    Q_OBJECT

private slots:
    void test_fonts();
};
}
#endif // TEST_QSYNEDIT_INFRA_H
