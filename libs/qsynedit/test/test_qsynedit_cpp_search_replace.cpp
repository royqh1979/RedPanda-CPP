#include <QtTest>
#include <QCoreApplication>

#include "test_qsynedit_cpp_search_replace.h"
#include "qsynedit/qsynedit.h"
#include "qsynedit/document.h"
#include "qsynedit/syntaxer/cpp.h"
#include "qsynedit/formatter/cppformatter.h"
#include "qsynedit/searcher/basicsearcher.h"

namespace QSynedit {


void TestQSyneditCppSearchReplace::initTestCase()
{
    mEdit = std::make_shared<QSynEdit>();
    QSynedit::EditorOptions options = QSynedit::EditorOption::AltSetsColumnMode
            | QSynedit::EditorOption::DragDropEditing | QSynedit::EditorOption::DropFiles
            | QSynedit::EditorOption::EnhanceHomeKey | QSynedit::EditorOption::EnhanceEndKey
            | QSynedit::EditorOption::RightMouseMovesCursor
            | QSynedit::EditorOption::TabIndent
            | QSynedit::EditorOption::GroupUndo
            | QSynedit::EditorOption::AutoIndent
            | QSynedit::EditorOption::SelectWordByDblClick;
    mEdit->setOptions(options);
    mEdit->setSyntaxer(std::make_shared<CppSyntaxer>());
    mEdit->setFormatter(std::make_shared<CppFormatter>());

    mBasicSearcher = std::make_shared<BasicSearcher>();
    mBasicMatchedAndExitProc = [this](const QString& sFound,
            const QString& sReplace, const CharPos& pos, int wordLen){
        mFoundStrings.append(sFound);
        mReplaces.append(sReplace);
        mFoundPositions.append(pos);
        mFoundLens.append(wordLen);
        return SearchAction::Exit;
    };
    mBasicMatchedAndContinueProc = [this](const QString& sFound,
            const QString& sReplace, const CharPos& pos, int wordLen){
        mFoundStrings.append(sFound);
        mReplaces.append(sReplace);
        mFoundPositions.append(pos);
        mFoundLens.append(wordLen);
        return SearchAction::Skip;
    };
}

void TestQSyneditCppSearchReplace::clearFounds()
{
    mFoundStrings.clear();
    mReplaces.clear();
    mFoundPositions.clear();
    mFoundLens.clear();
}

void TestQSyneditCppSearchReplace::test_basic_search_forward_in_empty_doc()
{
    SearchOptions options = SearchOption::ssoFromCaret;
    mEdit->clear();
    clearFounds();
    QCOMPARE(mEdit->searchReplace(
                 "thread","",
                 mEdit->fileBegin(),
                 mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),0);
    QCOMPARE(mEdit->caretXY(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretXY(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));

}

void TestQSyneditCppSearchReplace::test_basic_search_forward_without_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret;

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(10,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(18,18));
    QCOMPARE(mEdit->selBegin(), CharPos(12,18));
    QCOMPARE(mEdit->selEnd(), CharPos(18,18));


    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(11,48));
    QCOMPARE(mEdit->selBegin(), CharPos(5,48));
    QCOMPARE(mEdit->selEnd(), CharPos(11,48));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(27,52));
    QCOMPARE(mEdit->selBegin(), CharPos(21,52));
    QCOMPARE(mEdit->selEnd(), CharPos(27,52));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(27,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(35,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(10,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));

    //not found
     QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),0);
    QCOMPARE(mEdit->caretXY(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
}

void TestQSyneditCppSearchReplace::test_basic_search_forward_without_wrap_with_callback()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret;

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,15)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(10,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(12,18)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(18,18));
    QCOMPARE(mEdit->selBegin(), CharPos(12,18));
    QCOMPARE(mEdit->selEnd(), CharPos(18,18));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(5,48)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(11,48));
    QCOMPARE(mEdit->selBegin(), CharPos(5,48));
    QCOMPARE(mEdit->selEnd(), CharPos(11,48));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,52)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(27,52));
    QCOMPARE(mEdit->selBegin(), CharPos(21,52));
    QCOMPARE(mEdit->selEnd(), CharPos(27,52));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(27,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(35,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(10,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(22,72)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));

    //not found
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretXY(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
}

void TestQSyneditCppSearchReplace::test_basic_search_forward_with_selection()
{
    //when caret is at selection begin/end, forward search will start from selection end
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret;

    //no selection, caret at the second "thread" begin
    //should found the second "thread"
    clearFounds();
    mEdit->setCaretXY(CharPos{6,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));

    //no selection, caret at the second "thread" end
    //should found the third "thread"
    clearFounds();
    mEdit->setCaretXY(CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,15)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(10,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));

    //the second "thread" selected, caret at selection begin
    //should found the third "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{6,12},CharPos{6,12},CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,15)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(10,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));

    //the second "thread" selected, caret at selection end
    //should found the third "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{12,12},CharPos{6,12},CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,15)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(10,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));

    // the third "thread" selected, caret before the second "thread"
    // should found the second "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{4,12},CharPos{4,15},CharPos{4,15});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));

    // the third "thread" selected, caret after the fifth "thread"
    // should found the sixth "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{16,19},CharPos{4,15},CharPos{4,15});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(5,48)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(11,48));
    QCOMPARE(mEdit->selBegin(), CharPos(5,48));
    QCOMPARE(mEdit->selEnd(), CharPos(11,48));
}

