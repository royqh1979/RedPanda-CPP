/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "codecompletionpopup.h"
#include "../utils.h"
#include "../mainwindow.h"
#include "../editor.h"
#include "../editorlist.h"
#include "../symbolusagemanager.h"
#include "../colorscheme.h"
#include "../iconsmanager.h"

#include <QKeyEvent>
#include <QVBoxLayout>
#include <QDebug>
#include <QApplication>
#include <QPainter>

CodeCompletionPopup::CodeCompletionPopup(QWidget *parent) :
    QWidget(parent),
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    mMutex()
#else
    mMutex(QMutex::Recursive)
#endif
{
    setWindowFlags(Qt::Popup);
    mListView = new CodeCompletionListView(this);
    mModel=new CodeCompletionListModel(&mCompletionStatementList);
    mDelegate = new CodeCompletionListItemDelegate(mModel,this);
    QItemSelectionModel *m=mListView->selectionModel();
    mListView->setModel(mModel);
    delete m;
    mListView->setItemDelegate(mDelegate);
    setLayout(new QVBoxLayout());
    layout()->addWidget(mListView);
    layout()->setMargin(0);
    mListView->setFocus();

    mShowKeywords=true;
    mRecordUsage = false;
    mSortByScope = true;

    mShowCount = 1000;
    mShowCodeSnippets = true;

    mIgnoreCase = false;

    mHideSymbolsStartWithTwoUnderline = false;
    mHideSymbolsStartWithUnderline = false;
}

CodeCompletionPopup::~CodeCompletionPopup()
{
    delete mListView;
    delete mModel;
}

void CodeCompletionPopup::setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback)
{
    mListView->setKeypressedCallback(newKeypressedCallback);
}

void CodeCompletionPopup::prepareSearch(
        const QString &preWord,
        const QStringList &ownerExpression,
        const QString& memberOperator,
        const QStringList& memberExpression,
        const QString &filename,
        int line,
        CodeCompletionType type,
        const QSet<QString>& customKeywords)
{
    QMutexLocker locker(&mMutex);
    if (!isEnabled())
        return;
    //Screen.Cursor := crHourglass;
    QCursor oldCursor = cursor();
    setCursor(Qt::CursorShape::WaitCursor);

    mMemberPhrase = memberExpression.join("");
    mMemberOperator = memberOperator;
    switch(type) {
    case CodeCompletionType::ComplexKeyword:
        getCompletionListForComplexKeyword(preWord);
        break;
    case CodeCompletionType::Types:
        mIncludedFiles = mParser->getIncludedFiles(filename);
        getCompletionListForTypes(preWord,filename,line);
        break;
    case CodeCompletionType::FunctionWithoutDefinition:
        mIncludedFiles = mParser->getIncludedFiles(filename);
        getCompletionForFunctionWithoutDefinition(preWord, ownerExpression,memberOperator,memberExpression, filename,line);
        break;
    case CodeCompletionType::Namespaces:
        mIncludedFiles = mParser->getIncludedFiles(filename);
        getCompletionListForNamespaces(preWord,filename,line);
        break;
    case CodeCompletionType::KeywordsOnly:
        mIncludedFiles.clear();
        getKeywordCompletionFor(customKeywords);
        break;
    default:
        mIncludedFiles = mParser->getIncludedFiles(filename);
        getCompletionFor(ownerExpression,memberOperator,memberExpression, filename,line, customKeywords);
    }
    setCursor(oldCursor);
}

bool CodeCompletionPopup::search(const QString &memberPhrase, bool autoHideOnSingleResult)
{
    QMutexLocker locker(&mMutex);

    mMemberPhrase = memberPhrase;

    if (!isEnabled()) {
        hide();
        return false;
    }

    QCursor oldCursor = cursor();
    setCursor(Qt::CursorShape::WaitCursor);

    // filter fFullCompletionStatementList to fCompletionStatementList
    filterList(memberPhrase);

    //if can't find a destructor, maybe '~' is only an operator
//    if (mCompletionStatementList.isEmpty() && phrase.startsWith('~')) {
//        symbol = phrase.mid(1);
//        filterList(symbol);
//    }

    mModel->notifyUpdated();
    setCursor(oldCursor);

    if (!mCompletionStatementList.isEmpty()) {
        PColorSchemeItem item = mColors->value(StatementKind::skUnknown,PColorSchemeItem());
        if (item)
            mDelegate->setNormalColor(item->foreground());
        else
            mDelegate->setNormalColor(palette().color(QPalette::Text));
        item = mColors->value(StatementKind::skKeyword,PColorSchemeItem());
        if (item)
            mDelegate->setMatchedColor(item->foreground());
        else
            mDelegate->setMatchedColor(palette().color(QPalette::HighlightedText));
        mListView->setCurrentIndex(mModel->index(0,0));
        // if only one suggestion, and is exactly the symbol to search, hide the frame (the search is over)
        // if only one suggestion and auto hide , don't show the frame
        if(mCompletionStatementList.count() == 1)
            if (autoHideOnSingleResult
                    || (memberPhrase == mCompletionStatementList.front()->command)) {
            return true;
        }
    } else {
        hide();
    }
    return false;
}

PStatement CodeCompletionPopup::selectedStatement()
{
    if (isEnabled()) {
        int index = mListView->currentIndex().row();
        if (mListView->currentIndex().isValid()
                && (index<mCompletionStatementList.count()) ) {
            return mCompletionStatementList[index];
        } else {
            if (!mCompletionStatementList.isEmpty())
                return mCompletionStatementList.front();
            else
                return PStatement();
        }
    } else
        return PStatement();
}

