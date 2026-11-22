#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include "test_freeproblemsetformat.h"
#include "src/problems/freeprojectsetformat.h"
#include "src/utils.h"

static void writeXml(const QString &path, const QByteArray &content)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(content), content.size());
}

void TestFreeProblemSetFormat::test_roundtrip()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto problem = std::make_shared<OJProblem>();
    problem->setName(QString::fromUtf8("A&B <题>"));
    problem->setUrl("https://example.com/?a=1&b=2");
    problem->setDescription(QString::fromUtf8("<p>描述 & 内容</p>"));
    problem->setHint("line 1\nline 2");
    problem->setTimeLimit(1500);
    problem->setTimeLimitUnit(ProblemTimeLimitUnit::Milliseconds);
    problem->setMemoryLimit(512);
    problem->setMemoryLimitUnit(ProblemMemoryLimitUnit::MB);
    auto problemCase = std::make_shared<OJProblemCase>();
    problemCase->setName("Problem Case 1");
    problemCase->setInput(QString::fromUtf8("1 2\n汉字\n"));
    problemCase->setExpected(QString::fromUtf8("3\n结果\n"));
    problem->addCase(problemCase);
    auto second = std::make_shared<OJProblem>();
    second->setName("B");
    second->setTimeLimit(2);
    second->setTimeLimitUnit(ProblemTimeLimitUnit::Seconds);
    second->setMemoryLimit(2);
    second->setMemoryLimitUnit(ProblemMemoryLimitUnit::GB);

    QString path = directory.filePath("set.xml");
    exportFreeProblemSet({problem, second}, path);
    QList<POJProblem> loaded = importFreeProblemSet(path);
    QCOMPARE(loaded.count(), 2);
    QCOMPARE(loaded.at(0)->name(), problem->name());
    QCOMPARE(loaded.at(0)->url(), problem->url());
    QCOMPARE(loaded.at(0)->description(), problem->description());
    QCOMPARE(loaded.at(0)->hint(), problem->hint());
    QCOMPARE(loaded.at(0)->timeLimit(), problem->timeLimit());
    QCOMPARE(loaded.at(0)->timeLimitUnit(), problem->timeLimitUnit());
    QCOMPARE(loaded.at(0)->memoryLimit(), problem->memoryLimit());
    QCOMPARE(loaded.at(0)->memoryLimitUnit(), problem->memoryLimitUnit());
    QCOMPARE(loaded.at(0)->cases().count(), 1);
    QCOMPARE(loaded.at(0)->cases().at(0)->input(), problemCase->input());
    QCOMPARE(loaded.at(0)->cases().at(0)->expected(), problemCase->expected());
    QVERIFY(!loaded.at(0)->isModified());
    QVERIFY(!loaded.at(0)->cases().at(0)->isModified());
    QCOMPARE(loaded.at(1)->name(), second->name());
    QCOMPARE(loaded.at(1)->timeLimitUnit(), ProblemTimeLimitUnit::Seconds);
    QCOMPARE(loaded.at(1)->memoryLimitUnit(), ProblemMemoryLimitUnit::GB);
}

void TestFreeProblemSetFormat::test_case_name_preserved()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString path = directory.filePath("name.xml");
    writeXml(path, R"(<fps><item><title><![CDATA[A]]></title>
        <test_input name="custom name"><![CDATA[input]]></test_input>
        <test_output><![CDATA[output]]></test_output>
        </item></fps>)");
    QList<POJProblem> loaded = importFreeProblemSet(path);
    QCOMPARE(loaded.count(), 1);
    QCOMPARE(loaded.at(0)->cases().count(), 1);
    QCOMPARE(loaded.at(0)->cases().at(0)->name(), "custom name");
}

void TestFreeProblemSetFormat::test_split_character_sections()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString path = directory.filePath("sections.xml");
    writeXml(path, R"(<fps><item><title><![CDATA[A ]]><![CDATA[B]]></title>
        <url><![CDATA[https://example.com/]]><![CDATA[test]]></url>
        <description><![CDATA[desc ]]><![CDATA[ription]]></description>
        <hint><![CDATA[first hint ]]><![CDATA[second hint]]></hint>
        <time_limit unit="ms"><![CDATA[12]]><![CDATA[50]]></time_limit>
        <memory_limit unit="MB"><![CDATA[6]]><![CDATA[4]]></memory_limit>
        <test_input><![CDATA[first ]]><![CDATA[second]]></test_input>
        <test_output><![CDATA[third ]]><![CDATA[fourth]]></test_output>
        </item></fps>)");
    QList<POJProblem> loaded = importFreeProblemSet(path);
    QCOMPARE(loaded.count(), 1);
    QCOMPARE(loaded.at(0)->name(), "A B");
    QCOMPARE(loaded.at(0)->url(), "https://example.com/test");
    QCOMPARE(loaded.at(0)->description(), "desc ription");
    QCOMPARE(loaded.at(0)->hint(), "first hint second hint");
    QCOMPARE(loaded.at(0)->timeLimit(), 1250);
    QCOMPARE(loaded.at(0)->timeLimitUnit(), ProblemTimeLimitUnit::Milliseconds);
    QCOMPARE(loaded.at(0)->memoryLimit(), 64);
    QCOMPARE(loaded.at(0)->memoryLimitUnit(), ProblemMemoryLimitUnit::MB);
    QCOMPARE(loaded.at(0)->cases().count(), 1);
    QCOMPARE(loaded.at(0)->cases().at(0)->input(), "first second");
    QCOMPARE(loaded.at(0)->cases().at(0)->expected(), "third fourth");
}

void TestFreeProblemSetFormat::test_invalid_xml()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString path = directory.filePath("invalid.xml");
    writeXml(path, "<fps><item><title>broken</item></fps>");
    QVERIFY_EXCEPTION_THROWN(importFreeProblemSet(path), FileError);
    QVERIFY_EXCEPTION_THROWN(importFreeProblemSet(directory.filePath("missing.xml")), FileError);
}
