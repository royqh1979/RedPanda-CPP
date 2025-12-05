#include <QtTest>
#include <QCoreApplication>

#include "qsynedit/qsynedit.h"
#include "test_qsynedit.h"
#include "qsynedit/document.h"

namespace QSynedit {

void TestQSynedit::init()
{
    mEdit = std::make_shared<QSynEdit>();
    QSynedit::EditorOptions options = QSynedit::EditorOption::AltSetsColumnMode
            | QSynedit::EditorOption::DragDropEditing | QSynedit::EditorOption::DropFiles
            | QSynedit::EditorOption::RightMouseMovesCursor
            | QSynedit::EditorOption::TabIndent
            | QSynedit::EditorOption::GroupUndo
            | QSynedit::EditorOption::SelectWordByDblClick;
    mEdit->setOptions(options);
}

void TestQSynedit::test_test1()
{
    QByteArray encoding;
    mEdit->document()->loadFromFile("resources/test1.cpp",ENCODING_AUTO_DETECT,encoding);

    QString s = mEdit->wordAtRowCol(CharPos{1,0});
    QCOMPARE(s,"#include");
    QCOMPARE(mEdit->wordStart(CharPos{1,0}),CharPos(0,0));
    QCOMPARE(mEdit->wordEnd(CharPos{1,0}),CharPos(8,0));
}

}

