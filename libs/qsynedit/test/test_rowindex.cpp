#include "qsynedit/rowindex.h"
#include <QTest>
#include "test_rowindex.h"

#include <random>

namespace QSynedit {

namespace {

// brute-force reference model for the randomized property test
struct RefModel {
    QVector<int> segs;
    QVector<bool> hidden;

    int count() const { return segs.count(); }

    void insertLines(int at, int count)
    {
        at = qBound(0, at, (int)segs.count());
        for (int i = 0; i < count; i++) {
            segs.insert(at + i, 1);
            hidden.insert(at + i, false);
        }
    }

    void eraseLines(int at, int count)
    {
        if (at < 0) {
            count += at;
            at = 0;
        }
        count = qMin(count, (int)segs.count() - at);
        for (int i = 0; i < count; i++) {
            segs.remove(at);
            hidden.remove(at);
        }
    }

    void setHidden(int from, int to, bool h)
    {
        from = qMax(from, 0);
        to = qMin(to, count() - 1);
        for (int i = from; i <= to; i++)
            hidden[i] = h;
    }

    int totalSegsBefore(int line) const
    {
        int acc = 0;
        for (int i = 0; i < line; i++)
            acc += segs[i];
        return acc;
    }

    int hiddenSegsBefore(int line) const
    {
        int acc = 0;
        for (int i = 0; i < line; i++)
            if (hidden[i])
                acc += segs[i];
        return acc;
    }

    int visibleSegCount() const
    {
        return totalSegsBefore(count()) - hiddenSegsBefore(count());
    }

    void lineSegAtVisibleRow(int row, int &line, int &segment) const
    {
        line = 0;
        segment = 0;
        int vis = visibleSegCount();
        if (vis <= 0)
            return;
        row = qBound(0, row, vis - 1);
        for (int i = 0; i < count(); i++) {
            if (hidden[i])
                continue;
            if (row < segs[i]) {
                line = i;
                segment = row;
                return;
            }
            row -= segs[i];
        }
        Q_ASSERT(false); // unreachable: row was clamped
    }

    int nextVisibleLine(int line) const
    {
        for (int i = line + 1; i < count(); i++)
            if (!hidden[i])
                return i;
        return -1;
    }