void CodeCompletionPopup::addChildren(const PStatement& scopeStatement,
                                      const QString &fileName,
                                      int line,
                                      bool onlyTypes)
{
    if (scopeStatement && !isIncluded(scopeStatement->fileName)
      && !isIncluded(scopeStatement->definitionFileName))
        return;
    const StatementMap& children = mParser->statementList().childrenStatements(scopeStatement);
    if (children.isEmpty())
        return;

    if (onlyTypes) {
        if (!scopeStatement) { //Global scope
            for (const PStatement& childStatement: children) {
                if (!isTypeKind(childStatement->kind))
                    continue;
                if (childStatement->fileName.isEmpty()) {
                    // hard defines
                    addStatement(childStatement,fileName,-1);
                } else if (
                           isIncluded(childStatement->fileName)
                           || isIncluded(childStatement->definitionFileName)
                           ) {
                    //we must check if the statement is included by the file
                    addStatement(childStatement,fileName,line);
                }
            }
        } else {
            for (const PStatement& childStatement: children) {
                if (!isTypeKind(childStatement->kind))
                    continue;
                addStatement(childStatement,fileName,line);
            }
        }
    } else {
        if (!scopeStatement) { //Global scope
            for (const PStatement& childStatement: children) {
                if (childStatement->fileName.isEmpty()) {
                    // hard defines
                    addStatement(childStatement,fileName,-1);
                } else if (
                           isIncluded(childStatement->fileName)
                           || isIncluded(childStatement->definitionFileName)
                           ) {
                    //we must check if the statement is included by the file
                    addStatement(childStatement,fileName,line);
                }
            }
        } else {
            for (const PStatement& childStatement: children) {
                addStatement(childStatement,fileName,line);
            }
        }

    }
}

void CodeCompletionPopup::addFunctionWithoutDefinitionChildren(const PStatement& scopeStatement, const QString &fileName, int line)
{
    if (scopeStatement && !isIncluded(scopeStatement->fileName)
      && !isIncluded(scopeStatement->definitionFileName))
        return;
    const StatementMap& children = mParser->statementList().childrenStatements(scopeStatement);
    if (children.isEmpty())
        return;

    for (const PStatement& childStatement: children) {
        if (childStatement->inSystemHeader())
            continue;
        if (childStatement->fileName.isEmpty()) {
            // hard defines, do nothing
            continue;
        }
        switch(childStatement->kind) {
        case StatementKind::skConstructor:
        case StatementKind::skFunction:
        case StatementKind::skDestructor:
            if (!childStatement->hasDefinition())
                addStatement(childStatement,fileName,line);
            break;
        case StatementKind::skClass:
        case StatementKind::skNamespace:
            if (isIncluded(childStatement->fileName))
                addStatement(childStatement,fileName,line);
            break;
        default:
            break;
        }
    }
}

void CodeCompletionPopup::addStatement(const PStatement& statement, const QString &fileName, int line)
{
    if (mAddedStatements.contains(statement->command))
        return;
    if (statement->kind == StatementKind::skConstructor
            || statement->kind == StatementKind::skDestructor
            || statement->kind == StatementKind::skBlock
            || statement->kind == StatementKind::skLambda
            || statement->properties.testFlag(StatementProperty::spOperatorOverloading)
            || statement->properties.testFlag(StatementProperty::spDummyStatement)
            )
        return;
    if ((line!=-1)
            && (line < statement->line)
            && (fileName == statement->fileName))
        return;
    mAddedStatements.insert(statement->command);
    if (statement->kind == StatementKind::skUserCodeSnippet || !statement->command.contains("<"))
        mFullCompletionStatementList.append(statement);
}

static bool nameComparator(PStatement statement1,PStatement statement2) {
    return statement1->command < statement2->command;
}

static bool defaultComparator(PStatement statement1,PStatement statement2) {
    if (statement1->matchPosSpan!=statement2->matchPosSpan)
        return statement1->matchPosSpan < statement2->matchPosSpan;
    if (statement1->firstMatchLength != statement2->firstMatchLength)
        return statement1->firstMatchLength > statement2->firstMatchLength;
    if (statement1->matchPosTotal != statement2->matchPosTotal)
        return statement1->matchPosTotal < statement2->matchPosTotal;
    if (statement1->caseMatched != statement2->caseMatched)
        return statement1->caseMatched > statement2->caseMatched;
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeSnippet) {
        if (statement2->kind != StatementKind::skUserCodeSnippet)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeSnippet) {
        return false;
        // show keywords first
    } else if ((statement1->kind == StatementKind::skKeyword)
               && (statement2->kind != StatementKind::skKeyword)) {
        return true;
    } else if ((statement1->kind != StatementKind::skKeyword)
               && (statement2->kind == StatementKind::skKeyword)) {
        return false;
    } else
        return nameComparator(statement1,statement2);
}

static bool sortByScopeComparator(PStatement statement1,PStatement statement2){
    if (statement1->matchPosSpan!=statement2->matchPosSpan)
        return statement1->matchPosSpan < statement2->matchPosSpan;
    if (statement1->firstMatchLength != statement2->firstMatchLength)
        return statement1->firstMatchLength > statement2->firstMatchLength;
    if (statement1->matchPosTotal != statement2->matchPosTotal)
        return statement1->matchPosTotal < statement2->matchPosTotal;
    if (statement1->caseMatched != statement2->caseMatched)
        return statement1->caseMatched > statement2->caseMatched;
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeSnippet) {
        if (statement2->kind != StatementKind::skUserCodeSnippet)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeSnippet) {
        return false;
        // show non-system defines before keyword
    } else if (statement1->kind == StatementKind::skKeyword) {
        if (statement2->kind != StatementKind::skKeyword) {
            //s1 keyword / s2 system defines, s1 < s2, should return true
            //s1 keyword / s2 not system defines, s2 < s1, should return false;
            return  statement2->inSystemHeader();
        } else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skKeyword) {
        //s1 system defines / s2 keyword, s2 < s1, should return false;
        //s1 not system defines / s2 keyword, s1 < s2, should return true;
        return  (!statement1->inSystemHeader());
    }
    // Show stuff from local headers first
    if (statement1->inSystemHeader() != statement2->inSystemHeader())
        return !(statement1->inSystemHeader());
        // Show local statements first
    if (statement1->scope != StatementScope::Global
               && statement2->scope == StatementScope::Global ) {
        return true;
    } else if (statement1->scope == StatementScope::Global
               && statement2->scope != StatementScope::Global ) {
        return false;
    } else
        return nameComparator(statement1,statement2);
}