void TestQSyneditCppSearchReplace::test_basic_search_forward_ignore_caret_pos()
{
    //when caret is at selection begin/end, forward search will start from selection end
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoNone;

    //no selection, caret at the second "thread" begin
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretXY(CharPos{6,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));

    //no selection, caret at the second "thread" end
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretXY(CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));

    //the second "thread" selected, caret at selection begin
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{6,12},CharPos{6,12},CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));

    //the second "thread" selected, caret at selection end
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{12,12},CharPos{6,12},CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));

    // the third "thread" selected, caret before the second "thread"
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{4,12},CharPos{4,15},CharPos{4,15});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));

    // the third "thread" selected, caret after the fifth "thread"
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{16,19},CharPos{4,15},CharPos{4,15});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
}

void TestQSyneditCppSearchReplace::test_basic_search_forward_with_wrap()
{
    //when caret is at selection begin/end, forward search will start from selection end
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoWrapAround;

    //caret before last "thread"
    mEdit->setCaretXY({10,67});

    //found the last "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(22,72)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));

    //found the first "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));

    //caret at file end, and don't confirm wrap
    mEdit->setCaretXY(mEdit->fileEnd());
    clearFounds();
    QCOMPARE(mEdit->searchReplace(
                 "thread","",
                 mEdit->fileBegin(),
                 mEdit->fileEnd(),
                 options,
                 mBasicSearcher.get(),
                 mBasicMatchedAndExitProc,
                 [](){ return false; }),
             0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretXY(), mEdit->fileEnd());
    QCOMPARE(mEdit->selBegin(), mEdit->fileEnd());
    QCOMPARE(mEdit->selEnd(), mEdit->fileEnd());

    //caret at file end, and confirm wrap
    mEdit->setCaretXY(mEdit->fileEnd());
    clearFounds();
    QCOMPARE(mEdit->searchReplace(
                 "thread","",
                 mEdit->fileBegin(),
                 mEdit->fileEnd(),
                 options,
                 mBasicSearcher.get(),
                 mBasicMatchedAndExitProc,
                 [](){ return true; }),
             1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
}

void TestQSyneditCppSearchReplace::test_search_all_in_whole_file_from_start()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoNone;
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),10);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "Thread",
                     "Thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                 }));
    QCOMPARE(mReplaces,
             QStringList(
                 {
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(10,3),
                     CharPos(6,12),
                     CharPos(4,15),
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
                     CharPos(4,67),
                     CharPos(22,72)

             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                 }));

}

void TestQSyneditCppSearchReplace::test_search_all_in_whole_file_from_caret_and_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoWrapAround;

    //before the 3rd "thread"
    mEdit->setCaretXY({3,15});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),10);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
                     "Thread",
                 }));
    QCOMPARE(mReplaces,
             QStringList(
                 {
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(4,15),
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
                     CharPos(4,67),
                     CharPos(22,72),
                     CharPos(10,3),
                     CharPos(6,12),
             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                 }));
}

void TestQSyneditCppSearchReplace::test_search_all_in_whole_file_from_caret_and_no_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret;

    //before the 3rd "thread"
    mEdit->setCaretXY({3,15});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),8);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                 }));
    QCOMPARE(mReplaces,
             QStringList(
                 {
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                     "",
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(4,15),
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
                     CharPos(4,67),
                     CharPos(22,72),
             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                 }));
}