    int prevVisibleLine(int line) const
    {
        for (int i = qMin(line, count()) - 1; i >= 0; i--)
            if (!hidden[i])
                return i;
        return -1;
    }
};

// returns an error description, empty when the tree matches the reference
QString compareAgainstRef(const RowIndex &tree, const RefModel &ref)
{
    if (tree.count() != ref.count())
        return QString("count: %1 vs %2").arg(tree.count()).arg(ref.count());
    if (tree.visibleSegCount() != ref.visibleSegCount())
        return QString("visibleSegCount: %1 vs %2")
                .arg(tree.visibleSegCount()).arg(ref.visibleSegCount());
    if (tree.totalSegCount() != ref.totalSegsBefore(ref.count()))
        return QString("totalSegCount: %1 vs %2")
                .arg(tree.totalSegCount()).arg(ref.totalSegsBefore(ref.count()));
    for (int i = 0; i < ref.count(); i++) {
        if (tree.segCount(i) != ref.segs[i])
            return QString("segCount(%1): %2 vs %3")
                    .arg(i).arg(tree.segCount(i)).arg(ref.segs[i]);
        if (tree.isHidden(i) != bool(ref.hidden[i]))
            return QString("isHidden(%1): %2 vs %3")
                    .arg(i).arg(tree.isHidden(i)).arg(bool(ref.hidden[i]));
        if (tree.totalSegsBefore(i) != ref.totalSegsBefore(i))
            return QString("totalSegsBefore(%1): %2 vs %3")
                    .arg(i).arg(tree.totalSegsBefore(i)).arg(ref.totalSegsBefore(i));
        if (tree.hiddenSegsBefore(i) != ref.hiddenSegsBefore(i))
            return QString("hiddenSegsBefore(%1): %2 vs %3")
                    .arg(i).arg(tree.hiddenSegsBefore(i)).arg(ref.hiddenSegsBefore(i));
        if (tree.nextVisibleLine(i) != ref.nextVisibleLine(i))
            return QString("nextVisibleLine(%1): %2 vs %3")
                    .arg(i).arg(tree.nextVisibleLine(i)).arg(ref.nextVisibleLine(i));
        if (tree.prevVisibleLine(i) != ref.prevVisibleLine(i))
            return QString("prevVisibleLine(%1): %2 vs %3")
                    .arg(i).arg(tree.prevVisibleLine(i)).arg(ref.prevVisibleLine(i));
    }
    int vis = ref.visibleSegCount();
    QList<int> rows{0, vis / 2, vis - 1, vis, vis + 5, -1};
    for (int row : rows) {
        int l1, s1, l2, s2;
        tree.lineSegAtVisibleRow(row, l1, s1);
        ref.lineSegAtVisibleRow(row, l2, s2);
        if (l1 != l2 || s1 != s2)
            return QString("lineSegAtVisibleRow(%1): (%2,%3) vs (%4,%5)")
                    .arg(row).arg(l1).arg(s1).arg(l2).arg(s2);
    }
    return QString();
}

}

void TestRowIndex::test_empty()
{
    RowIndex tree;
    QCOMPARE(tree.count(), 0);
    QCOMPARE(tree.visibleSegCount(), 0);
    QCOMPARE(tree.totalSegCount(), 0);
    QCOMPARE(tree.visibleSegsBefore(0), 0);
    int line, seg;
    tree.lineSegAtVisibleRow(5, line, seg);
    QCOMPARE(line, 0);
    QCOMPARE(seg, 0);
    QCOMPARE(tree.nextVisibleLine(0), -1);
    QCOMPARE(tree.prevVisibleLine(5), -1);
}

void TestRowIndex::test_reset_and_counts()
{
    RowIndex tree;
    tree.reset(10);
    QCOMPARE(tree.count(), 10);
    QCOMPARE(tree.visibleSegCount(), 10);
    QCOMPARE(tree.totalSegCount(), 10);
    QCOMPARE(tree.segCount(0), 1);
    QCOMPARE(tree.segCount(9), 1);
    QCOMPARE(tree.visibleSegsBefore(5), 5);
    QCOMPARE(tree.visibleSegsBefore(10), 10);
    tree.clear();
    QCOMPARE(tree.count(), 0);
}

void TestRowIndex::test_insert_lines()
{
    RowIndex tree;
    tree.insertLines(0, 3);
    QCOMPARE(tree.count(), 3);
    tree.insertLines(1, 2);
    QCOMPARE(tree.count(), 5);
    tree.setSegCount(0, 7);
    tree.setSegCount(1, 8);
    tree.setSegCount(2, 9);
    tree.setSegCount(3, 10);
    tree.setSegCount(4, 11);
    // insert in the middle: later lines shift, values preserved
    tree.insertLines(2, 2);
    QCOMPARE(tree.count(), 7);
    QCOMPARE(tree.segCount(0), 7);
    QCOMPARE(tree.segCount(1), 8);
    QCOMPARE(tree.segCount(2), 1); // new
    QCOMPARE(tree.segCount(3), 1); // new
    QCOMPARE(tree.segCount(4), 9);
    QCOMPARE(tree.segCount(5), 10);
    QCOMPARE(tree.segCount(6), 11);
    QCOMPARE(tree.totalSegCount(), 7 + 8 + 1 + 1 + 9 + 10 + 11);
    // `at` clamps to the end
    tree.insertLines(100, 1);
    QCOMPARE(tree.count(), 8);
    QCOMPARE(tree.segCount(7), 1);
}

void TestRowIndex::test_erase_lines()
{
    RowIndex tree;
    tree.reset(10);
    for (int i = 0; i < 10; i++)
        tree.setSegCount(i, i + 1);
    tree.eraseLines(3, 2); // remove segs 4 and 5
    QCOMPARE(tree.count(), 8);
    QCOMPARE(tree.segCount(2), 3);
    QCOMPARE(tree.segCount(3), 6);
    QCOMPARE(tree.segCount(7), 10);
    QCOMPARE(tree.totalSegCount(), 55 - 4 - 5);
    // clamped range: erase past the end
    tree.eraseLines(6, 100);
    QCOMPARE(tree.count(), 6);
    // erase everything
    tree.eraseLines(0, 100);
    QCOMPARE(tree.count(), 0);
    QCOMPARE(tree.visibleSegCount(), 0);
}

void TestRowIndex::test_set_seg_count()
{
    RowIndex tree;
    tree.reset(5);
    tree.setSegCount(2, 4);
    QCOMPARE(tree.totalSegCount(), 8);
    QCOMPARE(tree.visibleSegsBefore(3), 6);
    QCOMPARE(tree.visibleSegsBefore(2), 2);
    // prefix sums of later lines track the change
    QCOMPARE(tree.visibleSegsBefore(5), 8);
    tree.setSegCount(2, 1);
    QCOMPARE(tree.totalSegCount(), 5);
}

void TestRowIndex::test_reset_seg_counts()
{
    RowIndex tree;
    tree.reset(6);
    tree.setSegCount(1, 4);
    tree.setSegCount(3, 2);
    tree.setSegCount(4, 5);
    tree.setHidden(3, 4, true);
    QCOMPARE(tree.totalSegCount(), 14);
    QCOMPARE(tree.visibleSegCount(), 14 - 7);
    tree.resetSegCounts();
    QCOMPARE(tree.totalSegCount(), 6);
    QCOMPARE(tree.visibleSegCount(), 4);
    QCOMPARE(tree.segCount(1), 1);
    QCOMPARE(tree.segCount(3), 1);
    // hidden flags survive the reset
    QVERIFY(tree.isHidden(3));
    QVERIFY(tree.isHidden(4));
    QVERIFY(!tree.isHidden(2));
    QCOMPARE(tree.hiddenSegsBefore(5), 2);
    // and queries keep working afterwards
    int line, seg;
    tree.lineSegAtVisibleRow(3, line, seg);
    QCOMPARE(line, 5);
    QCOMPARE(seg, 0);
    tree.resetSegCounts(3);
    QCOMPARE(tree.totalSegCount(), 18);
    QCOMPARE(tree.visibleSegCount(), 12);
}

void TestRowIndex::test_hidden_basic()
{
    RowIndex tree;
    tree.reset(10);
    tree.setSegCount(3, 3);
    tree.setSegCount(4, 5);
    // hide lines 3..4 (total segs 16; hidden segs 3+5=8)
    tree.setHidden(3, 4, true);
    QVERIFY(!tree.isHidden(2));
    QVERIFY(tree.isHidden(3));
    QVERIFY(tree.isHidden(4));
    QVERIFY(!tree.isHidden(5));
    QCOMPARE(tree.visibleSegCount(), 16 - 8);
    QCOMPARE(tree.hiddenSegsBefore(5), 8);
    QCOMPARE(tree.visibleSegsBefore(5), 3); // lines 0,1,2: 1+1+1
    tree.setHidden(3, 4, false);
    QCOMPARE(tree.visibleSegCount(), 16);
    QVERIFY(!tree.isHidden(3));
}

void TestRowIndex::test_hidden_overlapping_ranges()
{
    RowIndex tree;
    tree.reset(10);
    // callers pass non-overlapping effective ranges, but arbitrary
    // sequences must still behave as plain range assignment
    tree.setHidden(2, 5, true);
    tree.setHidden(4, 7, true);
    for (int i = 2; i <= 7; i++)
        QVERIFY(tree.isHidden(i));
    QVERIFY(!tree.isHidden(1));
    QVERIFY(!tree.isHidden(8));
    QCOMPARE(tree.visibleSegCount(), 4);
    // partially unhide
    tree.setHidden(3, 5, false);
    QVERIFY(tree.isHidden(2));
    QVERIFY(!tree.isHidden(3));
    QVERIFY(!tree.isHidden(5));
    QVERIFY(tree.isHidden(6));
    QCOMPARE(tree.visibleSegCount(), 7);
    // hide all, then unhide all
    tree.setHidden(0, 9, true);
    QCOMPARE(tree.visibleSegCount(), 0);
    int line, seg;
    tree.lineSegAtVisibleRow(3, line, seg);
    QCOMPARE(line, 0);
    QCOMPARE(seg, 0);
    tree.setHidden(0, 9, false);
    QCOMPARE(tree.visibleSegCount(), 10);
}

void TestRowIndex::test_line_seg_at_visible_row()
{
    RowIndex tree;
    tree.reset(5);
    tree.setSegCount(0, 3); // rows 0..2
    tree.setSegCount(1, 1); // row 3, hidden below
    tree.setSegCount(2, 2); // rows 4..5
    tree.setSegCount(3, 2); // rows 6..7
    // line 4: 1 seg, row 8
    tree.setHidden(1, 1, true);
    // visible rows: line0 rows 0-2, line2 rows 3-4, line3 rows 5-6, line4 row 7
    QCOMPARE(tree.visibleSegCount(), 8);
    int line, seg;
    tree.lineSegAtVisibleRow(0, line, seg);
    QCOMPARE(line, 0);
    QCOMPARE(seg, 0);
    tree.lineSegAtVisibleRow(2, line, seg);
    QCOMPARE(line, 0);
    QCOMPARE(seg, 2);
    tree.lineSegAtVisibleRow(3, line, seg);
    QCOMPARE(line, 2);
    QCOMPARE(seg, 0);
    tree.lineSegAtVisibleRow(6, line, seg);
    QCOMPARE(line, 3);
    QCOMPARE(seg, 1);
    tree.lineSegAtVisibleRow(7, line, seg);
    QCOMPARE(line, 4);
    QCOMPARE(seg, 0);
    // clamp: past the end -> last visible segment
    tree.lineSegAtVisibleRow(100, line, seg);
    QCOMPARE(line, 4);
    QCOMPARE(seg, 0);
    // clamp: negative -> first segment
    tree.lineSegAtVisibleRow(-5, line, seg);
    QCOMPARE(line, 0);
    QCOMPARE(seg, 0);
}

void TestRowIndex::test_next_prev_visible()
{
    RowIndex tree;
    tree.reset(10);
    tree.setHidden(3, 6, true);
    QCOMPARE(tree.nextVisibleLine(2), 7);
    QCOMPARE(tree.nextVisibleLine(3), 7); // hidden start
    QCOMPARE(tree.nextVisibleLine(6), 7);
    QCOMPARE(tree.prevVisibleLine(7), 2);
    QCOMPARE(tree.prevVisibleLine(6), 2); // hidden end
    QCOMPARE(tree.prevVisibleLine(3), 2);
    QCOMPARE(tree.nextVisibleLine(9), -1);
    QCOMPARE(tree.prevVisibleLine(0), -1);
    // hidden suffix/prefix
    tree.setHidden(7, 9, true);
    QCOMPARE(tree.nextVisibleLine(2), -1);
    QCOMPARE(tree.prevVisibleLine(9), 2);
}

void TestRowIndex::test_random_against_reference()
{
    for (unsigned seed : {1u, 7u, 12345u}) {
        std::mt19937 rng(seed);
        RowIndex tree;
        RefModel ref;
        QStringList opLog;
        tree.insertLines(0, 50);
        ref.insertLines(0, 50);
        for (int op = 0; op < 2000; op++) {
            int kind = rng() % 100;
            int n = ref.count();
            if (kind < 30 && n < 600) {
                int at = int(rng() % (n + 2)) - 1; // exercise clamping
                int count = 1 + rng() % 20;
                tree.insertLines(at, count);
                ref.insertLines(at, count);
                opLog.append(QString("insertLines(%1, %2)").arg(at).arg(count));
            } else if (kind < 50 && n > 0) {
                int at = int(rng() % (n + 2)) - 1;
                int count = 1 + rng() % 20;
                tree.eraseLines(at, count);
                ref.eraseLines(at, count);
                opLog.append(QString("eraseLines(%1, %2)").arg(at).arg(count));
            } else if (kind < 75 && n > 0) {
                int line = rng() % n;
                int segs = 1 + rng() % 6;
                tree.setSegCount(line, segs);
                ref.segs[line] = segs;
                opLog.append(QString("setSegCount(%1, %2)").arg(line).arg(segs));
            } else if (n > 0) {
                int a = int(rng() % (n + 1)) - 1;
                int b = int(rng() % (n + 1)) - 1;
                bool h = rng() % 2;
                tree.setHidden(qMin(a, b), qMax(a, b), h);
                ref.setHidden(qMin(a, b), qMax(a, b), h);
                opLog.append(QString("setHidden(%1, %2, %3)")
                             .arg(qMin(a, b)).arg(qMax(a, b)).arg(h));
            } else {
                opLog.append("nop");
            }
            if (op % 10 == 9) {
                QString err = compareAgainstRef(tree, ref);
                if (!err.isEmpty()) {
                    qDebug() << "seed" << seed << "op" << op << ":" << err;
                    qDebug() << "recent ops:" << opLog.mid(qMax(0, opLog.count() - 25));
                    QFAIL("tree/reference mismatch");
                }
            }
        }
        QString err = compareAgainstRef(tree, ref);
        if (!err.isEmpty()) {
            qDebug() << "seed" << seed << "final:" << err;
            QFAIL("tree/reference mismatch");
        }
    }
}

}
