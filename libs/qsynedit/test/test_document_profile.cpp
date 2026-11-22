#include "test_document_profile.h"

#include <QElapsedTimer>
#include <QFile>
#include <QTest>
#include <climits>

#include "qsynedit/document.h"
#include "qsynedit/syntaxer/cpp.h"

namespace QSynedit {
namespace {

QString profileLine(bool foldBoundary, int index)
{
    QString suffix = foldBoundary ? (index % 2 == 0 ? QStringLiteral("{") : QStringLiteral("}"))
                                  : QStringLiteral(" ");
    return QStringLiteral("static int red_panda_profile_value = 1; ") + suffix;
}

int profileStartLine(const Document &document, int count, const QString &position)
{
    int lastStart = qMax(0, document.count() - count - 1);
    int startLine = qMin((document.count() - count) / 2, lastStart);
    if (position == QStringLiteral("begin"))
        startLine = qMin(1000, lastStart);
    else if (position == QStringLiteral("end"))
        startLine = qMax(0, document.count() - count - 1000);

    auto syntaxer = document.createSyntaxerInstance();
    while (startLine > 0 && startLine < lastStart) {
        PSyntaxState state = document.getSyntaxState(startLine - 1);
        if (!state || (!syntaxer->isCommentNotFinished(state)
                       && !syntaxer->isStringNotFinished(state)))
            break;
        ++startLine;
    }
    return startLine;
}

void replaceLines(Document &document, int startLine, int count, bool foldBoundary)
{
    document.beginUpdate();
    for (int i = 0; i < count; ++i) {
        document.putLine(startLine + i, foldBoundary ? profileLine(true, i)
                         : QStringLiteral("static int red_panda_profile_value = 0;  "),
                         i == count - 1);
    }
    document.endUpdate();
}

void applyEdit(Document &document, const QString &operation, int startLine,
               int count, bool foldBoundary)
{
    document.beginUpdate();
    if (operation == QStringLiteral("modify")) {
        for (int i = 0; i < count; ++i) {
            document.putLine(startLine + i, profileLine(foldBoundary, i),
                             i == count - 1);
        }
    } else if (operation == QStringLiteral("insert")) {
        document.insertLinesDeferred(startLine, count);
        for (int i = 0; i < count; ++i) {
            document.putLine(startLine + i, profileLine(foldBoundary, i),
                             i == count - 1);
        }
    } else if (operation == QStringLiteral("move")) {
        document.moveLines(startLine, startLine + 100, count, true);
    } else if (operation == QStringLiteral("delete")) {
        document.deleteLines(startLine, count, true);
    } else if (operation == QStringLiteral("replace")) {
        document.deleteLines(startLine, count, false);
        document.insertLinesDeferred(startLine, count);
        for (int i = 0; i < count; ++i) {
            document.putLine(startLine + i, profileLine(foldBoundary, i),
                             i == count - 1);
        }
    } else {
        Q_ASSERT(false);
    }
    document.endUpdate();
}

}

void TestDocumentProfile::initTestCase()
{
    if (qEnvironmentVariable("QSYNEDIT_EDIT_PROFILE") != "1")
        QSKIP("Set QSYNEDIT_EDIT_PROFILE=1 to run document edit profiling");
    QFile file("resources/miniaudio.h");
    QVERIFY(file.open(QIODevice::ReadOnly));
    mSourceLines = QString::fromUtf8(file.readAll()).split('\n', Qt::KeepEmptyParts);
    QVERIFY(mSourceLines.count() > 20000);
    bool ok = false;
    int repeats = qEnvironmentVariableIntValue("QSYNEDIT_EDIT_PROFILE_REPEAT", &ok);
    if (ok && repeats > 0)
        mRepeats = repeats;
}

void TestDocumentProfile::test_edit_matrix_data()
{
    QTest::addColumn<QString>("operation");
    QTest::addColumn<int>("count");
    QTest::addColumn<bool>("foldBoundary");
    QTest::addColumn<QString>("position");

    const QStringList operations{"modify", "insert", "delete", "replace", "move"};
    const QList<int> counts{1, 10, 100, 1000, 10000};
    for (const QString &operation : operations) {
        for (int count : counts) {
            for (bool foldBoundary : {false, true}) {
                QString name = QStringLiteral("%1_%2_%3_middle")
                        .arg(operation).arg(count)
                        .arg(foldBoundary ? QStringLiteral("fold") : QStringLiteral("plain"));
                QTest::newRow(name.toUtf8().constData())
                        << operation << count << foldBoundary << QStringLiteral("middle");
            }
        }
    }
    for (int count : {1, 1000}) {
        for (const QString &position : {QStringLiteral("begin"), QStringLiteral("end")}) {
            for (bool foldBoundary : {false, true}) {
                QString name = QStringLiteral("modify_%1_%2_%3")
                        .arg(count).arg(foldBoundary ? QStringLiteral("fold") : QStringLiteral("plain"), position);
                QTest::newRow(name.toUtf8().constData())
                        << QStringLiteral("modify") << count << foldBoundary << position;
            }
        }
    }
}

void TestDocumentProfile::test_edit_matrix()
{
    QFETCH(QString, operation);
    QFETCH(int, count);
    QFETCH(bool, foldBoundary);
    QFETCH(QString, position);

    for (int sample = 0; sample < mRepeats; ++sample) {
        auto document = std::make_shared<Document>();
        document->setContents(mSourceLines);
        document->setSyntaxer(std::make_shared<CppSyntaxer>());
        int startLine = profileStartLine(*document, count, position);
        if (operation != QStringLiteral("insert")) {
            bool prepareFold = (operation == QStringLiteral("delete")
                                || operation == QStringLiteral("move")) && foldBoundary;
            replaceLines(*document, startLine, count, prepareFold);
        }

        QObject context;
        int reparsedLines = 0;
        int changeEntries = 0;
        int changedLines = 0;
        bool foldStateDirty = false;
        connect(document.get(), &Document::linesReparsed, &context,
                [&](int firstLine, int endLine) {
            int boundedEnd = endLine == INT_MAX ? document->count()
                                                : qMin(endLine, document->count());
            reparsedLines += qMax(0, boundedEnd - firstLine);
        });
        connect(document.get(), &Document::contentChanged, &context,
                [&](const DocumentChangeSet &changes, bool dirty, int, bool) {
            changeEntries += changes.count();
            for (const DocumentChange &change : changes)
                changedLines += change.kind == DocumentChangeKind::Move ? change.moveLineCount : change.count;
            foldStateDirty = foldStateDirty || dirty;
        });

        int oldLineCount = document->count();
        QElapsedTimer timer;
        timer.start();
        applyEdit(*document, operation, startLine, count, foldBoundary);
        qint64 elapsed = timer.nsecsElapsed();

        int expectedLineCount = oldLineCount;
        if (operation == QStringLiteral("insert"))
            expectedLineCount += count;
        else if (operation == QStringLiteral("delete"))
            expectedLineCount -= count;
        QCOMPARE(document->count(), expectedLineCount);
        QVERIFY(!document->inUpdate());
        QVERIFY(reparsedLines > 0);
        QCOMPARE(foldStateDirty, foldBoundary);

        qInfo().noquote() << QStringLiteral(
                    "DOCUMENT_EDIT_PROFILE,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10")
                .arg(operation).arg(count)
                .arg(foldBoundary ? QStringLiteral("fold") : QStringLiteral("plain"), position)
                .arg(sample).arg(elapsed).arg(reparsedLines)
                .arg(changeEntries).arg(changedLines).arg(foldStateDirty ? 1 : 0);
    }
}

}