static bool sortWithUsageComparator(PStatement statement1,PStatement statement2) {
    if (statement1->matchPosSpan!=statement2->matchPosSpan)
        return statement1->matchPosSpan < statement2->matchPosSpan;
    if (statement1->firstMatchLength != statement2->firstMatchLength)
        return statement1->firstMatchLength > statement2->firstMatchLength;
    if (statement1->matchPosTotal != statement2->matchPosTotal)
        return statement1->matchPosTotal < statement2->matchPosTotal;
    if (statement1->caseMatched != statement2->caseMatched)
        return statement1->caseMatched > statement2->caseMatched;
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeSnippet) {
        if (statement2->kind != StatementKind::skUserCodeSnippet)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeSnippet) {
        return false;
        //show most freq first
    }
    if (statement1->usageCount != statement2->usageCount)
        return statement1->usageCount > statement2->usageCount;

    if ((statement1->kind != StatementKind::skKeyword)
               && (statement2->kind == StatementKind::skKeyword)) {
        return true;
    } else if ((statement1->kind == StatementKind::skKeyword)
               && (statement2->kind != StatementKind::skKeyword)) {
        return false;
    } else
        return nameComparator(statement1,statement2);
}

static bool sortByScopeWithUsageComparator(PStatement statement1,PStatement statement2){
    if (statement1->matchPosSpan!=statement2->matchPosSpan)
        return statement1->matchPosSpan < statement2->matchPosSpan;
    if (statement1->firstMatchLength != statement2->firstMatchLength)
        return statement1->firstMatchLength > statement2->firstMatchLength;
    if (statement1->matchPosTotal != statement2->matchPosTotal)
        return statement1->matchPosTotal < statement2->matchPosTotal;
    if (statement1->caseMatched != statement2->caseMatched)
        return statement1->caseMatched > statement2->caseMatched;
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeSnippet) {
        if (statement2->kind != StatementKind::skUserCodeSnippet)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeSnippet) {
        return false;
        //show most freq first
    }
    if (statement1->usageCount != statement2->usageCount)
        return statement1->usageCount > statement2->usageCount;

        // show non-system defines before keyword
    if (statement1->kind == StatementKind::skKeyword) {
        if (statement2->kind != StatementKind::skKeyword) {
            //s1 keyword / s2 system defines, s1 < s2, should return true
            //s1 keyword / s2 not system defines, s2 < s1, should return false;
            return  statement2->inSystemHeader();
        } else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skKeyword) {
        //s1 system defines / s2 keyword, s2 < s1, should return false;
        //s1 not system defines / s2 keyword, s1 < s2, should return true;
        return  (!statement1->inSystemHeader());
    }
    // Show stuff from local headers first
    if (statement1->inSystemHeader() != statement2->inSystemHeader())
        return !(statement1->inSystemHeader());
        // Show local statements first
    if (statement1->scope != StatementScope::Global
               && statement2->scope == StatementScope::Global ) {
        return true;
    } else if (statement1->scope == StatementScope::Global
               && statement2->scope != StatementScope::Global ) {
        return false;
    } else
        return nameComparator(statement1,statement2);
}

