#include <QTest>
#include <QCoreApplication>

#include "test_qsynedit_cpp_search_replace.h"
#include "qsynedit/qsynedit.h"
#include "qsynedit/document.h"
#include "qsynedit/syntaxer/cpp.h"
#include "qsynedit/formatter/cppformatter.h"
#include "qsynedit/searcher/basicsearcher.h"
#include "qsynedit/searcher/regexsearcher.h"

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
    mRegexSearcher = std::make_shared<RegexSearcher>();
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
    mReplaceAndExitProc = [this](const QString& sFound,
            const QString& sReplace, const CharPos& pos, int wordLen){
        mFoundStrings.append(sFound);
        mReplaces.append(sReplace);
        mFoundPositions.append(pos);
        mFoundLens.append(wordLen);
        return SearchAction::ReplaceAndExit;
    };
    mReplaceAndContinueProc = [this](const QString& sFound,
            const QString& sReplace, const CharPos& pos, int wordLen){
        mFoundStrings.append(sFound);
        mReplaces.append(sReplace);
        mFoundPositions.append(pos);
        mFoundLens.append(wordLen);
        return SearchAction::Replace;
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

    CharPos newEndPos;
    QCOMPARE(mEdit->searchReplace(
                 "thread","",
                 mEdit->fileBegin(),
                 mEdit->fileEnd(),
                 newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),0);
    QCOMPARE(mEdit->caretPos(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(newEndPos,CharPos(0,0));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretPos(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(newEndPos,CharPos(0,0));

}

void TestQSyneditCppSearchReplace::test_basic_search_forward_from_caret_without_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret;

    CharPos newEndPos;
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(10,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(18,18));
    QCOMPARE(mEdit->selBegin(), CharPos(12,18));
    QCOMPARE(mEdit->selEnd(), CharPos(18,18));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(11,48));
    QCOMPARE(mEdit->selBegin(), CharPos(5,48));
    QCOMPARE(mEdit->selEnd(), CharPos(11,48));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(27,52));
    QCOMPARE(mEdit->selBegin(), CharPos(21,52));
    QCOMPARE(mEdit->selEnd(), CharPos(27,52));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(27,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(35,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(10,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
    QCOMPARE(newEndPos,CharPos(1,76));

    //not found
     QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                   newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),0);
    QCOMPARE(mEdit->caretPos(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_basic_search_forward_from_caret_without_wrap_with_callback()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret;
    CharPos newEndPos;
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,15)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(10,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(12,18)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(18,18));
    QCOMPARE(mEdit->selBegin(), CharPos(12,18));
    QCOMPARE(mEdit->selEnd(), CharPos(18,18));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(5,48)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(11,48));
    QCOMPARE(mEdit->selBegin(), CharPos(5,48));
    QCOMPARE(mEdit->selEnd(), CharPos(11,48));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,52)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(27,52));
    QCOMPARE(mEdit->selBegin(), CharPos(21,52));
    QCOMPARE(mEdit->selEnd(), CharPos(27,52));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(27,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(35,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(10,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(22,72)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
    QCOMPARE(newEndPos,CharPos(1,76));

    //not found
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretPos(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_basic_search_forward_from_caret_with_selection()
{
    //when caret is at selection begin/end, forward search will start from selection end
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret;
    CharPos newEndPos;
    //no selection, caret at the second "thread" begin
    //should found the second "thread"
    clearFounds();
    mEdit->setCaretPos(CharPos{6,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));
    QCOMPARE(newEndPos,CharPos(1,76));

    //no selection, caret at the second "thread" end
    //should found the third "thread"
    clearFounds();
    mEdit->setCaretPos(CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,15)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(10,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the second "thread" selected, caret at selection begin
    //should found the third "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{6,12},CharPos{6,12},CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,15)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(10,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the second "thread" selected, caret at selection end
    //should found the third "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{12,12},CharPos{6,12},CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,15)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(10,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));
    QCOMPARE(newEndPos,CharPos(1,76));

    // the third "thread" selected, caret before the second "thread"
    // should found the second "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{4,12},CharPos{4,15},CharPos{4,15});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));
    QCOMPARE(newEndPos,CharPos(1,76));

    // the third "thread" selected, caret after the fifth "thread"
    // should found the sixth "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{16,19},CharPos{4,15},CharPos{4,15});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(5,48)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(11,48));
    QCOMPARE(mEdit->selBegin(), CharPos(5,48));
    QCOMPARE(mEdit->selEnd(), CharPos(11,48));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_basic_search_forward_from_caret_include_current_selection()
{
    //when caret is at selection begin/end, forward search will start from selection end
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoIncludeCurrentSelection;
    CharPos newEndPos;

    //no selection, caret at the second "thread" begin
    //should found the second "thread"
    clearFounds();
    mEdit->setCaretPos(CharPos{6,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));
    QCOMPARE(newEndPos,CharPos(1,76));

    //no selection, caret at the second "thread" end
    //should found the third "thread"
    clearFounds();
    mEdit->setCaretPos(CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,15)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(10,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the second "thread" selected, caret at selection begin
    //should found the second "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{6,12},CharPos{6,12},CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the second "thread" selected, caret at selection end
    //should found the second "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{12,12},CharPos{6,12},CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));
    QCOMPARE(newEndPos,CharPos(1,76));

    // the third "thread" selected, caret before the second "thread"
    // should found the second "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{4,12},CharPos{4,15},CharPos{4,15});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(12,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));
    QCOMPARE(newEndPos,CharPos(1,76));

    // the third "thread" selected, caret after the fifth "thread"
    // should found the sixth "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{16,19},CharPos{4,15},CharPos{4,15});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(5,48)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(11,48));
    QCOMPARE(mEdit->selBegin(), CharPos(5,48));
    QCOMPARE(mEdit->selEnd(), CharPos(11,48));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_basic_search_forward()
{
    //when caret is at selection begin/end, forward search will start from selection end
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoNone;
    CharPos newEndPos;
    //no selection, caret at the second "thread" begin
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretPos(CharPos{6,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));

    //no selection, caret at the second "thread" end
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretPos(CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the second "thread" selected, caret at selection begin
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{6,12},CharPos{6,12},CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the second "thread" selected, caret at selection end
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{12,12},CharPos{6,12},CharPos{12,12});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));

    // the third "thread" selected, caret before the second "thread"
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{4,12},CharPos{4,15},CharPos{4,15});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));

    // the third "thread" selected, caret after the fifth "thread"
    //should found the first "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos{16,19},CharPos{4,15},CharPos{4,15});
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_basic_search_forward_from_caret_with_wrap()
{
    //when caret is at selection begin/end, forward search will start from selection end
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoWrapAround;
    CharPos newEndPos;

    //caret before last "thread"
    mEdit->setCaretPos({10,67});

    //found the last "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(22,72)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
    QCOMPARE(newEndPos,CharPos(1,76));

    //found the first "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));

    //caret at file end, and don't confirm wrap
    mEdit->setCaretPos(mEdit->fileEnd());
    clearFounds();
    QCOMPARE(mEdit->searchReplace(
                 "thread","",
                 mEdit->fileBegin(),
                 mEdit->fileEnd(),
                 newEndPos,
                 options,
                 mBasicSearcher.get(),
                 mBasicMatchedAndExitProc,
                 [](){ return false; }),
             0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretPos(), mEdit->fileEnd());
    QCOMPARE(mEdit->selBegin(), mEdit->fileEnd());
    QCOMPARE(mEdit->selEnd(), mEdit->fileEnd());
    QCOMPARE(newEndPos,CharPos(1,76));

    //caret at file end, and confirm wrap
    mEdit->setCaretPos(mEdit->fileEnd());
    clearFounds();
    QCOMPARE(mEdit->searchReplace(
                 "thread","",
                 mEdit->fileBegin(),
                 mEdit->fileEnd(),
                 newEndPos,
                 options,
                 mBasicSearcher.get(),
                 mBasicMatchedAndExitProc,
                 [](){ return true; }),
             1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_search_forward_from_caret_whole_word()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoWholeWord;
    CharPos newEndPos;

    clearFounds();
    QCOMPARE(mEdit->searchReplace("Thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("Thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(27,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    //not found
    clearFounds();
    QCOMPARE(mEdit->searchReplace("Thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretPos(), CharPos(27,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_search_forward_whole_word2()
{
    QStringList text = {
        "uint uint,uint,uint uint",
        "int int,int,int int",
        "uintx uintx,uintx,uintx uintx",
        "uintx uintx,int,uintx uintx",
        "intx intx,intx,intx intx",
    };

    SearchOptions options = SearchOption::ssoWholeWord;
    CharPos newEndPos;

    mEdit->loadContent(text);
    clearFounds();
    QCOMPARE(mEdit->searchReplace("int","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),6);
    QCOMPARE(newEndPos,CharPos(24,4));

    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "int",
                     "int",
                     "int",
                     "int",
                     "int",
                     "int",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(0,1),
                     CharPos(4,1),
                     CharPos(8,1),
                     CharPos(12,1),
                     CharPos(16,1),
                     CharPos(12,3),
             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     3,
                     3,
                     3,
                     3,
                     3,
                     3,
                 }));
}

void TestQSyneditCppSearchReplace::test_search_forward_from_caret_match_case()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoMatchCase ;
    CharPos newEndPos;

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(16,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(12,18)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(18,18));
    QCOMPARE(mEdit->selBegin(), CharPos(12,18));
    QCOMPARE(mEdit->selEnd(), CharPos(18,18));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,52)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(27,52));
    QCOMPARE(mEdit->selBegin(), CharPos(21,52));
    QCOMPARE(mEdit->selEnd(), CharPos(27,52));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(27,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(35,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(22,72)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
    QCOMPARE(newEndPos,CharPos(1,76));

    //not found
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretPos(), CharPos(28,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_search_all_forward_in_whole_file_from_start()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoNone;
    CharPos newEndPos;

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
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
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_search_all_forward_in_whole_file_from_caret_and_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoWrapAround;
    CharPos newEndPos;
    //caret at begin of the 3rd "thread"
    mEdit->setCaretPos({4,15});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
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
    QCOMPARE(newEndPos,CharPos(1,76));

    //caret at end of the third "thread"
    mEdit->setCaretPos({10,15});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),10);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
                     "Thread",
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
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
                     CharPos(4,67),
                     CharPos(22,72),
                     CharPos(10,3),
                     CharPos(6,12),
                     CharPos(4,15),
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
    QCOMPARE(newEndPos,CharPos(1,76));

    //caret in the middle of the third "thread"
    mEdit->setCaretPos({5,15});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),9);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
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
                 }));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_search_all_forward_in_whole_file_from_caret_and_no_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret;
    CharPos newEndPos;

    //before the 3rd "thread"
    mEdit->setCaretPos({3,15});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
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
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_search_forward_in_scope()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoNone;
    CharPos newEndPos;
    // Caret position not used in scoped search

    //search from begin of the 2nd "thread" to begin of the 9th "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(6,12),
                                  CharPos(4,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),7);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "Thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(6,12),
                     CharPos(4,15),
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
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
                 }));
    QCOMPARE(newEndPos,CharPos(4,67));

    //search from begin of the 2nd "thread" to middle of the 9th "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(6,12),
                                  CharPos(5,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),7);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "Thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(6,12),
                     CharPos(4,15),
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
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
                 }));
    QCOMPARE(newEndPos,CharPos(5,67));

    //search from begin of the 2nd "thread" to end of the 9th "thread"
    //caret not in it
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(6,12),
                                  CharPos(10,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),8);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "Thread",
                     "thread",
                     "Thread",
                     "thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(6,12),
                     CharPos(4,15),
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
                     CharPos(4,67),
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
    QCOMPARE(newEndPos,CharPos(10,67));

    //search from middle of the 2nd "thread" to begin of the 9th "thread"
    //caret not in it
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(7,12),
                                  CharPos(4,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),6);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(4,15),
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                 }));
    QCOMPARE(newEndPos,CharPos(4,67));

    //search from middle of the 2nd "thread" to middle of the 9th "thread"
    //caret not in it
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(7,12),
                                  CharPos(5,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),6);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(4,15),
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                 }));
    QCOMPARE(newEndPos,CharPos(5,67));

    //search from middle of the 2nd "thread" to end of the 9th "thread"
    //caret not in it
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(7,12),
                                  CharPos(10,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),7);
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
                 }));
    QCOMPARE(newEndPos,CharPos(10,67));

    //search from end of the 2nd "thread" to begin of the 9th "thread"
    //caret not in it
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(12,12),
                                  CharPos(4,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),6);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(4,15),
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                 }));
    QCOMPARE(newEndPos,CharPos(4,67));

    //search from end of the 2nd "thread" to middle of the 9th "thread"
    //caret not in it
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(12,12),
                                  CharPos(5,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),6);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(4,15),
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                 }));
    QCOMPARE(newEndPos,CharPos(5,67));

    //search from end of the 2nd "thread" to end of the 9th "thread"
    //caret not in it
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(12,12),
                                  CharPos(10,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),7);
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
                 }));
    QCOMPARE(newEndPos,CharPos(10,67));

}

