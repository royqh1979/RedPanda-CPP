#include <QtTest>
#include <QCoreApplication>
#include <QWindow>;

namespace QSynedit{

class QSynEdit;

class TestQSynedit : public QObject
{
    Q_OBJECT
private:
    std::shared_ptr<QSynEdit> mEdit;
private slots:
    void init();
    void test_test1();
};

}
