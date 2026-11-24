#ifndef TEST_CPPPARSERPUBLICAPIBASELINE_H
#define TEST_CPPPARSERPUBLICAPIBASELINE_H

#include "test_cppparsertestcase.h"

#ifdef REDPANDA_PARSER_PUBLIC_API_BASELINE_TEST
class TestCppParserPublicApiBaseline : public CppParserTestCase
{
    Q_OBJECT
private slots:
    void init() { init_parser(); }
    void test_cpp17_public_api_validation_and_eval_profile();
    void test_c11_public_api_validation_and_eval_profile();

private:
    void verifyAndProfile(const QString &stem);
};
#endif

#endif // TEST_CPPPARSERPUBLICAPIBASELINE_H