void TestQSyneditCppSearchReplace::test_ensure_caret_pos_not_used_in_scope_search()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret;
    CharPos newEndPos;
    // Caret position not used in scoped search

    //search from begin of the 2nd "thread" to begin of the 9th "thread"
    //caret in the middle of the 6th "thread"
    mEdit->setCaretPos({22,52});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(6,12),
                                  CharPos(4,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),7);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "Thread",
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(6,12),
                     CharPos(4,15),
                     CharPos(12,18),
                     CharPos(5,48),
                     CharPos(21,52),
                     CharPos(21,62),
                     CharPos(29,62),
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
                 }));
    QCOMPARE(newEndPos,CharPos(4,67));
}


void TestQSyneditCppSearchReplace::test_basic_search_backward_in_empty_doc()
{
    mEdit->clear();

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoBackwards;
    CharPos newEndPos;

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),0);
    QCOMPARE(mEdit->caretPos(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(newEndPos,CharPos(0,0));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretPos(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(newEndPos,CharPos(0,0));
}

void TestQSyneditCppSearchReplace::test_basic_search_backward_from_caret_without_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoBackwards;
    CharPos newEndPos;

    //caret at file end
    mEdit->setCaretPos(mEdit->fileEnd());

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(22,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(29,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(21,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(21,52));
    QCOMPARE(mEdit->selBegin(), CharPos(21,52));
    QCOMPARE(mEdit->selEnd(), CharPos(27,52));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(5,48));
    QCOMPARE(mEdit->selBegin(), CharPos(5,48));
    QCOMPARE(mEdit->selEnd(), CharPos(11,48));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(12,18));
    QCOMPARE(mEdit->selBegin(), CharPos(12,18));
    QCOMPARE(mEdit->selEnd(), CharPos(18,18));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(4,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(6,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(10,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));

    //not found
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),0);
    QCOMPARE(mEdit->caretPos(), CharPos(10,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_basic_search_backward_from_caret_without_wrap_with_callback()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoBackwards;
    CharPos newEndPos;

    //caret at file end
    mEdit->setCaretPos(mEdit->fileEnd());

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(22,72)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(22,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(29,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(21,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,52)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(21,52));
    QCOMPARE(mEdit->selBegin(), CharPos(21,52));
    QCOMPARE(mEdit->selEnd(), CharPos(27,52));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(5,48)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(5,48));
    QCOMPARE(mEdit->selBegin(), CharPos(5,48));
    QCOMPARE(mEdit->selEnd(), CharPos(11,48));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(12,18)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(12,18));
    QCOMPARE(mEdit->selBegin(), CharPos(12,18));
    QCOMPARE(mEdit->selEnd(), CharPos(18,18));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,15)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(4,15));
    QCOMPARE(mEdit->selBegin(), CharPos(4,15));
    QCOMPARE(mEdit->selEnd(), CharPos(10,15));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(6,12)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(6,12));
    QCOMPARE(mEdit->selBegin(), CharPos(6,12));
    QCOMPARE(mEdit->selEnd(), CharPos(12,12));
    QCOMPARE(newEndPos,CharPos(1,76));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(10,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_basic_search_backward_from_caret_with_selection()
{
    //when caret is at selection begin/end, backward search will start from selection begin
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoBackwards;
    CharPos newEndPos;

    //no selection, caret at the 9th "thread" end
    //should found the 9th "thread"
    clearFounds();
    mEdit->setCaretPos(CharPos(10,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));
    QCOMPARE(newEndPos,CharPos(1,76));

    //no selection, caret at the 9th "thread" begin
    //should found the 8th "thread"
    clearFounds();
    mEdit->setCaretPos(CharPos(4,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(29,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the 9th "thread" selected, caret at selection begin
    //should found the 8th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(4,67),CharPos(4,67),CharPos(10,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(29,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the 9th "thread" selected, caret at selection end
    //should found the 8th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(10,67),CharPos(4,67),CharPos(10,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(29,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the 8th "thread" selected, caret after the 9th "thread"
    //should found the 9th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(0,68),CharPos(29,62),CharPos(35,62));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the 8th "thread" selected, caret before the 7th "thread"
    //should found the 6th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(28,62),CharPos(29,62),CharPos(35,62));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(21,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_basic_search_backward_from_caret_include_current_selection()
{
    //when caret is at selection begin/end, backward search will start from selection begin
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoBackwards | SearchOption::ssoIncludeCurrentSelection;
    CharPos newEndPos;

    //no selection, caret at the 9th "thread" end
    //should found the 9th "thread"
    clearFounds();
    mEdit->setCaretPos(CharPos(10,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));
    QCOMPARE(newEndPos,CharPos(1,76));

    //no selection, caret at the 9th "thread" begin
    //should found the 8th "thread"
    clearFounds();
    mEdit->setCaretPos(CharPos(4,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(29,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(29,62));
    QCOMPARE(mEdit->selBegin(), CharPos(29,62));
    QCOMPARE(mEdit->selEnd(), CharPos(35,62));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the 9th "thread" selected, caret at selection begin
    //should found the 9th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(4,67),CharPos(4,67),CharPos(10,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the 9th "thread" selected, caret at selection end
    //should found the 9th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(10,67),CharPos(4,67),CharPos(10,67));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the 8th "thread" selected, caret after the 9th "thread"
    //should found the 9th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(0,68),CharPos(29,62),CharPos(35,62));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"Thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(4,67)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(4,67));
    QCOMPARE(mEdit->selBegin(), CharPos(4,67));
    QCOMPARE(mEdit->selEnd(), CharPos(10,67));
    QCOMPARE(newEndPos,CharPos(1,76));

    //the 8th "thread" selected, caret before the 7th "thread"
    //should found the 6th "thread"
    clearFounds();
    mEdit->setCaretAndSelection(CharPos(28,62),CharPos(29,62),CharPos(35,62));
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(21,62)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(21,62));
    QCOMPARE(mEdit->selBegin(), CharPos(21,62));
    QCOMPARE(mEdit->selEnd(), CharPos(27,62));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_basic_search_backward_from_caret_with_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret | SearchOption::ssoBackwards | SearchOption::ssoWrapAround;
    CharPos newEndPos;

    //found the first "thread"
    mEdit->setCaretPos({16,3});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(10,3)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(10,3));
    QCOMPARE(mEdit->selBegin(), CharPos(10,3));
    QCOMPARE(mEdit->selEnd(), CharPos(16,3));
    QCOMPARE(newEndPos,CharPos(1,76));

    //found the last "thread" (wrap around)
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(22,72)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(22,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
    QCOMPARE(newEndPos,CharPos(1,76));

    //at file begin, don't confirm
    mEdit->setCaretPos({0,0});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         [](){ return false; }),
             0);
    QCOMPARE(mFoundStrings,{});
    QCOMPARE(mReplaces,{});
    QCOMPARE(mFoundPositions,{});
    QCOMPARE(mFoundLens,{});
    QCOMPARE(mEdit->caretPos(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(newEndPos,CharPos(1,76));

    // at file begin, confirm
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndExitProc,
                         [](){ return true; }),
             1);
    QCOMPARE(mFoundStrings,{"thread"});
    QCOMPARE(mReplaces,{""});
    QCOMPARE(mFoundPositions,{CharPos(22,72)});
    QCOMPARE(mFoundLens,{6});
    QCOMPARE(mEdit->caretPos(), CharPos(22,72));
    QCOMPARE(mEdit->selBegin(), CharPos(22,72));
    QCOMPARE(mEdit->selEnd(), CharPos(28,72));
    QCOMPARE(newEndPos,CharPos(1,76));

}

void TestQSyneditCppSearchReplace::test_search_all_backward_in_whole_file_from_end()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoBackwards;
    CharPos newEndPos;

    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),10);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                     "Thread",
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
                     CharPos(22,72),
                     CharPos(4,67),
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
                     CharPos(6,12),
                     CharPos(10,3),
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
    QCOMPARE(newEndPos,CharPos(1,76));

}

void TestQSyneditCppSearchReplace::test_search_all_backward_in_whole_file_from_caret_and_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoBackwards | SearchOption::ssoWrapAround | SearchOption::ssoFromCaret;
    CharPos newEndPos;

    //caret at begin of the 8th "thread"
    mEdit->setCaretPos({29,62});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),10);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                     "Thread",
                     "Thread",
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
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
                     CharPos(6,12),
                     CharPos(10,3),
                     CharPos(22,72),
                     CharPos(4,67),
                     CharPos(29,62),
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
    QCOMPARE(newEndPos,CharPos(1,76));

    //caret at end of the 8th "thread"
    mEdit->setCaretPos({35,62});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),10);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                     "Thread",
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
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
                     CharPos(6,12),
                     CharPos(10,3),
                     CharPos(22,72),
                     CharPos(4,67),
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
    QCOMPARE(newEndPos,CharPos(1,76));

    //caret in the middle of the 8th "thread"
    mEdit->setCaretPos({30,62});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),9);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                     "Thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
                     CharPos(6,12),
                     CharPos(10,3),
                     CharPos(22,72),
                     CharPos(4,67),
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
                 }));
    QCOMPARE(newEndPos,CharPos(1,76));

}

