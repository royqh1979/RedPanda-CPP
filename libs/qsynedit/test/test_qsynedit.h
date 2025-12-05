#include <QtTest>
#include <QCoreApplication>

namespace QSynedit{

class QSynEdit;

class TestQSyneditCpp : public QObject
{
    Q_OBJECT
private:
    std::shared_ptr<QSynEdit> mEdit;
private slots:
    void init();
    void test_get_token_data();
    void test_get_token();
    void test_token_start_data();
    void test_token_start();
    void test_token_end_data();
    void test_token_end();

    void test_move_caret_x_data();
    void test_move_caret_x();
    void test_move_caret_y_data();
};

}
