#ifndef TEST_QSYNEDIT_BASE_H
#define TEST_QSYNEDIT_BASE_H
#include <QTest>
#include "qsynedit/qsynedit.h"

template <>inline char *QTest::toString(const QSynedit::CharPos &pos) {
    return toString(QString("CharPos(ch=%1,line=%2)").arg(pos.ch).arg(pos.line));
}

namespace QSynedit{

class TestQSyneditBase : public QObject
{
    Q_OBJECT
protected:
    std::shared_ptr<QSynEdit> mEdit;
    QList<int> mDeleteStartLines;
    QList<int> mDeleteLineCounts;
    QList<int> mInsertStartLines;
    QList<int> mInsertLineCounts;
    QList<int> mLineMovedFroms;
    QList<int> mLineMovedTos;
    QList<int> mStatusChanges;
    QList<int> mReparseStarts;
    QList<int> mReparseCounts;
protected:
    void clearReparseDatas();
    void clearSignalDatas();
    void clearContent();
    void connectEditSignals();
protected slots:
    void onLinesDeleted(int line, int count);
    void onLinesInserted(int line, int count);
    void onLineMoved(int from, int to);
    void onStatusChanged(StatusChanges change);
    void onReparsed(int start, int count);
};

}
#endif