void TestQSyneditCppSearchReplace::test_search_all_backward_in_whole_file_from_caret_and_no_wrap()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoBackwards | SearchOption::ssoFromCaret;
    CharPos newEndPos;

    //before the 8th "thread"
    mEdit->setCaretPos({28,62});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),7);
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                     "Thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
                     CharPos(6,12),
                     CharPos(10,3),
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
                 }));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_search_backward_in_scope()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoBackwards;
    CharPos newEndPos;

    //search from begin of the 9th "thread" to begin of the 2th "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(6,12),
                                  CharPos(4,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),7);
    QCOMPARE(newEndPos,CharPos(4,67));
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                     "Thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
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
                 }));

    //search from begin of the 9th "thread" to middle of the 2th "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(7,12),
                                  CharPos(4,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),6);
    QCOMPARE(newEndPos,CharPos(4,67));
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                 }));

    //search from begin of the 9th "thread" to end of the 2th "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(12,12),
                                  CharPos(4,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),6);
    QCOMPARE(newEndPos,CharPos(4,67));
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                 }));

    //search from middle of the 9th "thread" to begin of the 2th "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(6,12),
                                  CharPos(5,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),7);
    QCOMPARE(newEndPos,CharPos(5,67));
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                     "Thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
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
                 }));

    //search from middle of the 9th "thread" to middle of the 2th "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(7,12),
                                  CharPos(5,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),6);
    QCOMPARE(newEndPos,CharPos(5,67));
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                 }));

    //search from middle of the 9th "thread" to end of the 2th "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(12,12),
                                  CharPos(5,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),6);
    QCOMPARE(newEndPos,CharPos(5,67));
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
             }));
    QCOMPARE(mFoundLens,QList<int>(
                 {
                     6,
                     6,
                     6,
                     6,
                     6,
                     6,
                 }));

    //search from end of the 9th "thread" to begin of the 2th "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(6,12),
                                  CharPos(10,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),8);
    QCOMPARE(newEndPos,CharPos(10,67));
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                     "Thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(4,67),
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
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
                 }));

    //search from end of the 9th "thread" to middle of the 2th "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(7,12),
                                  CharPos(10,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),7);
    QCOMPARE(newEndPos,CharPos(10,67));
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(4,67),
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
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
                 }));

    //search from end of the 9th "thread" to end of the 2th "thread"
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  CharPos(12,12),
                                  CharPos(10,67),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),7);
    QCOMPARE(newEndPos,CharPos(10,67));
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "Thread",
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
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
                 }));
    QCOMPARE(mFoundPositions,QList<CharPos>(
                 {
                     CharPos(4,67),
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
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
                 }));

    //template
    clearFounds();
    QCOMPARE(mEdit->searchReplace("thread","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mBasicMatchedAndContinueProc,
                         nullptr),10);
    QCOMPARE(newEndPos,CharPos(1,76));
    QCOMPARE(mFoundStrings,
             QStringList(
                 {
                     "thread",
                     "Thread",
                     "thread",
                     "thread",
                     "thread",
                     "Thread",
                     "thread",
                     "Thread",
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
                     CharPos(22,72),
                     CharPos(4,67),
                     CharPos(29,62),
                     CharPos(21,62),
                     CharPos(21,52),
                     CharPos(5,48),
                     CharPos(12,18),
                     CharPos(4,15),
                     CharPos(6,12),
                     CharPos(10,3),
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

void TestQSyneditCppSearchReplace::test_search_regex()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoNone;
    CharPos newEndPos;

    clearFounds();
    QCOMPARE(mEdit->searchReplace("\\w+","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mRegexSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(newEndPos,CharPos(1,76));
    QCOMPARE(mFoundStrings,{"include"});
    QCOMPARE(mReplaces, {""});
    QCOMPARE(mFoundPositions,{CharPos(1,0)});
    QCOMPARE(mFoundLens,{7});
    QCOMPARE(mEdit->caretPos(), CharPos(8,0));
    QCOMPARE(mEdit->selBegin(), CharPos(1,0));
    QCOMPARE(mEdit->selEnd(), CharPos(8,0));

    clearFounds();
    QCOMPARE(mEdit->searchReplace("\\d+","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mRegexSearcher.get(),
                         mBasicMatchedAndExitProc,
                         nullptr),1);
    QCOMPARE(newEndPos,CharPos(1,76));
    QCOMPARE(mFoundStrings,{"123"});
    QCOMPARE(mReplaces, {""});
    QCOMPARE(mFoundPositions,{CharPos(13,10)});
    QCOMPARE(mFoundLens,{3});
    QCOMPARE(mEdit->caretPos(), CharPos(16,10));
    QCOMPARE(mEdit->selBegin(), CharPos(13,10));
    QCOMPARE(mEdit->selEnd(), CharPos(16,10));
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
//    QCOMPARE(mEdit->caretPos(), CharPos(33,15));
//    QCOMPARE(mEdit->selBegin(), CharPos(29,15));
//    QCOMPARE(mEdit->selEnd(), CharPos(33,15));

    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    SearchOptions options = SearchOption::ssoFromCaret;
    CharPos newEndPos;

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(33,15));
    QCOMPARE(mEdit->selBegin(), CharPos(29,15));
    QCOMPARE(mEdit->selEnd(), CharPos(33,15));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(65,23));
    QCOMPARE(mEdit->selBegin(), CharPos(61,23));
    QCOMPARE(mEdit->selEnd(), CharPos(65,23));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(52,26));
    QCOMPARE(mEdit->selBegin(), CharPos(48,26));
    QCOMPARE(mEdit->selEnd(), CharPos(52,26));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(12,49));
    QCOMPARE(mEdit->selBegin(), CharPos(8,49));
    QCOMPARE(mEdit->selEnd(), CharPos(12,49));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),1);
    QCOMPARE(mEdit->caretPos(), CharPos(26,59));
    QCOMPARE(mEdit->selBegin(), CharPos(22,59));
    QCOMPARE(mEdit->selEnd(), CharPos(26,59));
    QCOMPARE(newEndPos,CharPos(1,76));

    QCOMPARE(mEdit->searchReplace("stop","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                                  newEndPos,
                         options,
                         mBasicSearcher.get(),
                         nullptr,
                         nullptr),0);
    QCOMPARE(mEdit->caretPos(), CharPos(26,59));
    QCOMPARE(mEdit->selBegin(), CharPos(22,59));
    QCOMPARE(mEdit->selEnd(), CharPos(26,59));
    QCOMPARE(newEndPos,CharPos(1,76));
}