void TestQSyneditCppSearchReplace::test_basic_search_backward_in_empty_doc()
{
    mEdit->clear();

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoBackwards;
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),0);
    QCOMPARE(mEdit->caretXY(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretXY(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
}

void TestQSyneditCppSearchReplace::test_basic_search_backward_without_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoBackwards;

    //caret at file end
    mEdit->setCaretXY(mEdit->fileEnd());

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(22,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(29,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(21,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(21,52));
    QCOMPARE(mEdit->selBegin(), CharPos(21,52));
    QCOMPARE(mEdit->selEnd(), CharPos(27,52));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(5,48));
    QCOMPARE(mEdit->selBegin(), CharPos(5,48));
    QCOMPARE(mEdit->selEnd(), CharPos(11,48));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(12,18));
    QCOMPARE(mEdit->selBegin(), CharPos(12,18));
    QCOMPARE(mEdit->selEnd(), CharPos(18,18));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(4,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(6,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(10,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));

    //not found
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),0);
    QCOMPARE(mEdit->caretXY(), CharPos(10,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
}

void TestQSyneditCppSearchReplace::test_basic_search_backward_without_wrap_with_callback()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoBackwards;

    //caret at file end
    mEdit->setCaretXY(mEdit->fileEnd());

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(22,72)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(22,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(29,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(21,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,52)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(21,52));
    QCOMPARE(mEdit->selBegin(), CharPos(21,52));
    QCOMPARE(mEdit->selEnd(), CharPos(27,52));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(5,48)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(5,48));
    QCOMPARE(mEdit->selBegin(), CharPos(5,48));
    QCOMPARE(mEdit->selEnd(), CharPos(11,48));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(12,18)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(12,18));
    QCOMPARE(mEdit->selBegin(), CharPos(12,18));
    QCOMPARE(mEdit->selEnd(), CharPos(18,18));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,15)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(4,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(6,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(10,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
}

void TestQSyneditCppSearchReplace::test_basic_search_backward_with_selection()
{
    //when caret is at selection begin/end, backward search will start from selection begin
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoBackwards;

    //no selection, caret at the 9th "thread" end
    //should found the 9th "thread"
    clearFounds();
    mEdit->setCaretXY(CharPos(10,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));

    //no selection, caret at the 9th "thread" begin
    //should found the 8th "thread"
    clearFounds();
    mEdit->setCaretXY(CharPos(4,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(29,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));

    //the 9th "thread" selected, caret at selection begin
    //should found the 8th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(4,67),CharPos(4,67),CharPos(10,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(29,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));

    //the 9th "thread" selected, caret at selection end
    //should found the 8th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(10,67),CharPos(4,67),CharPos(10,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(29,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));

    //the 8th "thread" selected, caret after the 9th "thread"
    //should found the 9th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(0,68),CharPos(29,62),CharPos(35,62));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));

    //the 8th "thread" selected, caret before the 7th "thread"
    //should found the 6th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(28,62),CharPos(29,62),CharPos(35,62));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(21,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));
}

void TestQSyneditCppSearchReplace::test_basic_search_backward_with_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoBackwards | SearchOption::ssoWrapAround;

    //found the first "thread"
    mEdit->setCaretXY({16,3});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(10,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));

    //found the last "thread" (wrap around)
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(22,72)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(22,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));

    //at file begin, don't confirm
    mEdit->setCaretXY({0,0});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         [](){ return false; }),
             0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretXY(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));

    // at file begin, confirm
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         [](){ return true; }),
             1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(22,72)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretXY(), CharPos(22,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));

}

void QSynedit::TestQSyneditCppSearchReplace::test_basic_forward_search_without_wrap2()
{
//    QByteArray encoding;
//    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

//    SearchOptions options = SearchOption::ssoEntireScope;
//    mEdit->searchReplace("stop","",options,
//                         mBasicSearcher.get(),
//                         mBasicMatchedProc,
//                         nullptr);
//    QCOMPARE(mFoundStrings, {"stop"});
//    QCOMPARE(mReplaces, {""});
//    QCOMPARE(mFoundPositions, {CharPos(29,15)});
//    QCOMPARE(mFoundLens, {4});
//    QCOMPARE(mEdit->caretXY(), CharPos(33,15));
//    QCOMPARE(mEdit->selBegin(), CharPos(29,15));
//    QCOMPARE(mEdit->selEnd(), CharPos(33,15));

    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret;

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(33,15));
    QCOMPARE(mEdit->selBegin(), CharPos(29,15));
    QCOMPARE(mEdit->selEnd(), CharPos(33,15));

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(65,23));
    QCOMPARE(mEdit->selBegin(), CharPos(61,23));
    QCOMPARE(mEdit->selEnd(), CharPos(65,23));

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(52,26));
    QCOMPARE(mEdit->selBegin(), CharPos(48,26));
    QCOMPARE(mEdit->selEnd(), CharPos(52,26));

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(12,49));
    QCOMPARE(mEdit->selBegin(), CharPos(8,49));
    QCOMPARE(mEdit->selEnd(), CharPos(12,49));

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretXY(), CharPos(26,59));
    QCOMPARE(mEdit->selBegin(), CharPos(22,59));
    QCOMPARE(mEdit->selEnd(), CharPos(26,59));

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),0);
    QCOMPARE(mEdit->caretXY(), CharPos(26,59));
    QCOMPARE(mEdit->selBegin(), CharPos(22,59));
    QCOMPARE(mEdit->selEnd(), CharPos(26,59));

}

}