void CodeCompletionPopup::filterList(const QString &member)
{
    QMutexLocker locker(&mMutex);
    mCompletionStatementList.clear();
//    if (!mParser)
//        return;
//    if (!mParser->enabled())
//        return;
    //we don't need to freeze here since we use smart pointers
    //  and data have been retrieved from the parser

    mCompletionStatementList.clear();
    mCompletionStatementList.reserve(mFullCompletionStatementList.size());
    bool hideSymbolsTwoUnderline = mHideSymbolsStartWithTwoUnderline && !member.startsWith("__") ;
    bool hideSymbolsUnderline = mHideSymbolsStartWithUnderline && !member.startsWith("_") ;
    int len = member.length();
    foreach (const PStatement& statement, mFullCompletionStatementList) {

        int matched = 0;
        int caseMatched = 0;
        QString command = statement->command;
        int pos = 0;
        int lastPos = -10;
        int totalPos = 0;
        statement->matchPositions.clear();
        if (hideSymbolsTwoUnderline && statement->command.startsWith("__")) {
            continue;
        } else if (hideSymbolsUnderline && statement->command.startsWith("_")) {
            continue;
        } else {
            foreach (const QChar& ch, member) {
                if (mIgnoreCase)
                    pos = command.indexOf(ch,pos,Qt::CaseInsensitive);
                else
                    pos = command.indexOf(ch,pos,Qt::CaseSensitive);
                if (pos<0) {
                    break;
                }
                if (pos == lastPos+1) {
                    statement->matchPositions.last()->end++;
                } else {
                    PStatementMathPosition matchPosition=std::make_shared<StatementMatchPosition>();
                    matchPosition->start = pos;
                    matchPosition->end = pos+1;
                    statement->matchPositions.append(matchPosition);
                }
                if (ch==command[pos])
                    caseMatched++;
                matched++;
                totalPos += pos;
                lastPos = pos;
                pos+=1;
            }
        }

        if (mIgnoreCase && matched== len) {
            statement->caseMatched = caseMatched;
            statement->matchPosTotal = totalPos;
            if (member.length()>0) {
                statement->firstMatchLength = statement->matchPositions.front()->end - statement->matchPositions.front()->start;
                statement->matchPosSpan = statement->matchPositions.last()->end - statement->matchPositions.front()->start;
            } else
                statement->firstMatchLength = 0;
            mCompletionStatementList.append(statement);
        } else if (caseMatched == len) {
            statement->caseMatched = caseMatched;
            statement->matchPosTotal = totalPos;
            if (member.length()>0) {
                statement->firstMatchLength = statement->matchPositions.front()->end - statement->matchPositions.front()->start;
                statement->matchPosSpan = statement->matchPositions.last()->end - statement->matchPositions.front()->start;
            } else
                statement->firstMatchLength = 0;
            mCompletionStatementList.append(statement);
        } else {
            statement->matchPositions.clear();
            statement->caseMatched = 0;
            statement->matchPosTotal = 0;
            statement->firstMatchLength = 0;
            statement->matchPosSpan = 0;
        }
    }
    if (mRecordUsage) {
        int usageCount;
        foreach (const PStatement& statement,mCompletionStatementList) {
            if (statement->usageCount == -1) {
                PSymbolUsage usage = pMainWindow->symbolUsageManager()->findUsage(statement->fullName);
                if (usage) {
                    usageCount = usage->count;
                } else {
                    usageCount = 0;
                }
                statement->usageCount = usageCount;
            }
        }
        if (mSortByScope) {
            std::sort(mCompletionStatementList.begin(),
                      mCompletionStatementList.end(),
                      sortByScopeWithUsageComparator);
        } else {
            std::sort(mCompletionStatementList.begin(),
                      mCompletionStatementList.end(),
                      sortWithUsageComparator);
        }
    } else if (mSortByScope) {
        std::sort(mCompletionStatementList.begin(),
                  mCompletionStatementList.end(),
                  sortByScopeComparator);
    } else {
        std::sort(mCompletionStatementList.begin(),
                  mCompletionStatementList.end(),
                  defaultComparator);
    }
    //    }
}

void CodeCompletionPopup::getKeywordCompletionFor(const QSet<QString> &customKeywords)
{
    //add keywords
    if (!customKeywords.isEmpty()) {
        foreach (const QString& keyword,customKeywords) {
            addKeyword(keyword);
        }
    }
}

