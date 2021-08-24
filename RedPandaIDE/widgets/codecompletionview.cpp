#include "codecompletionview.h"
#include "../utils.h"

#include <QKeyEvent>
#include <QVBoxLayout>

CodeCompletionView::CodeCompletionView(QWidget *parent) :
    QWidget(parent)
{
    setWindowFlags(Qt::Popup);
    mListView = new CodeCompletionListView(this);
    setLayout(new QVBoxLayout());
    layout()->addWidget(mListView);
    layout()->setMargin(0);
}

CodeCompletionView::~CodeCompletionView()
{

}

void CodeCompletionView::setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback)
{
    mListView->setKeypressedCallback(newKeypressedCallback);
}

void CodeCompletionView::addChildren(PStatement scopeStatement, const QString &fileName, int line)
{
    if (scopeStatement && !isIncluded(scopeStatement->fileName)
      && !isIncluded(scopeStatement->definitionFileName))
        return;
    const StatementMap& children = mParser->statementList().childrenStatements(scopeStatement);
    if (children.isEmpty())
        return;

    if (!scopeStatement) { //Global scope
        for (PStatement childStatement: children) {
            if (childStatement->fileName.isEmpty()) {
                // hard defines
                addStatement(childStatement,fileName,-1);
            } else if (!( childStatement->kind == StatementKind::skConstructor
                          || childStatement->kind == StatementKind::skDestructor
                          || childStatement->kind == StatementKind::skBlock)
                       && (!mAddedStatements.contains(childStatement->command))
                       && (
                           isIncluded(childStatement->fileName)
                           || isIncluded(childStatement->definitionFileName)
                           )
                       ) {
                //we must check if the statement is included by the file
                addStatement(childStatement,fileName,line);
            }
        }
    } else {
        for (PStatement childStatement: children) {
            if (!( childStatement->kind == StatementKind::skConstructor
                                      || childStatement->kind == StatementKind::skDestructor
                                      || childStatement->kind == StatementKind::skBlock)
                                   && (!mAddedStatements.contains(childStatement->command)))
                addStatement(childStatement,fileName,line);
        }
    }
}

void CodeCompletionView::addStatement(PStatement statement, const QString &fileName, int line)
{
    if ((line!=-1)
            && (line < statement->line)
            && (fileName == statement->fileName))
        return;
    mAddedStatements.insert(statement->command);
    mFullCompletionStatementList.append(statement);
}

static bool defaultComparator(PStatement statement1,PStatement statement2) {
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeIn) {
        if (statement2->kind != StatementKind::skUserCodeIn)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeIn) {
        return false;
        // show keywords first
    } else if ((statement1->kind == StatementKind::skKeyword)
               && (statement2->kind != StatementKind::skKeyword)) {
        return true;
    } else if ((statement1->kind != StatementKind::skKeyword)
               && (statement2->kind == StatementKind::skKeyword)) {
        return false;
    } else
        return statement1->command < statement2->command;
}

static bool sortByScopeComparator(PStatement statement1,PStatement statement2){
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeIn) {
        if (statement2->kind != StatementKind::skUserCodeIn)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeIn) {
        return false;
        // show keywords first
    } else if (statement1->kind == StatementKind::skKeyword) {
        if (statement2->kind != StatementKind::skKeyword)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skKeyword) {
        return false;
        // Show stuff from local headers first
    } else if (statement1->inSystemHeader && ! statement2->inSystemHeader) {
        return true;
    } else if (!statement1->inSystemHeader && statement2->inSystemHeader) {
        return false;
        // Show local statements first
    } else if (statement1->scope != StatementScope::ssGlobal
               && statement1->scope == StatementScope::ssGlobal ) {
        return true;
    } else if (statement1->scope == StatementScope::ssGlobal
               && statement1->scope != StatementScope::ssGlobal ) {
        return false;
        // otherwise, sort by name
    } else
        return statement1->command < statement2->command;
}

static bool sortWithUsageComparator(PStatement statement1,PStatement statement2) {
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeIn) {
        if (statement2->kind != StatementKind::skUserCodeIn)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeIn) {
        return false;
        //show most freq first
    } else if (statement1->freqTop > statement2->freqTop) {
        return true;
    } else if (statement1->freqTop < statement2->freqTop) {
        return false;
        // show keywords first
    } else if ((statement1->kind == StatementKind::skKeyword)
               && (statement2->kind != StatementKind::skKeyword)) {
        return true;
    } else if ((statement1->kind != StatementKind::skKeyword)
               && (statement2->kind == StatementKind::skKeyword)) {
        return false;
    } else
        return statement1->command < statement2->command;
}