void TestQSyneditCppSearchReplace::test_replace_forward_from_caret()
{
//    QStringList text1{
//        "int iNTel() {",
//        " ttt;",
//        "}",
//        "int main() {",
//        " UNIT xint0;",
//        " if (xint0>0) {",
//        "  intel(xint0++);",
//        " }",
//        "}",
//        "int test() {",
//        " return 0;",
//        "}"
//    };

    QStringList text1{
        "int int int int",
        "int int int int",
        "int int int int",
    };
    QStringList text2{
        "   ",
        "  int ",
        "   ",
    };

    QStringList text3{
        "   ",
        "  int ",
        "   ",
    };
    QStringList text4{
        "beautiful beautiful beautiful beautiful",
        "beautiful beautiful int beautiful",
        "beautiful beautiful beautiful beautiful",
    };

    SearchOptions options = ssoWrapAround | ssoFromCaret;
    CharPos newEndPos;
    auto matchProc = [this](const QString& sFound,
            const QString& sReplace, const CharPos& pos, int wordLen){
        mFoundStrings.append(sFound);
        mReplaces.append(sReplace);
        mFoundPositions.append(pos);
        mFoundLens.append(wordLen);
        if (pos == CharPos{8,1})
            return SearchAction::Skip;
        return SearchAction::Replace;
    };

    mEdit->loadContent(text1);
    mEdit->setCaretPos({8,1});
    mEdit->searchReplace("int","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         matchProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(newEndPos,CharPos(3,2));
    QCOMPARE(mEdit->caretPos(), CharPos(1,1));
    QCOMPARE(mEdit->selBegin(), CharPos(1,1));
    QCOMPARE(mEdit->selEnd(), CharPos(1,1));

    mEdit->loadContent(text1);
    mEdit->setCaretPos({9,1});
    mEdit->searchReplace("int","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         matchProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(newEndPos,CharPos(3,2));
    QCOMPARE(mEdit->caretPos(), CharPos(1,1));
    QCOMPARE(mEdit->selBegin(), CharPos(1,1));
    QCOMPARE(mEdit->selEnd(), CharPos(1,1));

    mEdit->loadContent(text1);
    mEdit->setCaretPos({8,1});
    mEdit->searchReplace("int","beautiful",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         matchProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(newEndPos,CharPos(39,2));
    QCOMPARE(mEdit->caretPos(), CharPos(19,1));
    QCOMPARE(mEdit->selBegin(), CharPos(19,1));
    QCOMPARE(mEdit->selEnd(), CharPos(19,1));
}

void TestQSyneditCppSearchReplace::test_replace_forward_from_caret_with_selection()
{
    QStringList text1{
        "int int int int",
        "int int int int",
        "int int int int",
    };
    QStringList text2{
        "int int int int",
        "int int int int",
        "int replaced int int",
    };
    QStringList text3{
        "int int int int",
        "int int int int",
        "int int  int",
    };
    QStringList text4{
        "int int int int",
        "int int int int",
        "int int replaced1 int",
    };
    QStringList text5{
        "int int int int",
        "int int int int",
        "int int replaced2 int",
    };
    SearchOptions options = ssoFromCaret;
    CharPos newEndPos;

    //no selection,caret at begin of the second "int" on 3rd line
    mEdit->loadContent(text1);
    mEdit->setCaretPos({4,2});
    mEdit->searchReplace("int","replaced",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(newEndPos,CharPos(20,2));
    QCOMPARE(mEdit->caretPos(), CharPos(16,2));
    QCOMPARE(mEdit->selBegin(), CharPos(13,2));
    QCOMPARE(mEdit->selEnd(), CharPos(16,2));

    //no selection,caret at end of the second "int" on 3rd line
    mEdit->loadContent(text1);
    mEdit->setCaretPos({7,2});
    mEdit->searchReplace("int","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(newEndPos,CharPos(12,2));
    QCOMPARE(mEdit->caretPos(), CharPos(12,2));
    QCOMPARE(mEdit->selBegin(), CharPos(9,2));
    QCOMPARE(mEdit->selEnd(), CharPos(12,2));

    //the second "int" on 3rd line selected, caret at selection begin
    mEdit->loadContent(text1);
    mEdit->setCaretAndSelection(CharPos{4,2},CharPos{4,2},CharPos{7,2});
    mEdit->searchReplace("int","replaced1",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(newEndPos,CharPos(21,2));
    QCOMPARE(mEdit->caretPos(), CharPos(21,2));
    QCOMPARE(mEdit->selBegin(), CharPos(18,2));
    QCOMPARE(mEdit->selEnd(), CharPos(21,2));

    //the second "int" on 3rd line selected, caret at selection end
    mEdit->loadContent(text1);
    mEdit->setCaretAndSelection(CharPos{7,2},CharPos{4,2},CharPos{7,2});
    mEdit->searchReplace("int","replaced2",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text5);
    QCOMPARE(newEndPos,CharPos(21,2));
    QCOMPARE(mEdit->caretPos(), CharPos(21,2));
    QCOMPARE(mEdit->selBegin(), CharPos(18,2));
    QCOMPARE(mEdit->selEnd(), CharPos(21,2));
}

void TestQSyneditCppSearchReplace::test_replace_forward_from_caret_include_selection()
{
    QStringList text1{
        "int int int int",
        "int int int int",
        "int int int int",
    };
    QStringList text2{
        "int int int int",
        "int int int int",
        "int replaced int int",
    };
    QStringList text3{
        "int int int int",
        "int int int int",
        "int int  int",
    };
    QStringList text4{
        "int int int int",
        "int int int int",
        "int replaced1 int int",
    };
    QStringList text5{
        "int int int int",
        "int int int int",
        "int replaced2 int int",
    };
    QStringList text6{
        "int int int int",
        "int int int replaced2",
        "int int int int",
    };
    SearchOptions options = ssoFromCaret | ssoIncludeCurrentSelection;
    CharPos newEndPos;

    //no selection,caret at begin of the second "int" on 3rd line
    mEdit->loadContent(text1);
    mEdit->setCaretPos({4,2});
    mEdit->searchReplace("int","replaced",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(newEndPos,CharPos(20,2));
    QCOMPARE(mEdit->caretPos(), CharPos(16,2));
    QCOMPARE(mEdit->selBegin(), CharPos(13,2));
    QCOMPARE(mEdit->selEnd(), CharPos(16,2));


    //no selection,caret at end of the second "int" on 3rd line
    mEdit->loadContent(text1);
    mEdit->setCaretPos({7,2});
    mEdit->searchReplace("int","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(newEndPos,CharPos(12,2));
    QCOMPARE(mEdit->caretPos(), CharPos(12,2));
    QCOMPARE(mEdit->selBegin(), CharPos(9,2));
    QCOMPARE(mEdit->selEnd(), CharPos(12,2));


    //the second "int" on 3rd line selected, caret at selection begin
    mEdit->loadContent(text1);
    mEdit->setCaretAndSelection(CharPos{4,2},CharPos{4,2},CharPos{7,2});
    mEdit->searchReplace("int","replaced1",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(newEndPos,CharPos(21,2));
    QCOMPARE(mEdit->caretPos(), CharPos(17,2));
    QCOMPARE(mEdit->selBegin(), CharPos(14,2));
    QCOMPARE(mEdit->selEnd(), CharPos(17,2));

    //the second "int" on 3rd line selected, caret at selection end
    mEdit->loadContent(text1);
    mEdit->setCaretAndSelection(CharPos{7,2},CharPos{4,2},CharPos{7,2});
    mEdit->searchReplace("int","replaced2",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text5);
    QCOMPARE(newEndPos,CharPos(21,2));
    QCOMPARE(mEdit->caretPos(), CharPos(17,2));
    QCOMPARE(mEdit->selBegin(), CharPos(14,2));
    QCOMPARE(mEdit->selEnd(), CharPos(17,2));

    //the forth "int" on 2rd line selected, caret at selection begin
    mEdit->loadContent(text1);
    mEdit->setCaretAndSelection(CharPos{12,1},CharPos{12,1},CharPos{15,1});
    mEdit->searchReplace("int","replaced2",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text6);
    QCOMPARE(newEndPos,CharPos(15,2));
    QCOMPARE(mEdit->caretPos(), CharPos(3,2));
    QCOMPARE(mEdit->selBegin(), CharPos(0,2));
    QCOMPARE(mEdit->selEnd(), CharPos(3,2));
}

void TestQSyneditCppSearchReplace::test_replace_backward_from_caret()
{
    QStringList text1{
        "int int int int",
        "int int int int",
        "int int int int",
    };
    QStringList text2{
        "   ",
        " int  ",
        "   ",
    };

    QStringList text3{
        "   ",
        " int  ",
        "   ",
    };
    QStringList text4{
        "beautiful beautiful beautiful beautiful",
        "beautiful int beautiful beautiful",
        "beautiful beautiful beautiful beautiful",
    };

    SearchOptions options = ssoWrapAround | ssoFromCaret | ssoBackwards;
    CharPos newEndPos;

    auto matchProc = [this](const QString& sFound,
            const QString& sReplace, const CharPos& pos, int wordLen){
        mFoundStrings.append(sFound);
        mReplaces.append(sReplace);
        mFoundPositions.append(pos);
        mFoundLens.append(wordLen);
        if (pos == CharPos{4,1})
            return SearchAction::Skip;
        return SearchAction::Replace;
    };

    mEdit->loadContent(text1);
    mEdit->setCaretPos({8,1});
    mEdit->searchReplace("int","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         matchProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(newEndPos,CharPos(3,2));
    QCOMPARE(mEdit->caretPos(), CharPos(5,1));
    QCOMPARE(mEdit->selBegin(), CharPos(5,1));
    QCOMPARE(mEdit->selEnd(), CharPos(5,1));

    mEdit->loadContent(text1);
    mEdit->setCaretPos({12,1});
    mEdit->searchReplace("int","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         matchProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(newEndPos,CharPos(3,2));
    QCOMPARE(mEdit->caretPos(), CharPos(6,1));
    QCOMPARE(mEdit->selBegin(), CharPos(6,1));
    QCOMPARE(mEdit->selEnd(), CharPos(6,1));

    mEdit->loadContent(text1);
    mEdit->setCaretPos({12,1});
    mEdit->searchReplace("int","beautiful",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         matchProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(newEndPos,CharPos(39,2));
    QCOMPARE(mEdit->caretPos(), CharPos(24,1));
    QCOMPARE(mEdit->selBegin(), CharPos(24,1));
    QCOMPARE(mEdit->selEnd(), CharPos(24,1));
}

void TestQSyneditCppSearchReplace::test_replace_backward_from_caret_with_selection()
{
    QStringList text1{
        "int int int int",
        "int int int int",
        "int int int int",
    };
    QStringList text2{
        "int int int int",
        "int int int int",
        "int replaced int int",
    };
    QStringList text3{
        "int int int int",
        "int int int int",
        "int int  int",
    };
    QStringList text4{
        "int int int int",
        "int int int int",
        "int replaced1 int int",
    };
    QStringList text5{
        "int int int int",
        "int int int int",
        "int replaced2 int int",
    };
    SearchOptions options = ssoFromCaret | ssoBackwards;
    CharPos newEndPos;

    //no selection,caret at begin of the third "int" on 3rd line
    mEdit->loadContent(text1);
    mEdit->setCaretPos({8,2});
    mEdit->searchReplace("int","replaced",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(newEndPos,CharPos(20,2));
    QCOMPARE(mEdit->caretPos(), CharPos(0,2));
    QCOMPARE(mEdit->selBegin(), CharPos(0,2));
    QCOMPARE(mEdit->selEnd(), CharPos(3,2));

    //no selection,caret at end of the third "int" on 3rd line
    mEdit->loadContent(text1);
    mEdit->setCaretPos({11,2});
    mEdit->searchReplace("int","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(newEndPos,CharPos(12,2));
    QCOMPARE(mEdit->caretPos(), CharPos(4,2));
    QCOMPARE(mEdit->selBegin(), CharPos(4,2));
    QCOMPARE(mEdit->selEnd(), CharPos(7,2));

    //the thrid "int" on 3rd line selected, caret at selection begin
    mEdit->loadContent(text1);
    mEdit->setCaretAndSelection(CharPos{8,2},CharPos{8,2},CharPos{11,2});
    mEdit->searchReplace("int","replaced1",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(newEndPos,CharPos(21,2));
    QCOMPARE(mEdit->caretPos(), CharPos(0,2));
    QCOMPARE(mEdit->selBegin(), CharPos(0,2));
    QCOMPARE(mEdit->selEnd(), CharPos(3,2));

    //the third "int" on 3rd line selected, caret at selection end
    mEdit->loadContent(text1);
    mEdit->setCaretAndSelection(CharPos{11,2},CharPos{8,2},CharPos{11,2});
    mEdit->searchReplace("int","replaced2",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text5);
    QCOMPARE(newEndPos,CharPos(21,2));
    QCOMPARE(mEdit->caretPos(), CharPos(0,2));
    QCOMPARE(mEdit->selBegin(), CharPos(0,2));
    QCOMPARE(mEdit->selEnd(), CharPos(3,2));
}

void TestQSyneditCppSearchReplace::test_replace_backward_from_caret_include_selection()
{
    QStringList text1{
        "int int int int",
        "int int int int",
        "int int int int",
    };
    QStringList text2{
        "int int int int",
        "int int int int",
        "int replaced int int",
    };
    QStringList text3{
        "int int int int",
        "int int int int",
        "int int  int",
    };
    QStringList text4{
        "int int int int",
        "int int int int",
        "int int replaced1 int",
    };
    QStringList text5{
        "int int int int",
        "int int int int",
        "int int replaced2 int",
    };
    QStringList text6{
        "int int int int",
        "int int int int",
        "replaced2 int int int",
    };
    QStringList text7{
        "int int int int",
        "int int int replaced2",
        "int int int int",
    };
    SearchOptions options = ssoFromCaret | ssoBackwards | ssoIncludeCurrentSelection;
    CharPos newEndPos;

    //no selection,caret at begin of the third "int" on 3rd line
    mEdit->loadContent(text1);
    mEdit->setCaretPos({8,2});
    mEdit->searchReplace("int","replaced",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(newEndPos,CharPos(20,2));
    QCOMPARE(mEdit->caretPos(), CharPos(0,2));
    QCOMPARE(mEdit->selBegin(), CharPos(0,2));
    QCOMPARE(mEdit->selEnd(), CharPos(3,2));

    //no selection,caret at end of the third "int" on 3rd line
    mEdit->loadContent(text1);
    mEdit->setCaretPos({11,2});
    mEdit->searchReplace("int","",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(newEndPos,CharPos(12,2));
    QCOMPARE(mEdit->caretPos(), CharPos(4,2));
    QCOMPARE(mEdit->selBegin(), CharPos(4,2));
    QCOMPARE(mEdit->selEnd(), CharPos(7,2));

    //the thrid "int" on 3rd line selected, caret at selection begin
    mEdit->loadContent(text1);
    mEdit->setCaretAndSelection(CharPos{8,2},CharPos{8,2},CharPos{11,2});
    mEdit->searchReplace("int","replaced1",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(newEndPos,CharPos(21,2));
    QCOMPARE(mEdit->caretPos(), CharPos(4,2));
    QCOMPARE(mEdit->selBegin(), CharPos(4,2));
    QCOMPARE(mEdit->selEnd(), CharPos(7,2));

    //the third "int" on 3rd line selected, caret at selection end
    mEdit->loadContent(text1);
    mEdit->setCaretAndSelection(CharPos{11,2},CharPos{8,2},CharPos{11,2});
    mEdit->searchReplace("int","replaced2",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text5);
    QCOMPARE(newEndPos,CharPos(21,2));
    QCOMPARE(mEdit->caretPos(), CharPos(4,2));
    QCOMPARE(mEdit->selBegin(), CharPos(4,2));
    QCOMPARE(mEdit->selEnd(), CharPos(7,2));

    //the first "int" on 3rd line selected, caret at selection end
    mEdit->loadContent(text1);
    mEdit->setCaretAndSelection(CharPos{3,2},CharPos{0,2},CharPos{3,2});
    mEdit->searchReplace("int","replaced2",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text6);
    QCOMPARE(newEndPos,CharPos(21,2));
    QCOMPARE(mEdit->caretPos(), CharPos(12,1));
    QCOMPARE(mEdit->selBegin(), CharPos(12,1));
    QCOMPARE(mEdit->selEnd(), CharPos(15,1));

    //the forth "int" on 2rd line selected, caret at selection end
    mEdit->loadContent(text1);
    mEdit->setCaretAndSelection(CharPos{15,1},CharPos{12,1},CharPos{15,1});
    mEdit->searchReplace("int","replaced2",
                                  mEdit->fileBegin(),
                                  mEdit->fileEnd(),
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndExitProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text7);
    QCOMPARE(newEndPos,CharPos(15,2));
    QCOMPARE(mEdit->caretPos(), CharPos(8,1));
    QCOMPARE(mEdit->selBegin(), CharPos(8,1));
    QCOMPARE(mEdit->selEnd(), CharPos(11,1));
}

void TestQSyneditCppSearchReplace::test_replace_forward_scope()
{
    QStringList text1{
        "int int int int",
        "int int int int",
        "int int int int",
    };


    QStringList text2{
        "int   ",
        "   ",
        "  int int",
    };

    QStringList text3{
        "int replaced replaced replaced",
        "replaced replaced replaced replaced",
        "replaced replaced int int",
    };

    SearchOptions options = ssoNone;
    CharPos newEndPos;
    mEdit->loadContent(text1);
    mEdit->searchReplace("int","",
                         CharPos{4,0},
                         CharPos{8,2},
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndContinueProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(newEndPos,CharPos(2,2));

    mEdit->loadContent(text1);
    mEdit->searchReplace("int","replaced",
                         CharPos{4,0},
                         CharPos{8,2},
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndContinueProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(newEndPos,CharPos(18,2));
}

void TestQSyneditCppSearchReplace::test_replace_backward_scope()
{
    QStringList text1{
        "int int int int",
        "int int int int",
        "int int int int",
    };


    QStringList text2{
        "int   ",
        "   ",
        "  int int",
    };

    QStringList text3{
        "int replaced replaced replaced",
        "replaced replaced replaced replaced",
        "replaced replaced int int",
    };

    SearchOptions options = ssoBackwards;
    CharPos newEndPos;
    mEdit->loadContent(text1);
    mEdit->searchReplace("int","",
                         CharPos{4,0},
                         CharPos{8,2},
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndContinueProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(newEndPos,CharPos(2,2));

    mEdit->loadContent(text1);
    mEdit->searchReplace("int","replaced",
                         CharPos{4,0},
                         CharPos{8,2},
                         newEndPos,
                         options,
                         mBasicSearcher.get(),
                         mReplaceAndContinueProc,
                         nullptr);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(newEndPos,CharPos(18,2));
}

void TestQSyneditCppSearchReplace::test_replace_plain_variants_data()
{
    QTest::addColumn<QString>("replacement");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("same length") << "b" << QStringList{"b b", "bb"};
    QTest::newRow("shorter") << "" << QStringList{" ", ""};
    QTest::newRow("longer") << "long" << QStringList{"long long", "longlong"};
    QTest::newRow("contains pattern") << "aa" << QStringList{"aa aa", "aaaa"};
}

void TestQSyneditCppSearchReplace::test_replace_plain_variants()
{
    QFETCH(QString, replacement);
    QFETCH(QStringList, expected);

    mEdit->loadContent({"a a", "aa"});
    CharPos newScopeEnd;
    QCOMPARE(mEdit->searchReplace("a", replacement,
                                  mEdit->fileBegin(), mEdit->fileEnd(), newScopeEnd,
                                  ssoNone, mBasicSearcher.get(),
                                  mReplaceAndContinueProc, nullptr), 4);
    QCOMPARE(mEdit->content(), expected);
    QCOMPARE(newScopeEnd, mEdit->fileEnd());
}

void TestQSyneditCppSearchReplace::test_replace_regex_captures()
{
    mEdit->loadContent({"int value = 12;", "int count = 3;"});
    CharPos newScopeEnd;
    QCOMPARE(mEdit->searchReplace("(int) (\\w+) = (\\d+);", "\\2: \\3 (\\1);",
                                  mEdit->fileBegin(), mEdit->fileEnd(), newScopeEnd,
                                  ssoMatchCase, mRegexSearcher.get(),
                                  mReplaceAndContinueProc, nullptr), 2);
    QStringList expected{"value: 12 (int);", "count: 3 (int);"};
    QCOMPARE(mEdit->content(), expected);
    QCOMPARE(newScopeEnd, mEdit->fileEnd());
}

void TestQSyneditCppSearchReplace::test_replace_unicode()
{
    QString oldText = QString::fromUtf8("变量 变量");
    QString replacement = QString::fromUtf8("值😀");
    mEdit->loadContent({oldText, QString::fromUtf8("非变量 变量")});
    CharPos newScopeEnd;
    QCOMPARE(mEdit->searchReplace(QString::fromUtf8("变量"), replacement,
                                  mEdit->fileBegin(), mEdit->fileEnd(), newScopeEnd,
                                  ssoMatchCase, mBasicSearcher.get(),
                                  mReplaceAndContinueProc, nullptr), 4);
    QCOMPARE(mEdit->content(), QStringList({
                 replacement + " " + replacement,
                 QString::fromUtf8("非") + replacement + " " + replacement}));
    QCOMPARE(newScopeEnd, mEdit->fileEnd());
}

void TestQSyneditCppSearchReplace::test_replace_multiline()
{
    mEdit->loadContent(QStringList{"a a"});
    CharPos newScopeEnd;
    QCOMPARE(mEdit->searchReplace("a", "x\ny",
                                  mEdit->fileBegin(), mEdit->fileEnd(), newScopeEnd,
                                  ssoNone, mBasicSearcher.get(),
                                  mReplaceAndContinueProc, nullptr), 2);
    QCOMPARE(mEdit->content(), QStringList({"x", "y x", "y"}));
    QCOMPARE(newScopeEnd, mEdit->fileEnd());

    mEdit->loadContent(QStringList{"a a"});
    QCOMPARE(mEdit->searchReplace("a", "x\ny",
                                  mEdit->fileBegin(), mEdit->fileEnd(), newScopeEnd,
                                  ssoBackwards, mBasicSearcher.get(),
                                  mReplaceAndContinueProc, nullptr), 2);
    QCOMPARE(mEdit->content(), QStringList({"x", "y x", "y"}));
    QCOMPARE(newScopeEnd, mEdit->fileEnd());

    mEdit->loadContent({"pre a a post", "next"});
    QCOMPARE(mEdit->searchReplace("a", "x\ny",
                                  {4, 0}, {7, 0}, newScopeEnd,
                                  ssoNone, mBasicSearcher.get(),
                                  mReplaceAndContinueProc, nullptr), 2);
    QCOMPARE(mEdit->content(), QStringList({"pre x", "y x", "y post", "next"}));
    QCOMPARE(newScopeEnd, CharPos(1, 2));
}

void TestQSyneditCppSearchReplace::test_replace_zero_length_search_advances()
{
    CharPos newScopeEnd;
    mEdit->loadContent(QStringList{"ab"});
    clearFounds();
    QCOMPARE(mEdit->searchReplace("x*", "",
                                  mEdit->fileBegin(), mEdit->fileEnd(), newScopeEnd,
                                  ssoNone, mRegexSearcher.get(),
                                  mBasicMatchedAndContinueProc, nullptr), 3);
    QCOMPARE(mFoundPositions, QList<CharPos>({{0, 0}, {1, 0}, {2, 0}}));

    mEdit->setCaretPos(mEdit->fileEnd());
    clearFounds();
    QCOMPARE(mEdit->searchReplace("x*", "",
                                  mEdit->fileBegin(), mEdit->fileEnd(), newScopeEnd,
                                  ssoBackwards, mRegexSearcher.get(),
                                  mBasicMatchedAndContinueProc, nullptr), 3);
    QCOMPARE(mFoundPositions, QList<CharPos>({{2, 0}, {1, 0}, {0, 0}}));
}

void TestQSyneditCppSearchReplace::test_replace_undo_redo_single_group()
{
    mEdit->loadContent({"a a", "a"});
    mEdit->setCaretAndSelection({2, 0}, {0, 0}, {1, 0});
    CharPos newScopeEnd;
    auto replaceAll = [](const QString &, const QString &, const CharPos &, int) {
        return SearchAction::ReplaceAll;
    };
    QCOMPARE(mEdit->searchReplace("a", "aa",
                                  mEdit->fileBegin(), mEdit->fileEnd(), newScopeEnd,
                                  ssoNone, mBasicSearcher.get(),
                                  replaceAll, nullptr), 3);
    QStringList replaced = mEdit->content();
    CharPos replacedCaret = mEdit->caretPos();
    CharPos replacedSelBegin = mEdit->selBegin();
    CharPos replacedSelEnd = mEdit->selEnd();

    mEdit->undo();
    QCOMPARE(mEdit->content(), QStringList({"a a", "a"}));
    QCOMPARE(mEdit->caretPos(), CharPos(1, 0));
    QCOMPARE(mEdit->selBegin(), CharPos(0, 0));
    QCOMPARE(mEdit->selEnd(), CharPos(1, 0));
    QVERIFY(!mEdit->canUndo());

    mEdit->redo();
    QCOMPARE(mEdit->content(), replaced);
    QCOMPARE(mEdit->caretPos(), replacedCaret);
    QCOMPARE(mEdit->selBegin(), replacedSelBegin);
    QCOMPARE(mEdit->selEnd(), replacedSelEnd);
}

void TestQSyneditCppSearchReplace::test_replace_read_only()
{
    mEdit->loadContent(QStringList{"a a"});
    mEdit->setReadOnly(true);
    mEdit->setCaretPos({1, 0});
    CharPos newScopeEnd;
    QCOMPARE(mEdit->searchReplace("a", "b",
                                  mEdit->fileBegin(), mEdit->fileEnd(), newScopeEnd,
                                  ssoNone, mBasicSearcher.get(),
                                  mReplaceAndContinueProc, nullptr), 2);
    QCOMPARE(mEdit->content(), QStringList({"a a"}));
    QCOMPARE(newScopeEnd, CharPos(3, 0));
    mEdit->setReadOnly(false);
}

void TestQSyneditCppSearchReplace::test_replace_selective_then_replace_all()
{
    auto run = [this](SearchOptions options, const QStringList &expected) {
        mEdit->loadContent({"a a a", "a a"});
        int callbackCalls = 0;
        auto callback = [&callbackCalls](const QString &, const QString &,
                const CharPos &, int) {
            callbackCalls++;
            if (callbackCalls == 1)
                return SearchAction::Skip;
            if (callbackCalls == 2)
                return SearchAction::Replace;
            return SearchAction::ReplaceAll;
        };
        CharPos newScopeEnd;
        QCOMPARE(mEdit->searchReplace("a", "b",
                                      mEdit->fileBegin(), mEdit->fileEnd(), newScopeEnd,
                                      options, mBasicSearcher.get(), callback, nullptr), 5);
        QCOMPARE(callbackCalls, 3);
        QCOMPARE(mEdit->content(), expected);
        QCOMPARE(newScopeEnd, mEdit->fileEnd());
    };

    run(ssoNone, {"a b b", "b b"});
    run(ssoBackwards, {"b b b", "b a"});
}

void TestQSyneditCppSearchReplace::test_whole_word_unicode_boundaries()
{
    auto check = [this](const QStringList &lines, const QString &pattern,
            const QList<CharPos> &expected) {
        mEdit->loadContent(lines);
        clearFounds();
        CharPos newScopeEnd;
        QCOMPARE(mEdit->searchReplace(pattern, "",
                                      mEdit->fileBegin(), mEdit->fileEnd(), newScopeEnd,
                                      ssoWholeWord | ssoMatchCase, mBasicSearcher.get(),
                                      mBasicMatchedAndContinueProc, nullptr),
                 expected.size());
        QCOMPARE(mFoundPositions, expected);
    };

    check({"int intx xint _int int_ (int)"}, "int",
          {{0, 0}, {25, 0}});
    check({QString::fromUtf8("变量 非变量 变量1 变量_ （变量）")},
          QString::fromUtf8("变量"), {{0, 0}, {16, 0}});
    check({"e\u0301 e"}, "e", {{3, 0}});
    check({QString::fromUtf8("😀int😀")}, "int", {{2, 0}});
    check({QString::fromUtf8("𐐀int")}, "int", {});
    check({"// int", "\"int\"", "int"}, "int",
          {{3, 0}, {1, 1}, {0, 2}});
    check({"hello world xhello world hello world2"}, "hello world", {{0, 0}});
}

void TestQSyneditCppSearchReplace::test_document_search_session_wrap_and_stale_match()
{
    mEdit->loadContent(QStringList{"a b a"});
    DocumentSearchSession session(
                mEdit->document(), {"a", false, true, false},
                {mEdit->fileBegin(), mEdit->fileEnd()});
    auto match = session.findNext({5, 0}, SearchDirection::Forward, WrapMode::Once);
    QVERIFY(match);
    QCOMPARE(match->start, CharPos(0, 0));
    match = session.findNext({0, 0}, SearchDirection::Backward, WrapMode::Once);
    QVERIFY(match);
    QCOMPARE(match->start, CharPos(4, 0));

    match = session.findNext({0, 0}, SearchDirection::Forward);
    QVERIFY(match);
    mEdit->replaceLine(0, "changed");
    QVERIFY(!mEdit->replaceMatch(*match, "new"));
    QCOMPARE(mEdit->content(), QStringList({"changed"}));
}

void TestQSyneditCppSearchReplace::test_replace_all_session()
{
    mEdit->loadContent({"a a", "aa"});
    DocumentSearchSession session(
                mEdit->document(), {"a", false, true, false},
                {mEdit->fileBegin(), mEdit->fileEnd()});
    QCOMPARE(mEdit->replaceAll(session, "b"), 4);
    QCOMPARE(mEdit->content(), QStringList({"b b", "bb"}));
    mEdit->undo();
    QCOMPARE(mEdit->content(), QStringList({"a a", "aa"}));

    mEdit->loadContent(QStringList{"ab"});
    DocumentSearchSession zeroSession(
                mEdit->document(), {"x*", true, true, false},
                {mEdit->fileBegin(), mEdit->fileEnd()});
    QCOMPARE(mEdit->replaceAll(zeroSession, "X"), 3);
    QCOMPARE(mEdit->content(), QStringList({"XaXbX"}));

    mEdit->loadContent(QStringList{"a a"});
    DocumentSearchSession multilineSession(
                mEdit->document(), {"a", false, true, false},
                {mEdit->fileBegin(), mEdit->fileEnd()});
    QCOMPARE(mEdit->replaceAll(multilineSession, "x\ny"), 2);
    QCOMPARE(mEdit->content(), QStringList({"x", "y x", "y"}));
}

}