void CodeCompletionPopup::getCompletionFor(
        QStringList ownerExpression,
        const QString& memberOperator,
        const QStringList& memberExpression,
        const QString &fileName,
        int line,
        const QSet<QString>& customKeywords)
{

    if (memberOperator.isEmpty() && ownerExpression.isEmpty() && memberExpression.isEmpty())
        return;

    bool isLambdaReturnType = (
                memberOperator=="->"
                && ownerExpression.startsWith("[")
                && ownerExpression.endsWith(")"));

    if (memberOperator.isEmpty()) {
        //C++ preprocessor directives
        if (mMemberPhrase.startsWith('#')) {
            if (mShowKeywords) {
                foreach (const QString& keyword, CppDirectives) {
                    addKeyword(keyword);
                }
            }
            return;
        }

        //docstring tags (javadoc style)
        if (mMemberPhrase.startsWith('@')) {
            if (mShowKeywords) {
                foreach (const QString& keyword,JavadocTags) {
                    addKeyword(keyword);
                }
            }
            return;
        }

        //the identifier to be completed is not a member of variable/class
        if (mShowCodeSnippets) {
            //add custom code templates
            foreach (const PCodeSnippet& codeIn,mCodeSnippets) {
                if (!codeIn->code.isEmpty()) {
                    PStatement statement = std::make_shared<Statement>();
                    statement->command = codeIn->prefix;
                    statement->value = codeIn->code;
                    statement->kind = StatementKind::skUserCodeSnippet;
                    statement->fullName = codeIn->prefix;
                    statement->usageCount = 0;
                    mFullCompletionStatementList.append(statement);
                }
            }
        }

        if (mShowKeywords) {
            //add keywords
            if (!customKeywords.isEmpty()) {
                foreach (const QString& keyword,customKeywords) {
                    addKeyword(keyword);
                }
            }
        }
    } else if  (isLambdaReturnType) {
            foreach (const QString& keyword,CppTypeKeywords) {
                addKeyword(keyword);
            }
    }

    if (!mParser || !mParser->enabled())
        return;

    if (!mParser->freeze())
        return;
    {
        auto action = finally([this]{
            mParser->unFreeze();
        });

        if (memberOperator.isEmpty() || isLambdaReturnType) {
            PStatement scopeStatement = mCurrentScope;
            // repeat until reach global
            while (scopeStatement) {
                //add members of current scope that not added before
                if (scopeStatement->kind == StatementKind::skNamespace) {
                    PStatementList namespaceStatementsList =
                            mParser->findNamespace(scopeStatement->fullName);
                    foreach (const PStatement& namespaceStatement,*namespaceStatementsList) {
                        addChildren(namespaceStatement, fileName, line, isLambdaReturnType);
                    }
                } else if (scopeStatement->kind == StatementKind::skClass) {
                    addChildren(scopeStatement, fileName, -1, isLambdaReturnType);
                } else {
                    addChildren(scopeStatement, fileName, line, isLambdaReturnType);
                }

                // add members of all usings (in current scope ) and not added before
                foreach (const QString& namespaceName,scopeStatement->usingList) {
                    PStatementList namespaceStatementsList =
                            mParser->findNamespace(namespaceName);
                    if (!namespaceStatementsList)
                        continue;
                    foreach (const PStatement& namespaceStatement,*namespaceStatementsList) {
                        addChildren(namespaceStatement, fileName, line, isLambdaReturnType);
                    }
                }
                if (scopeStatement->kind == StatementKind::skLambda) {
                    foreach (const QString& phrase, scopeStatement->lambdaCaptures) {
                        if (phrase=="&" || phrase == "=" || phrase =="this")
                            continue;
                        PStatement statement = mParser->findStatementOf(
                            scopeStatement->fileName,
                                phrase,scopeStatement->line);
                        if (statement)
                            addStatement(statement,scopeStatement->fileName, scopeStatement->line);
                    }
                    if (scopeStatement->lambdaCaptures.contains("&")
                            || scopeStatement->lambdaCaptures.contains("=")) {
                        scopeStatement = scopeStatement->parentScope.lock();
                        continue;
                    } else if (scopeStatement->lambdaCaptures.contains("this")) {
                        do {
                            scopeStatement = scopeStatement->parentScope.lock();
                        } while (scopeStatement && scopeStatement->kind!=StatementKind::skClass);
                        continue;
                    }
                    break;
                }
                scopeStatement=scopeStatement->parentScope.lock();
            }

            // add all global members and not added before
            addChildren(nullptr, fileName, line, isLambdaReturnType);

            // add members of all fusings
            mUsings = mParser->getFileUsings(fileName);
            foreach (const QString& namespaceName, mUsings) {
                PStatementList namespaceStatementsList =
                        mParser->findNamespace(namespaceName);
                if (!namespaceStatementsList)
                    continue;
                foreach (const PStatement& namespaceStatement, *namespaceStatementsList) {
                    addChildren(namespaceStatement, fileName, line, isLambdaReturnType);
                }
            }

        } else {
            //the identifier to be completed is a member of variable/class
            if (memberOperator == "::" && ownerExpression.isEmpty()) {
                // start with '::', we only find in global
                // add all global members and not added before
                addChildren(nullptr, fileName, line);
                return;
            }
            if (memberExpression.length()==2 && memberExpression.front()!="~")
                return;
            if (memberExpression.length()>2)
                return;

            PStatement scope = mCurrentScope;//the scope the expression in
            PStatement parentTypeStatement;
//            QString scopeName = ownerExpression.join("");
//            PStatement ownerStatement = mParser->findStatementOf(
//                        fileName,
//                        scopeName,
//                        mCurrentStatement,
//                        parentTypeStatement);
            PEvalStatement ownerStatement = mParser->evalExpression(fileName,
                                        ownerExpression,
                                        scope);
//            qDebug()<<scopeName;
//            qDebug()<<memberOperator;
//            qDebug()<<memberExpression;
            if(!ownerStatement  || !ownerStatement->effectiveTypeStatement) {
//                qDebug()<<"statement not found!";
                return;
            }
//            qDebug()<<"found: "<<ownerStatement->fullName;
            if (memberOperator == "::") {
                if (ownerStatement->kind==EvalStatementKind::Namespace) {
                    //there might be many statements corresponding to one namespace;
                    PStatementList namespaceStatementsList =
                            mParser->findNamespace(ownerStatement->baseType);
                    if (namespaceStatementsList) {
                        foreach (const PStatement& namespaceStatement, *namespaceStatementsList) {
                            addChildren(namespaceStatement, fileName, line);
                        }
                    }
                    return;
                }
            }

            // find the most inner scope statement that has a name (not a block)
            PStatement scopeTypeStatement = mCurrentScope;
            while (scopeTypeStatement && !isScopeTypeKind(scopeTypeStatement->kind)) {
                scopeTypeStatement = scopeTypeStatement->parentScope.lock();
            }
            if (
                    (memberOperator != "::")
                    && (
                        ownerStatement->kind == EvalStatementKind::Variable)
                    ) {
                // Get type statement  of current (scope) statement
                PStatement classTypeStatement = ownerStatement->effectiveTypeStatement;

                if (!classTypeStatement)
                    return;
                // It's a iterator
                if (ownerStatement
                        && ownerStatement->typeStatement
                        && STLIterators.contains(ownerStatement->typeStatement->command)
                        && (memberOperator == "->"
                            || memberOperator == "->*")
                ) {
                    PStatement parentScope = ownerStatement->typeStatement->parentScope.lock();
                    if (STLContainers.contains(parentScope->fullName)) {
                        QString typeName=mParser->findFirstTemplateParamOf(fileName,ownerStatement->templateParams, parentScope);
//                        qDebug()<<"typeName"<<typeName<<lastResult->baseStatement->type<<lastResult->baseStatement->command;
                        classTypeStatement=mParser->findTypeDefinitionOf(fileName, typeName,parentScope);
                        if (!classTypeStatement)
                            return;
                    } else if (STLMaps.contains(parentScope->fullName)) {
                        QString typeName="std::pair";
                        classTypeStatement=mParser->findTypeDefinitionOf(fileName, typeName,parentScope);
                        if (!classTypeStatement)
                            return;
                    }
                } else if (STLPointers.contains(classTypeStatement->fullName)
                   && (memberOperator == "->"
                       || memberOperator == "->*")
                        && ownerStatement->baseStatement) {
                                   //is a smart pointer
                    QString typeName= mParser->findFirstTemplateParamOf(
                                fileName,
                                ownerStatement->baseStatement->type,
                                scope);
                    classTypeStatement = mParser->findTypeDefinitionOf(
                                fileName,
                                typeName,
                                scope);
                    if (!classTypeStatement)
                        return;
                } else {
                    //normal member access
                    if (memberOperator=="." && ownerStatement->pointerLevel !=0)
                        return;
                    if (memberOperator=="->" && ownerStatement->pointerLevel!=1)
                        return;
                }
                if (!isIncluded(classTypeStatement->fileName) &&
                    !isIncluded(classTypeStatement->definitionFileName))
                    return;
                if ((classTypeStatement == scopeTypeStatement) || (ownerStatement->effectiveTypeStatement->command == "this")) {
                    //we can use all members
                    addChildren(classTypeStatement,fileName,-1);
                } else { // we can only use public members
                    const StatementMap& children = mParser->statementList().childrenStatements(classTypeStatement);
                    if (children.isEmpty())
                        return;
                    foreach (const PStatement& childStatement, children) {
                        if ((childStatement->accessibility==StatementAccessibility::Public)
                                && !(
                                    childStatement->kind == StatementKind::skConstructor
                                    || childStatement->kind == StatementKind::skDestructor)
                                && !mAddedStatements.contains(childStatement->command)) {
                            addStatement(childStatement,fileName,-1);
                        }
                    }
                }
            //todo friend
            } else if ((memberOperator == "::")
                       && (ownerStatement->kind == EvalStatementKind::Type)) {
                //we can add all child enum definess
                PStatement classTypeStatement = ownerStatement->effectiveTypeStatement;
                if (!classTypeStatement)
                    return;
                if (!isIncluded(classTypeStatement->fileName) &&
                    !isIncluded(classTypeStatement->definitionFileName))
                    return;
                if (classTypeStatement->kind == StatementKind::skEnumType
                        || classTypeStatement->kind == StatementKind::skEnumClassType) {
                    const StatementMap& children =
                            mParser->statementList().childrenStatements(classTypeStatement);
                    foreach (const PStatement& child,children) {
                        addStatement(child,fileName,line);
                    }
                } else {
                    //class
                    if (classTypeStatement == scopeTypeStatement) {
                        //we can use all static members
                        const StatementMap& children =
                                mParser->statementList().childrenStatements(classTypeStatement);
                        foreach (const PStatement& childStatement, children) {
                            if (
                              (childStatement->isStatic())
                               || (childStatement->kind == StatementKind::skTypedef
                                || childStatement->kind == StatementKind::skClass
                                || childStatement->kind == StatementKind::skEnum
                                || childStatement->kind == StatementKind::skEnumClassType
                                || childStatement->kind == StatementKind::skEnumType
                                   )) {
                                addStatement(childStatement,fileName,-1);
                            }
                        }
                    } else {
                        // we can only use public static members
                        const StatementMap& children =
                                mParser->statementList().childrenStatements(classTypeStatement);
                        foreach (const PStatement& childStatement,children) {
                            if (
                              (childStatement->isStatic())
                               || (childStatement->kind == StatementKind::skTypedef
                                || childStatement->kind == StatementKind::skClass
                                || childStatement->kind == StatementKind::skEnum
                                || childStatement->kind == StatementKind::skEnumClassType
                                || childStatement->kind == StatementKind::skEnumType
                                   )) {
                                if (childStatement->accessibility == StatementAccessibility::Public)
                                    addStatement(childStatement,fileName,-1);
                            }
                        }
                    }
                }
            }
        }
    }
}

