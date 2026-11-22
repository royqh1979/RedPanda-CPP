#ifndef TEST_FREEPROBLEMSETFORMAT_H
#define TEST_FREEPROBLEMSETFORMAT_H

#include <QObject>

class TestFreeProblemSetFormat : public QObject
{
    Q_OBJECT
private slots:
    void test_roundtrip();
    void test_case_name_preserved();
    void test_split_character_sections();
    void test_invalid_xml();
};

#endif
