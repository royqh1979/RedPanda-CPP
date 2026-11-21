#ifndef TEST_SEARCHRESULTMODEL_H
#define TEST_SEARCHRESULTMODEL_H

#include <QObject>

class TestSearchResultModel : public QObject
{
    Q_OBJECT
private slots:
    void test_file_checkbox_synchronizes_children();
};

#endif // TEST_SEARCHRESULTMODEL_H