void CodeCompletionPopup::getCompletionForFunctionWithoutDefinition(const QString& preWord, QStringList ownerExpression, const QString &memberOperator, const QStringList &memberExpression, const QString &fileName, int line)
{
    if(!mParser) {
        return;
    }
    if (!mParser->enabled())
        return;
    if (memberOperator.isEmpty() && ownerExpression.isEmpty() && memberExpression.isEmpty())
        return;

    if (!mParser->freeze())
        return;
    {
        auto action = finally([this]{
            mParser->unFreeze();
        });

        if (memberOperator.isEmpty()) {
            getCompletionListForComplexKeyword(preWord);
            PStatement scopeStatement = mCurrentScope;
            //add members of current scope that not added before
            while (scopeStatement && scopeStatement->kind!=StatementKind::skNamespace
                   && scopeStatement->kind!=StatementKind::skClass) {
                scopeStatement = scopeStatement->parentScope.lock();
            }
            if (scopeStatement) {
                if (scopeStatement->kind == StatementKind::skNamespace) {
                    //namespace;
                    PStatementList namespaceStatementsList =
                            mParser->findNamespace(scopeStatement->fullName);
                    if (namespaceStatementsList) {
                        foreach (const PStatement& namespaceStatement, *namespaceStatementsList) {
                            addFunctionWithoutDefinitionChildren(namespaceStatement, fileName, line);
                        }
                    }
                } else {
                    //class
                    addKeyword("operator");
                    addFunctionWithoutDefinitionChildren(scopeStatement, fileName, line);
                }
            } else {
                //global
                addFunctionWithoutDefinitionChildren(scopeStatement, fileName, line);
            }
        } else {
            if (memberOperator != "::")
                return;
            //the identifier to be completed is a member of variable/class

            if (ownerExpression.isEmpty()) {
                // start with '::', we only find in global
                // add all global members and not added before
                addFunctionWithoutDefinitionChildren(nullptr, fileName, line);
                return;
            }
            if (memberExpression.length()==2 && memberExpression.front()!="~")
                return;
            if (memberExpression.length()>2)
                return;

            PStatement scope = mCurrentScope;//the scope the expression in
            PStatement parentTypeStatement;
            PEvalStatement ownerStatement = mParser->evalExpression(fileName,
                                        ownerExpression,
                                        scope);
            if(!ownerStatement  || !ownerStatement->effectiveTypeStatement) {
                return;
            }
            if (ownerStatement->kind==EvalStatementKind::Namespace) {
                //there might be many statements corresponding to one namespace;
                PStatementList namespaceStatementsList =
                        mParser->findNamespace(ownerStatement->baseType);
                if (namespaceStatementsList) {
                    foreach (const PStatement& namespaceStatement, *namespaceStatementsList) {
                        addFunctionWithoutDefinitionChildren(namespaceStatement, fileName, line);
                    }
                }
                return;
            } else if (ownerStatement->effectiveTypeStatement->kind == StatementKind::skClass) {
                addKeyword("operator");
                addFunctionWithoutDefinitionChildren(ownerStatement->effectiveTypeStatement, fileName, line);
            }
        }
    }
}

