#ifndef TEST_PROBLEMSETMODEL_H
#define TEST_PROBLEMSETMODEL_H

#include <QObject>

class TestProblemSetModel : public QObject
{
    Q_OBJECT
private slots:
    void test_problem_set_json_roundtrip();
    void test_problem_set_paths_and_write_failure();
    void test_problem_set_model_operations();
    void test_problem_model_operations();
};

#endif
