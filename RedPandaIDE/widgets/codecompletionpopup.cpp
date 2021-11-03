#include "codecompletionpopup.h"
#include "../utils.h"
#include "../mainwindow.h"
#include "../editor.h"
#include "../editorlist.h"
#include "../symbolusagemanager.h"
#include "../colorscheme.h"

#include <QKeyEvent>
#include <QVBoxLayout>
#include <QDebug>
#include <QApplication>

CodeCompletionPopup::CodeCompletionPopup(QWidget *parent) :
    QWidget(parent)
{
    setWindowFlags(Qt::Popup);
    mListView = new CodeCompletionListView(this);
    mModel=new CodeCompletionListModel(&mCompletionStatementList);
    mModel->setColorCallback([this](PStatement statement)->QColor{
        StatementKind kind;
        if (mParser) {
            kind = mParser->getKindOfStatement(statement);
        } else {
            kind = statement->kind;
        }
        PColorSchemeItem item = mColors->value(kind,PColorSchemeItem());
        if (item) {
            return item->foreground();
        }
        return palette().color(QPalette::Text);
    });
    mListView->setModel(mModel);
    setLayout(new QVBoxLayout());
    layout()->addWidget(mListView);
    layout()->setMargin(0);

    mShowKeywords=true;
    mUseCppKeyword=true;
    mRecordUsage = false;
    mSortByScope = true;

    mOnlyGlobals = false;
    mShowCount = 1000;
    mShowCodeSnippets = true;

    mIgnoreCase = false;

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

void CodeCompletionPopup::prepareSearch(const QString &phrase, const QString &filename, int line)
{
    QMutexLocker locker(&mMutex);
    if (!isEnabled())
        return;
    mPhrase = phrase;
    //Screen.Cursor := crHourglass;
    QCursor oldCursor = cursor();
    setCursor(Qt::CursorShape::WaitCursor);

    mIncludedFiles = mParser->getFileIncludes(filename);
    getCompletionFor(filename,phrase,line);

    //todo: notify model
//CodeComplForm.lbCompletion.Font.Size := FontSize;
//CodeComplForm.lbCompletion.ItemHeight := CodeComplForm.lbCompletion.Canvas.TextHeight('F')+6;
// Round(2 * FontSize);
//CodeComplForm.Update;
    setCursor(oldCursor);
}

bool CodeCompletionPopup::search(const QString &phrase, bool autoHideOnSingleResult)
{
    QMutexLocker locker(&mMutex);

    mPhrase = phrase;

    if (phrase.isEmpty()) {
        hide();
        return false;
    }
    if (!isEnabled()) {
        hide();
        return false;
    }

    QCursor oldCursor = cursor();
    setCursor(Qt::CursorShape::WaitCursor);

    // Sort here by member
    int i = mParser->findLastOperator(phrase);
    while ((i>=0) && (i<phrase.length()) && (
               phrase[i] == '.'
               || phrase[i] == ':'
               || phrase[i] == '-'
               || phrase[i] == '>'))
        i++;

    QString symbol = phrase.mid(i);
    // filter fFullCompletionStatementList to fCompletionStatementList
    filterList(symbol);
    mModel->notifyUpdated();
    setCursor(oldCursor);

    if (!mCompletionStatementList.isEmpty()) {
        mListView->setCurrentIndex(mModel->index(0,0));
        // if only one suggestion, and is exactly the symbol to search, hide the frame (the search is over)
        // if only one suggestion and auto hide , don't show the frame
        if(mCompletionStatementList.count() == 1)
            if (autoHideOnSingleResult
                    || (symbol == mCompletionStatementList.front()->command)) {
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

void CodeCompletionPopup::addChildren(PStatement scopeStatement, const QString &fileName, int line)
{
    if (scopeStatement && !isIncluded(scopeStatement->fileName)
      && !isIncluded(scopeStatement->definitionFileName))
        return;
    const StatementMap& children = mParser->statementList().childrenStatements(scopeStatement);
    if (children.isEmpty())
        return;

    if (!scopeStatement) { //Global scope
        for (const PStatement& childStatement: children) {
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
        for (const PStatement& childStatement: children) {
            if (!( childStatement->kind == StatementKind::skConstructor
                                      || childStatement->kind == StatementKind::skDestructor
                                      || childStatement->kind == StatementKind::skBlock)
                                   && (!mAddedStatements.contains(childStatement->command)))
                addStatement(childStatement,fileName,line);
        }
    }
}

void CodeCompletionPopup::addStatement(PStatement statement, const QString &fileName, int line)
{
    if (mAddedStatements.contains(statement->command))
        return;
    if ((line!=-1)
            && (line < statement->line)
            && (fileName == statement->fileName))
        return;
    mAddedStatements.insert(statement->command);
    mFullCompletionStatementList.append(statement);
}

static bool nameComparator(PStatement statement1,PStatement statement2) {
    if (statement1->caseMatch && !statement2->caseMatch) {
        return true;
    } else if (!statement1->caseMatch && statement2->caseMatch) {
        return false;
    } else
        return statement1->command < statement2->command;
}

static bool defaultComparator(PStatement statement1,PStatement statement2) {
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
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeSnippet) {
        if (statement2->kind != StatementKind::skUserCodeSnippet)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeSnippet) {
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
    } else if (!(statement1->inSystemHeader) && statement2->inSystemHeader) {
        return true;
    } else if (statement1->inSystemHeader && !(statement2->inSystemHeader)) {
        return false;
        // Show local statements first
    } else if (statement1->scope != StatementScope::ssGlobal
               && statement2->scope == StatementScope::ssGlobal ) {
        return true;
    } else if (statement1->scope == StatementScope::ssGlobal
               && statement2->scope != StatementScope::ssGlobal ) {
        return false;
    } else
        return nameComparator(statement1,statement2);
}

static bool sortWithUsageComparator(PStatement statement1,PStatement statement2) {
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeSnippet) {
        if (statement2->kind != StatementKind::skUserCodeSnippet)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeSnippet) {
        return false;
        //show most freq first
    } else if (statement1->freqTop > statement2->freqTop) {
        return true;
    } else if (statement1->freqTop < statement2->freqTop) {
        return false;
        // show keywords first
    } else if ((statement1->kind != StatementKind::skKeyword)
               && (statement2->kind == StatementKind::skKeyword)) {
        return true;
    } else if ((statement1->kind == StatementKind::skKeyword)
               && (statement2->kind != StatementKind::skKeyword)) {
        return false;
    } else
        return nameComparator(statement1,statement2);
}

static bool sortByScopeWithUsageComparator(PStatement statement1,PStatement statement2){
    // Show user template first
    if (statement1->kind == StatementKind::skUserCodeSnippet) {
        if (statement2->kind != StatementKind::skUserCodeSnippet)
            return true;
        else
            return statement1->command < statement2->command;
    } else if (statement2->kind == StatementKind::skUserCodeSnippet) {
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
    } else if (statement1->inSystemHeader && ! (statement2->inSystemHeader)) {
        return true;
    } else if (!(statement1->inSystemHeader) && statement2->inSystemHeader) {
        return false;
        // Show local statements first
    } else if (statement1->scope != StatementScope::ssGlobal
               && statement2->scope == StatementScope::ssGlobal ) {
        return true;
    } else if (statement1->scope == StatementScope::ssGlobal
               && statement2->scope != StatementScope::ssGlobal ) {
        return false;
    } else
        return nameComparator(statement1,statement2);
}

void CodeCompletionPopup::filterList(const QString &member)
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

    mCompletionStatementList.clear();
    if (!member.isEmpty()) { // filter
        mCompletionStatementList.reserve(mFullCompletionStatementList.size());
        foreach (const PStatement& statement, mFullCompletionStatementList) {
            Qt::CaseSensitivity cs = (mIgnoreCase?
                                          Qt::CaseInsensitive:
                                          Qt::CaseSensitive);
            if (statement->command.startsWith(member, cs)) {
                if (mIgnoreCase) {
                    statement->caseMatch =
                            statement->command.startsWith(
                                member,Qt::CaseSensitive);
                } else {
                    statement->caseMatch = true;
                }
                mCompletionStatementList.append(statement);
            }
        }
    } else
        mCompletionStatementList.append(mFullCompletionStatementList);
    if (mRecordUsage) {
        int topCount = 0;
        int secondCount = 0;
        int thirdCount = 0;
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
        foreach (const PStatement& statement, mCompletionStatementList) {
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

void CodeCompletionPopup::getCompletionFor(const QString &fileName, const QString &phrase, int line)
{
    if(!mParser)
        return;
    if (!mParser->enabled())
        return;

    if (!mParser->freeze())
        return;
    {
        auto action = finally([this]{
            mParser->unFreeze();
        });

        //C++ preprocessor directives
        if (phrase.startsWith('#')) {
            if (mShowKeywords) {
                foreach (const QString& keyword, CppDirectives) {
                    PStatement statement = std::make_shared<Statement>();
                    statement->command = keyword;
                    statement->kind = StatementKind::skKeyword;
                    statement->fullName = keyword;
                    statement->usageCount = 0;
                    statement->freqTop = 0;
                    mFullCompletionStatementList.append(statement);
                }
            }
            return;
        }

        //docstring tags (javadoc style)
        if (phrase.startsWith('@')) {
            if (mShowKeywords) {
                foreach (const QString& keyword,JavadocTags) {
                    PStatement statement = std::make_shared<Statement>();
                    statement->command = keyword;
                    statement->kind = StatementKind::skKeyword;
                    statement->fullName = keyword;
                    statement->usageCount = 0;
                    statement->freqTop = 0;
                    mFullCompletionStatementList.append(statement);
                }
            }
            return;
        }


        // Pulling off the same trick as in TCppParser.FindStatementOf, but ignore everything after last operator
        int i = mParser->findLastOperator(phrase);
        if (i < 0 ) { // don't have any scope prefix

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
                        statement->freqTop = 0;
                        mFullCompletionStatementList.append(statement);
                    }
                }
            }

            if (mShowKeywords) {
                //add keywords
                if (mUseCppKeyword) {
                    foreach (const QString& keyword,CppKeywords.keys()) {
                        PStatement statement = std::make_shared<Statement>();
                        statement->command = keyword;
                        statement->kind = StatementKind::skKeyword;
                        statement->fullName = keyword;
                        statement->usageCount = 0;
                        statement->freqTop = 0;
                        mFullCompletionStatementList.append(statement);
                    }
                } else {
                    foreach (const QString& keyword,CKeywords) {
                        PStatement statement = std::make_shared<Statement>();
                        statement->command = keyword;
                        statement->kind = StatementKind::skKeyword;
                        statement->fullName = keyword;
                        statement->usageCount = 0;
                        statement->freqTop = 0;
                        mFullCompletionStatementList.append(statement);
                    }
                }
            }

            PStatement scopeStatement = mCurrentStatement;
            // repeat until reach global
            while (scopeStatement) {
                //add members of current scope that not added before
                if (scopeStatement->kind == StatementKind::skClass) {
                    addChildren(scopeStatement, fileName, -1);
                } else {
                    addChildren(scopeStatement, fileName, line);
                }

                // add members of all usings (in current scope ) and not added before
                foreach (const QString& namespaceName,scopeStatement->usingList) {
                    PStatementList namespaceStatementsList =
                            mParser->findNamespace(namespaceName);
                    if (!namespaceStatementsList)
                        continue;
                    foreach (const PStatement& namespaceStatement,*namespaceStatementsList) {
                        addChildren(namespaceStatement, fileName, line);
                    }
                }
                scopeStatement=scopeStatement->parentScope.lock();
            }

            // add all global members and not added before
            addChildren(nullptr, fileName, line);

            // add members of all fusings
            mUsings = mParser->getFileUsings(fileName);
            foreach (const QString& namespaceName, mUsings) {
                PStatementList namespaceStatementsList =
                        mParser->findNamespace(namespaceName);
                if (!namespaceStatementsList)
                    continue;
                foreach (const PStatement& namespaceStatement, *namespaceStatementsList) {
                    addChildren(namespaceStatement, fileName, line);
                }
            }
        } else { //we are in some statement's scope
            MemberOperatorType opType=getOperatorType(phrase,i);
            QString scopeName = phrase.mid(0,i);
            if (opType == MemberOperatorType::otDColon) {
                if (scopeName.isEmpty()) {
                    // start with '::', we only find in global
                    // add all global members and not added before
                    addChildren(nullptr, fileName, line);
                    return;
                } else {
                    //assume the scope its a namespace
                    PStatementList namespaceStatementsList =
                            mParser->findNamespace(scopeName);
                    if (namespaceStatementsList) {
                        foreach (const PStatement& namespaceStatement, *namespaceStatementsList) {
                            addChildren(namespaceStatement, fileName, line);
                        }
                        return;
                    }
                    //namespace not found let's go on
                }
            }
            PStatement parentTypeStatement;
            PStatement statement = mParser->findStatementOf(
                        fileName,
                        scopeName,
                        mCurrentStatement,
                        parentTypeStatement);

            if (!statement)
                return;
            // find the most inner scope statement that has a name (not a block)
            PStatement scopeTypeStatement = mCurrentStatement;
            while (scopeTypeStatement && !isScopeTypeKind(scopeTypeStatement->kind)) {
                scopeTypeStatement = scopeTypeStatement->parentScope.lock();
            }
            if (
                    (opType == MemberOperatorType::otArrow
                     ||  opType == MemberOperatorType::otDot)
                    && (
                        statement->kind == StatementKind::skVariable
                        || statement->kind == StatementKind::skParameter
                        || statement->kind == StatementKind::skFunction)
                    ) {
                // Get type statement  of current (scope) statement
                PStatement classTypeStatement;
                PStatement parentScope = statement->parentScope.lock();
                if ((statement->kind == StatementKind::skFunction)
                        && parentScope
                        && STLContainers.contains(parentScope->fullName)
                        && STLElementMethods.contains(statement->command)){
                    // it's an element method of STL container
                    // we must find the type in the template parameter

                    // get the function's owner variable's definition
                    int lastI = mParser->findLastOperator(scopeName);
                    QString lastScopeName = scopeName.mid(0,lastI);
                    PStatement lastScopeStatement =
                            mParser->findStatementOf(
                                fileName, lastScopeName,
                                mCurrentStatement,parentTypeStatement);
                    if (!lastScopeStatement)
                        return;


                    QString typeName =
                            mParser->findFirstTemplateParamOf(
                                fileName,lastScopeStatement->type,
                                lastScopeStatement->parentScope.lock());
                    classTypeStatement = mParser->findTypeDefinitionOf(
                                fileName, typeName,
                                lastScopeStatement->parentScope.lock());
                } else
                    classTypeStatement=mParser->findTypeDefinitionOf(
                                fileName, statement->type,parentTypeStatement);

                if (!classTypeStatement)
                    return;
                //is a smart pointer
                if (STLPointers.contains(classTypeStatement->fullName)
                   && (opType == MemberOperatorType::otArrow)) {
                    QString typeName= mParser->findFirstTemplateParamOf(
                                fileName,
                                statement->type,
                                parentScope);
                    classTypeStatement = mParser->findTypeDefinitionOf(
                                fileName,
                                typeName,
                                parentScope);
                    if (!classTypeStatement)
                        return;
                }
                //is a stl container operator[]
                if (STLContainers.contains(classTypeStatement->fullName)
                        && scopeName.endsWith(']')) {
                    QString typeName= mParser->findFirstTemplateParamOf(
                                fileName,
                                statement->type,
                                parentScope);
                    classTypeStatement = mParser->findTypeDefinitionOf(
                                fileName,
                                typeName,
                                parentScope);
                    if (!classTypeStatement)
                        return;
                }
                if (!isIncluded(classTypeStatement->fileName) &&
                    !isIncluded(classTypeStatement->definitionFileName))
                    return;
                if ((classTypeStatement == scopeTypeStatement) || (statement->command == "this")) {
                    //we can use all members
                    addChildren(classTypeStatement,fileName,-1);
                } else { // we can only use public members
                    const StatementMap& children = mParser->statementList().childrenStatements(classTypeStatement);
                    if (children.isEmpty())
                        return;
                    foreach (const PStatement& childStatement, children) {
                        if ((childStatement->classScope==StatementClassScope::scsPublic)
                                && !(
                                    childStatement->kind == StatementKind::skConstructor
                                    || childStatement->kind == StatementKind::skDestructor)
                                && !mAddedStatements.contains(childStatement->command)) {
                            addStatement(childStatement,fileName,-1);
                        }
                    }
                }
            //todo friend
            } else if ((opType == MemberOperatorType::otDColon)
                       && ((statement->kind == StatementKind::skEnumType)
                       || (statement->kind == StatementKind::skEnumClassType))) {
                //we can add all child enum definess
                PStatement classTypeStatement = statement;
                if (!isIncluded(classTypeStatement->fileName) &&
                    !isIncluded(classTypeStatement->definitionFileName))
                    return;
                const StatementMap& children =
                        mParser->statementList().childrenStatements(classTypeStatement);
                foreach (const PStatement& child,children) {
                    addStatement(child,fileName,line);
                }
            } else if ((opType == MemberOperatorType::otDColon)
                       && (statement->kind == StatementKind::skClass)) {
                PStatement classTypeStatement = statement;
                if (!isIncluded(classTypeStatement->fileName) &&
                    !isIncluded(classTypeStatement->definitionFileName))
                    return;
                if (classTypeStatement == scopeTypeStatement) {
                    //we can use all static members
                    const StatementMap& children =
                            mParser->statementList().childrenStatements(classTypeStatement);
                    foreach (const PStatement& childStatement, children) {
                        if (
                          (childStatement->isStatic)
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
                          (childStatement->isStatic)
                           || (childStatement->kind == StatementKind::skTypedef
                            || childStatement->kind == StatementKind::skClass
                            || childStatement->kind == StatementKind::skEnum
                            || childStatement->kind == StatementKind::skEnumClassType
                            || childStatement->kind == StatementKind::skEnumType
                               )) {
                            if (childStatement->classScope == StatementClassScope::scsPublic)
                                addStatement(childStatement,fileName,-1);
                        }
                    }
                }
              //todo friend
            }
        }
    }
}

bool CodeCompletionPopup::isIncluded(const QString &fileName)
{
    return mIncludedFiles.contains(fileName);
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

const QString &CodeCompletionPopup::phrase() const
{
    return mPhrase;
}

void CodeCompletionPopup::showEvent(QShowEvent *)
{
    mListView->setFocus();
}

const PStatement &CodeCompletionPopup::currentStatement() const
{
    return mCurrentStatement;
}

void CodeCompletionPopup::setCurrentStatement(const PStatement &newCurrentStatement)
{
    mCurrentStatement = newCurrentStatement;
}

const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > >& CodeCompletionPopup::colors() const
{
    return mColors;
}

bool CodeCompletionPopup::useCppKeyword() const
{
    return mUseCppKeyword;
}

void CodeCompletionPopup::setUseCppKeyword(bool newUseCppKeyword)
{
    mUseCppKeyword = newUseCppKeyword;
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

bool CodeCompletionPopup::onlyGlobals() const
{
    return mOnlyGlobals;
}

void CodeCompletionPopup::setOnlyGlobals(bool newOnlyGlobals)
{
    mOnlyGlobals = newOnlyGlobals;
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
    mFullCompletionStatementList.clear();
    mIncludedFiles.clear();
    mUsings.clear();
    mAddedStatements.clear();
    QWidget::hideEvent(event);
}

bool CodeCompletionPopup::event(QEvent *event)
{
    bool result = QWidget::event(event);
    if (event->type() == QEvent::FontChange) {
          mListView->setFont(font());
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
    case Qt::ForegroundRole: {
        PStatement statement = mStatements->at(index.row());
        if (mColorCallback)
            return mColorCallback(statement);
        return qApp->palette().color(QPalette::Text);
    }
    }
    return QVariant();
}

void CodeCompletionListModel::notifyUpdated()
{
    beginResetModel();
    endResetModel();
}

const ColorCallback &CodeCompletionListModel::colorCallback() const
{
    return mColorCallback;
}

void CodeCompletionListModel::setColorCallback(const ColorCallback &newColorCallback)
{
    mColorCallback = newColorCallback;
}