void CodeCompletionPopup::getCompletionListForComplexKeyword(const QString &preWord)
{
    mFullCompletionStatementList.clear();
    if (preWord == "long") {
        addKeyword("long");
        addKeyword("double");
        addKeyword("int");
    } else if (preWord == "short") {
        addKeyword("int");
    } else if (preWord == "signed") {
        addKeyword("long");
        addKeyword("short");
        addKeyword("int");
        addKeyword("char");
    } else if (preWord == "unsigned") {
        addKeyword("long");
        addKeyword("short");
        addKeyword("int");
        addKeyword("char");
    } else if (preWord == "using") {
        addKeyword("namespace");
    }
}

void CodeCompletionPopup::getCompletionListForNamespaces(const QString &/*preWord*/,
                                                         const QString& fileName,
                                                         int line)
{
    if (!mParser->enabled())
        return;

    if (!mParser->freeze())
        return;
    {
        auto action = finally([this]{
            mParser->unFreeze();
        });
        QList<QString> namespaceNames = mParser->namespaces();
        foreach (const QString& name, namespaceNames) {
            PStatementList namespaces = mParser->findNamespace(name);
            foreach(const PStatement& statement, *namespaces) {
                if (isIncluded(statement->fileName)
                        || isIncluded(statement->definitionFileName)) {
                    addStatement(statement,fileName,line);
                    continue;
                }
            }

        }
    }
}

void CodeCompletionPopup::getCompletionListForTypes(const QString &preWord, const QString &fileName, int line)
{
    if (preWord=="typedef") {
        addKeyword("const");
        addKeyword("struct");
        addKeyword("class");
    }

    if (mShowKeywords) {
        //add keywords
        foreach (const QString& keyword,CppTypeKeywords) {
            addKeyword(keyword);
        }
    }
    if (!mParser->enabled())
        return;

    if (!mParser->freeze())
        return;
    {
        auto action = finally([this]{
            mParser->unFreeze();
        });
        QList<PStatement> statements = mParser->listTypeStatements(fileName,line);
        foreach(const PStatement& statement, statements) {
            if (isIncluded(statement->fileName)
                    || isIncluded(statement->definitionFileName)) {
                addStatement(statement,fileName,line);
            }
        }
    }
}

void CodeCompletionPopup::addKeyword(const QString &keyword)
{
    PStatement statement = std::make_shared<Statement>();
    statement->command = keyword;
    statement->kind = StatementKind::skKeyword;
    statement->fullName = keyword;
    statement->usageCount = 0;
    mFullCompletionStatementList.append(statement);
}

bool CodeCompletionPopup::isIncluded(const QString &fileName)
{
    return mIncludedFiles.contains(fileName);
}

void CodeCompletionPopup::setHideSymbolsStartWithTwoUnderline(bool newHideSymbolsStartWithTwoUnderline)
{
    mHideSymbolsStartWithTwoUnderline = newHideSymbolsStartWithTwoUnderline;
}

bool CodeCompletionPopup::hideSymbolsStartWithTwoUnderline() const
{
    return mHideSymbolsStartWithTwoUnderline;
}

bool CodeCompletionPopup::hideSymbolsStartWithUnderline() const
{
    return mHideSymbolsStartWithUnderline;
}

void CodeCompletionPopup::setHideSymbolsStartWithUnderline(bool newHideSymbolsStartWithUnderline)
{
    mHideSymbolsStartWithUnderline = newHideSymbolsStartWithUnderline;
}

const QString &CodeCompletionPopup::memberOperator() const
{
    return mMemberOperator;
}

const QList<PCodeSnippet> &CodeCompletionPopup::codeSnippets() const
{
    return mCodeSnippets;
}

void CodeCompletionPopup::setCodeSnippets(const QList<PCodeSnippet> &newCodeSnippets)
{
    mCodeSnippets = newCodeSnippets;
}

void CodeCompletionPopup::setColors(const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &newColors)
{
    mColors = newColors;
}

const QString &CodeCompletionPopup::memberPhrase() const
{
    return mMemberPhrase;
}

const PStatement &CodeCompletionPopup::currentScope() const
{
    return mCurrentScope;
}

void CodeCompletionPopup::setCurrentScope(const PStatement &newCurrentStatement)
{
    mCurrentScope = newCurrentStatement;
}

const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > >& CodeCompletionPopup::colors() const
{
    return mColors;
}

bool CodeCompletionPopup::sortByScope() const
{
    return mSortByScope;
}

void CodeCompletionPopup::setSortByScope(bool newSortByScope)
{
    mSortByScope = newSortByScope;
}

bool CodeCompletionPopup::ignoreCase() const
{
    return mIgnoreCase;
}

void CodeCompletionPopup::setIgnoreCase(bool newIgnoreCase)
{
    mIgnoreCase = newIgnoreCase;
}

bool CodeCompletionPopup::showCodeSnippets() const
{
    return mShowCodeSnippets;
}

void CodeCompletionPopup::setShowCodeSnippets(bool newShowCodeIns)
{
    mShowCodeSnippets = newShowCodeIns;
}

bool CodeCompletionPopup::showKeywords() const
{
    return mShowKeywords;
}

void CodeCompletionPopup::setShowKeywords(bool newShowKeywords)
{
    mShowKeywords = newShowKeywords;
}