static bool sortByScopeWithUsageComparator(PStatement statement1,PStatement statement2){
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeIn) {
        if (statement2->kind != StatementKind::skUserCodeIn)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeIn) {
        return false;
        //show most freq first
    } else if (statement1->freqTop > statement2->freqTop) {
        return true;
    } else if (statement1->freqTop < statement2->freqTop) {
        return false;
        // show keywords first
    } else if (statement1->kind == StatementKind::skKeyword) {
        if (statement2->kind != StatementKind::skKeyword)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skKeyword) {
        return false;
        // Show stuff from local headers first
    } else if (statement1->inSystemHeader && ! statement2->inSystemHeader) {
        return true;
    } else if (!statement1->inSystemHeader && statement2->inSystemHeader) {
        return false;
        // Show local statements first
    } else if (statement1->scope != StatementScope::ssGlobal
               && statement1->scope == StatementScope::ssGlobal ) {
        return true;
    } else if (statement1->scope == StatementScope::ssGlobal
               && statement1->scope != StatementScope::ssGlobal ) {
        return false;
        // otherwise, sort by name
    } else
        return statement1->command < statement2->command;
}


void CodeCompletionView::filterList(const QString &member)
{
    QMutexLocker locker(&mMutex);
    mCompletionStatementList.clear();
    if (!mParser)
        return;
    if (!mParser->enabled())
        return;
    //we don't need to freeze here since we use smart pointers
    //  and data have been retrieved from the parser
//    if (!mParser->freeze())
//        return;
//    {
//        auto action = finally([this]{
//            mParser->unFreeze();
//        });
//        if (mParserSerialId!=mParser->serialId()) {
//            return;
//        }

    QList<PStatement> tmpList;
    if (!member.isEmpty()) { // filter
        tmpList.reserve(mFullCompletionStatementList.size());
        for (PStatement statement:mFullCompletionStatementList) {
            Qt::CaseSensitivity cs = (mIgnoreCase?
                                          Qt::CaseInsensitive:
                                          Qt::CaseSensitive);
            if (statement->command.startsWith(member, cs))
                tmpList.append(statement);
        }
    } else
        tmpList = mCompletionStatementList;
    if (mRecordUsage) {
        int topCount = 0;
        int secondCount = 0;
        int thirdCount = 0;
        int usageCount;
        for (PStatement statement:tmpList) {
            if (statement->usageCount == 0) {
                usageCount = mSymbolUsage.value(statement->fullName,0);
                if (usageCount == 0)
                    continue;
                statement->usageCount = usageCount;
            } else
                usageCount = statement->usageCount;
            if (usageCount>topCount) {
                thirdCount = secondCount;
                secondCount = topCount;
                topCount = usageCount;
            } else if (usageCount == topCount) {
                continue;
            } else if (usageCount > secondCount) {
                thirdCount = secondCount;
                secondCount = usageCount;
            } else if (usageCount == secondCount) {
                continue;
            } else if (usageCount>thirdCount) {
                thirdCount = usageCount;
            }
        }
        for (PStatement statement:tmpList) {
            if (statement->usageCount == 0) {
                statement->freqTop = 0;
            } else if  (statement->usageCount == topCount) {
                statement->freqTop = 30;
            } else if  (statement->usageCount == secondCount) {
                statement->freqTop = 20;
            } else if  (statement->usageCount == thirdCount) {
                statement->freqTop = 10;
            }
        }
        if (mSortByScope) {
            std::sort(tmpList.begin(),tmpList.end(),sortByScopeWithUsageComparator);
        } else {
            std::sort(tmpList.begin(),tmpList.end(),sortWithUsageComparator);
        }
    } else if (mSortByScope) {
        std::sort(tmpList.begin(),tmpList.end(),sortByScopeComparator);
    } else {
        std::sort(tmpList.begin(),tmpList.end(),defaultComparator);
    }
//    }
}

CodeCompletionListView::CodeCompletionListView(QWidget *parent) : QListView(parent)
{

}

void CodeCompletionListView::keyPressEvent(QKeyEvent *event)
{
    if (!mKeypressedCallback(event)) {
        QListView::keyPressEvent(event);
    }
}

const KeyPressedCallback &CodeCompletionListView::keypressedCallback() const
{
    return mKeypressedCallback;
}

void CodeCompletionListView::setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback)
{
    mKeypressedCallback = newKeypressedCallback;
}
