#ifndef TEST_DOCUMENT_PROFILE_H
#define TEST_DOCUMENT_PROFILE_H

#include <QObject>
#include <QStringList>

namespace QSynedit {

class TestDocumentProfile : public QObject
{
    Q_OBJECT
private:
    QStringList mSourceLines;
    int mRepeats = 3;

private slots:
    void initTestCase();
    void test_edit_matrix_data();
    void test_edit_matrix();
};

}

#endif // TEST_DOCUMENT_PROFILE_H