bool CodeCompletionPopup::recordUsage() const
{
    return mRecordUsage;
}

void CodeCompletionPopup::setRecordUsage(bool newRecordUsage)
{
    mRecordUsage = newRecordUsage;
}

int CodeCompletionPopup::showCount() const
{
    return mShowCount;
}

void CodeCompletionPopup::setShowCount(int newShowCount)
{
    mShowCount = newShowCount;
}

const PCppParser &CodeCompletionPopup::parser() const
{
    return mParser;
}

void CodeCompletionPopup::setParser(const PCppParser &newParser)
{
    mParser = newParser;
}

void CodeCompletionPopup::hideEvent(QHideEvent *event)
{
    QMutexLocker locker(&mMutex);
    mListView->setKeypressedCallback(nullptr);
    mCompletionStatementList.clear();
//    foreach (PStatement statement, mFullCompletionStatementList) {
//        statement->matchPositions.clear();
//    }
    mFullCompletionStatementList.clear();
    mIncludedFiles.clear();
    mUsings.clear();
    mAddedStatements.clear();
    mCurrentScope = nullptr;
    mParser = nullptr;
    QWidget::hideEvent(event);
}

bool CodeCompletionPopup::event(QEvent *event)
{
    bool result = QWidget::event(event);
    if (event->type() == QEvent::FontChange) {
        mListView->setFont(font());
        mDelegate->setFont(font());
    }
    return result;
}

CodeCompletionListModel::CodeCompletionListModel(const StatementList *statements, QObject *parent):
    QAbstractListModel(parent),
    mStatements(statements)
{

}

int CodeCompletionListModel::rowCount(const QModelIndex &) const
{
    return mStatements->count();
}

QVariant CodeCompletionListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()>=mStatements->count())
        return QVariant();

    switch(role) {
    case Qt::DisplayRole: {
        PStatement statement = mStatements->at(index.row());
        return statement->command;
        }
    case Qt::DecorationRole:{
        PStatement statement = mStatements->at(index.row());
        return pIconsManager->getPixmapForStatement(statement);
    }
    }
    return QVariant();
}

PStatement CodeCompletionListModel::statement(const QModelIndex &index) const
{
    if (!index.isValid())
        return PStatement();
    if (index.row()>=mStatements->count())
        return PStatement();
    return mStatements->at(index.row());
}

QPixmap CodeCompletionListModel::statementIcon(const QModelIndex &index) const
{
    if (!index.isValid())
        return QPixmap();
    if (index.row()>=mStatements->count())
        return QPixmap();
    PStatement statement = mStatements->at(index.row());
    return pIconsManager->getPixmapForStatement(statement);
}

void CodeCompletionListModel::notifyUpdated()
{
    beginResetModel();
    endResetModel();
}

void CodeCompletionListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    PStatement statement;
    if (mModel && (statement = mModel->statement(index)) ) {
        painter->save();
        painter->setFont(font());
        QColor normalColor = mNormalColor;
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
            normalColor = option.palette.color(QPalette::HighlightedText);
        }
        QPixmap icon = mModel->statementIcon(index);
        int x=option.rect.left();
        if (!icon.isNull()) {
            qreal dpr=icon.devicePixelRatioF();
            int x1=x+(option.rect.height()-icon.width()/dpr)/2;
            int y1=option.rect.top()+(option.rect.height()-icon.height()/dpr)/2;
            painter->drawPixmap(x1,y1,icon);
            x+=option.rect.height();
        }
        QString text = statement->command;
        int pos=0;
        int y=option.rect.bottom()-painter->fontMetrics().descent();
        foreach (const PStatementMathPosition& matchPosition, statement->matchPositions) {
            if (pos<matchPosition->start) {
                QString t = text.mid(pos,matchPosition->start-pos);
                painter->setPen(normalColor);
                painter->drawText(x,y,t);
                x+=painter->fontMetrics().horizontalAdvance(t);
            }
            QString t = text.mid(matchPosition->start, matchPosition->end-matchPosition->start);
            painter->setPen(mMatchedColor);
            painter->drawText(x,y,t);
            x+=painter->fontMetrics().horizontalAdvance(t);
            pos=matchPosition->end;
        }
        if (pos<text.length()) {
            QString t = text.mid(pos,text.length()-pos);
            painter->setPen(normalColor);
            painter->drawText(x,y,t);
            x+=painter->fontMetrics().horizontalAdvance(t);
        }
        painter->restore();
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

CodeCompletionListModel *CodeCompletionListItemDelegate::model() const
{
    return mModel;
}

void CodeCompletionListItemDelegate::setModel(CodeCompletionListModel *newModel)
{
    mModel = newModel;
}

const QColor &CodeCompletionListItemDelegate::normalColor() const
{
    return mNormalColor;
}

void CodeCompletionListItemDelegate::setNormalColor(const QColor &newNormalColor)
{
    mNormalColor = newNormalColor;
}

const QColor &CodeCompletionListItemDelegate::matchedColor() const
{
    return mMatchedColor;
}

void CodeCompletionListItemDelegate::setMatchedColor(const QColor &newMatchedColor)
{
    mMatchedColor = newMatchedColor;
}

const QFont &CodeCompletionListItemDelegate::font() const
{
    return mFont;
}

void CodeCompletionListItemDelegate::setFont(const QFont &newFont)
{
    mFont = newFont;
}

CodeCompletionListItemDelegate::CodeCompletionListItemDelegate(CodeCompletionListModel *model, QWidget *parent) : QStyledItemDelegate(parent),
    mModel(model)
{
    mNormalColor = qApp->palette().color(QPalette::Text);
    mMatchedColor = qApp->palette().color(QPalette::BrightText);
}
