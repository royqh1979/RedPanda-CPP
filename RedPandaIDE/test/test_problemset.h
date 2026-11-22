#ifndef TEST_PROBLEMSET_H
#define TEST_PROBLEMSET_H

#include <QObject>

class TestProblemSet : public QObject
{
    Q_OBJECT
private slots:
    void test_case_modification_propagates();
    void test_problem_case_operations();
    void test_set_problem_operations();
    void test_limit_conversions();
    void test_all_property_changes_mark_modified();
};

#endif
