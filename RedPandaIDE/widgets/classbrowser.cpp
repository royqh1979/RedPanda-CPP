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
#include "classbrowser.h"
#include "../utils.h"
#include <QDebug>
#include <QColor>
#include <QPalette>
#include "../mainwindow.h"
#include "../settings.h"
#include "../colorscheme.h"
#include "../utils.h"
#include "../iconsmanager.h"

ClassBrowserModel::ClassBrowserModel(QObject *parent):QAbstractItemModel(parent),
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    mMutex()
#else
    mMutex(QMutex::Recursive)
#endif
{
    mClassBrowserType = ProjectClassBrowserType::CurrentFile;
    mRoot = new ClassBrowserNode();
    mRoot->parent = nullptr;
    mRoot->statement = PStatement();
//    mRoot->childrenFetched = true;
    mUpdating = false;
    mUpdateCount = 0;
}

ClassBrowserModel::~ClassBrowserModel()
{
    delete mRoot;
}

QModelIndex ClassBrowserModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row,column,parent))
        return QModelIndex();

    ClassBrowserNode *parentNode;
    if (!parent.isValid()) { // top level
        parentNode = mRoot;
    } else {
        parentNode = static_cast<ClassBrowserNode *>(parent.internalPointer());
    }
    return createIndex(row,column,parentNode->children[row]);
}

QModelIndex ClassBrowserModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }
    ClassBrowserNode *childNode = static_cast<ClassBrowserNode *>(child.internalPointer());
    ClassBrowserNode *parentNode = childNode->parent;
    if (parentNode->parent == nullptr) //it's root node
        return QModelIndex();

    ClassBrowserNode *grandNode = parentNode->parent;
    int row = grandNode->children.indexOf(parentNode);
    return createIndex(row,0,parentNode);
}

bool ClassBrowserModel::hasChildren(const QModelIndex &parent) const
{
    ClassBrowserNode *parentNode;
    if (!parent.isValid()) { // top level
        return mRoot->children.count()>0;
    } else {
        parentNode = static_cast<ClassBrowserNode *>(parent.internalPointer());
//        if (parentNode->childrenFetched)
        return parentNode->children.count()>0;
//        if (parentNode->statement)
//            return !parentNode->statement->children.isEmpty();
//        return false;
    }
}

int ClassBrowserModel::rowCount(const QModelIndex &parent) const
{
    ClassBrowserNode *parentNode;
    if (!parent.isValid()) { // top level
        parentNode = mRoot;
    } else {
        parentNode = static_cast<ClassBrowserNode *>(parent.internalPointer());
    }
    return parentNode->children.count();
}

int ClassBrowserModel::columnCount(const QModelIndex&) const
{
    return 1;
}

//void ClassBrowserModel::fetchMore(const QModelIndex &parent)
//{
//    if (!parent.isValid()) { // top level
//        return;
//    }

//    ClassBrowserNode *parentNode = static_cast<ClassBrowserNode *>(parent.internalPointer());
//    if (!parentNode->childrenFetched) {
//        parentNode->childrenFetched = true;
//        if (parentNode->statement && !parentNode->statement->children.isEmpty()) {
//            filterChildren(parentNode, parentNode->statement->children);
//            beginInsertRows(parent,0,parentNode->children.count());
//            endInsertRows();
//        }
//    }
//}

//bool ClassBrowserModel::canFetchMore(const QModelIndex &parent) const
//{
//    if (!parent.isValid()) { // top level
//        return false;
//    }
//    ClassBrowserNode *parentNode = static_cast<ClassBrowserNode *>(parent.internalPointer());
//    if (!parentNode->childrenFetched) {
//        if (parentNode->statement && !parentNode->statement->children.isEmpty())
//            return true;
//        else
//            parentNode->childrenFetched = true;
//    }
//    return false;
//}

QVariant ClassBrowserModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()){
        return QVariant();
    }
    ClassBrowserNode *node = static_cast<ClassBrowserNode *>(index.internalPointer());
    if (!node)
        return QVariant();
    if (role == Qt::DisplayRole) {
        if (node->statement) {
            if (!(node->statement->type.isEmpty())) {
                if ((node->statement->kind == StatementKind::Function)
                     || (node->statement->kind == StatementKind::Variable)
                     || (node->statement->kind == StatementKind::Typedef)
                     ) {
                    return node->statement->command + node->statement->args + " : " + node->statement->type;
                }
            }
            if (node->statement->kind == StatementKind::Enum) {
                if (!node->statement->value.isEmpty())
                    return node->statement->command + node->statement->args + QString("(%1)").arg(node->statement->value);
                else
                    return node->statement->command;
            }
            return node->statement->command + node->statement->args;
        }
    } else if (role == Qt::ForegroundRole) {
        if (mColors && node->statement) {
            PStatement statement = (node->statement);
            StatementKind kind = getKindOfStatement(statement);
            if (kind == StatementKind::Keyword) {
                if (statement->command.startsWith('#'))
                    kind = StatementKind::Preprocessor;
            }
            PColorSchemeItem item = mColors->value(kind,PColorSchemeItem());
            if (item) {
                return item->foreground();
            } else {
                return pMainWindow->palette().color(QPalette::Text);
            }
        }
        return pMainWindow->palette().color(QPalette::Text);
    } else if (role == Qt::DecorationRole) {
        if (node->statement) {
            return pIconsManager->getPixmapForStatement(node->statement);
        }
    }
    return QVariant();
}

const PCppParser &ClassBrowserModel::parser() const
{
    return mParser;
}

void ClassBrowserModel::setParser(const PCppParser &newCppParser)
{
    if (mParser) {
        disconnect(mParser.get(),
                   &CppParser::onEndParsing,
                   this,
                   &ClassBrowserModel::fillStatements);
    }
    mParser = newCppParser;
    if (mParser) {
        connect(mParser.get(),
                   &CppParser::onEndParsing,
                   this,
                   &ClassBrowserModel::fillStatements);
    } else {
        clear();
    }
}

void ClassBrowserModel::clear()
{
    beginResetModel();
    mRoot->children.clear();
    mNodes.clear();
    mNodeIndex.clear();
    mProcessedStatements.clear();
    mDummyStatements.clear();
    mScopeNodes.clear();
    endResetModel();
}

void ClassBrowserModel::fillStatements()
{
    {
        QMutexLocker locker(&mMutex);
        if (mUpdateCount!=0 || mUpdating)
            return;
        mUpdating = true;
    }
    emit refreshStarted();
    beginResetModel();
    clear();
    {
        auto action = finally([this]{
            endResetModel();
            mUpdating = false;
            emit refreshEnd();
        });
        if (!mParser)
            return;
        if (!mParser->enabled())
            return;
        if (!mParser->freeze())
            return;
        QString mParserSerialId = mParser->serialId();
        addMembers();
        mParser->unFreeze();
    }
}

PClassBrowserNode ClassBrowserModel::addChild(ClassBrowserNode *node, const PStatement& statement)
{
    PClassBrowserNode newNode = std::make_shared<ClassBrowserNode>();
    newNode->parent = node;
    newNode->statement = statement;
//    newNode->childrenFetched = false;
    node->children.append(newNode.get());
    mNodes.append(newNode);
    mNodeIndex.insert(
                QString("%1+%2+%3")
                .arg(statement->fullName)
                .arg(statement->noNameArgs)
                .arg((int)statement->kind),newNode);
    mProcessedStatements.insert(statement.get());
    if (isScopeStatement(statement)) {
        mScopeNodes.insert(statement->fullName,newNode);
    }
    //don't show enum type's children values (they are displayed in parent scope)
//    if (statement->kind != StatementKind::skEnumType) {
        filterChildren(newNode.get(), statement->children);
//    }
    return newNode;
}

void ClassBrowserModel::addMembers()
{
    if (mClassBrowserType==ProjectClassBrowserType::CurrentFile) {
        if (mCurrentFile.isEmpty())
            return;
        // show statements in the file
        PFileIncludes p = mParser->findFileIncludes(mCurrentFile);
        if (!p)
            return;
        filterChildren(mRoot,p->statements);
    } else {
        if (mParser->projectFiles().isEmpty())
            return;
        foreach(const QString& file,mParser->projectFiles()) {
            PFileIncludes p = mParser->findFileIncludes(file);
            if (!p)
                return;
            filterChildren(mRoot,p->statements);
        }
    }
    sortNode(mRoot);
}

void ClassBrowserModel::sortNode(ClassBrowserNode *node)
{
    if (!pSettings->ui().classBrowserSortAlpha()) {
        if (mClassBrowserType==ProjectClassBrowserType::CurrentFile) {
            std::sort(node->children.begin(),node->children.end(),
                      [](ClassBrowserNode* node1,ClassBrowserNode* node2) {
                return (node1->statement->line < node2->statement->line);
            });
        } else {
            std::sort(node->children.begin(),node->children.end(),
                      [](ClassBrowserNode* node1,ClassBrowserNode* node2) {
                int comp=QString::compare(node1->statement->fileName, node2->statement->fileName);
                if (comp<0)
                    return true;
                else if (comp==0)
                    return (node1->statement->line < node2->statement->line);
                return false;
            });
        };
    }

    if (pSettings->ui().classBrowserSortAlpha()
            && pSettings->ui().classBrowserSortType()) {
        std::sort(node->children.begin(),node->children.end(),
                  [](ClassBrowserNode* node1,ClassBrowserNode* node2) {
            if (node1->statement->kind < node2->statement->kind) {
                return true;
            } else if (node1->statement->kind == node2->statement->kind) {
                return node1->statement->command.toLower() < node2->statement->command.toLower();
            } else {
                return false;
            }
        });
    } else if (pSettings->ui().classBrowserSortAlpha()) {
        std::sort(node->children.begin(),node->children.end(),
                  [](ClassBrowserNode* node1,ClassBrowserNode* node2) {
            return node1->statement->command.toLower() < node2->statement->command.toLower();
        });
    } else if (pSettings->ui().classBrowserSortType()) {
        std::sort(node->children.begin(),node->children.end(),
                  [](ClassBrowserNode* node1,ClassBrowserNode* node2) {
            return node1->statement->kind < node2->statement->kind;
        });
    }
    foreach(ClassBrowserNode* child,node->children) {
        sortNode(child);
    }
}

void ClassBrowserModel::filterChildren(ClassBrowserNode *node, const StatementMap &statements)
{
    for (PStatement statement:statements) {
        if (mClassBrowserType==ProjectClassBrowserType::WholeProject
                && !statement->inProject())
            continue;

        if (mProcessedStatements.contains(statement.get()))
            continue;
//        if (statement->properties.testFlag(StatementProperty::spDummyStatement))
//            continue;

        if (statement->kind == StatementKind::Block)
            continue;
        if (statement->kind == StatementKind::Lambda)
            continue;
        if (statement->isInherited() && !pSettings->ui().classBrowserShowInherited())
            continue;

        if (statement == node->statement) // prevent infinite recursion
            continue;

        if (statement->scope == StatementScope::Local)
            continue;

        if (pSettings->codeCompletion().hideSymbolsStartsWithTwoUnderLine()
                && statement->command.startsWith("__"))
            continue;

        if (pSettings->codeCompletion().hideSymbolsStartsWithUnderLine()
                && statement->command.startsWith('_'))
            continue;

        ClassBrowserNode *parentNode=node;
        // we only test and handle orphan statements in the top level (node->statement is null)
        PStatement parentScope = statement->parentScope.lock();
        if ( (parentScope!=node->statement)
                && (!parentScope || !node->statement
                    || parentScope->fullName!=node->statement->fullName)) {
//          //should not happend, just in case of error
//            if (!parentScope)
//                continue;

            // Processing the orphan statement
                //the statement's parent is in this file, so it's not a real orphan
//            if ((parentScope->fileName==mCurrentFile)
//                    ||(parentScope->definitionFileName==mCurrentFile))
//                continue;

            ClassBrowserNode *dummyNode = getParentNode(parentScope,1);
            if (dummyNode)
                parentNode = dummyNode;
        }
        if (isScopeStatement(statement)) {
            //PStatement dummy = mDummyStatements.value(statement->fullName,PStatement());
            PClassBrowserNode scopeNode = mScopeNodes.value(statement->fullName,PClassBrowserNode());
            if (!scopeNode) {
                PStatement dummy = createDummy(statement);
                scopeNode = addChild(parentNode,dummy);
            }
            filterChildren(scopeNode.get(),statement->children);
        } else {
            addChild(parentNode,statement);
        }
    }
}

PStatement ClassBrowserModel::createDummy(const PStatement& statement)
{
    PStatement result = std::make_shared<Statement>();
    result->parentScope = statement->parentScope;
    result->command = statement->command;
    result->args = statement->args;
    result->noNameArgs = statement->noNameArgs;
    result->fullName = statement->fullName;
    result->kind = statement->kind;
    result->type = statement->type;
    result->value = statement->value;
    result->scope = statement->scope;
    result->accessibility = statement->accessibility;
    result->properties = statement->properties;
    result->fileName= statement->fileName;
    result->line = statement->line;
    result->definitionFileName = statement->fileName;
    result->definitionLine = statement->definitionLine;
    mDummyStatements.insert(result->fullName,result);
    return result;
}

ClassBrowserNode* ClassBrowserModel::getParentNode(const PStatement &parentStatement, int depth)
{
    Q_ASSERT(depth<=10);
    if (depth>10) return mRoot;
    if (!parentStatement) return mRoot;
    if (!isScopeStatement(parentStatement)) return mRoot;

    PClassBrowserNode parentNode = mScopeNodes.value(parentStatement->fullName,PClassBrowserNode());
    if (!parentNode) {
        PStatement dummyParent = createDummy(parentStatement);
        ClassBrowserNode *grandNode = getParentNode(parentStatement->parentScope.lock(), depth+1);
        parentNode = addChild(grandNode,dummyParent);
    }
    return parentNode.get();
}

bool ClassBrowserModel::isScopeStatement(const PStatement &statement)
{
    switch(statement->kind) {
    case StatementKind::Class:
    case StatementKind::Namespace:
    case StatementKind::EnumClassType:
    case StatementKind::EnumType:
        return true;
    default:
        return false;
    }
}

QModelIndex ClassBrowserModel::modelIndexForStatement(const QString &key)
{
    QMutexLocker locker(&mMutex);
    if (mUpdating)
        return QModelIndex();
    PClassBrowserNode node=mNodeIndex.value(key,PClassBrowserNode());
    if (!node)
        return QModelIndex();

    ClassBrowserNode *parentNode=node->parent;
    if (!parentNode)
        return QModelIndex();
    int row=parentNode->children.indexOf(node.get());
    if (row<0)
        return QModelIndex();
    return createIndex(row,0,node.get());
}

ProjectClassBrowserType ClassBrowserModel::classBrowserType() const
{
    return mClassBrowserType;
}

void ClassBrowserModel::setClassBrowserType(ProjectClassBrowserType newClassBrowserType)
{
    if (mClassBrowserType != newClassBrowserType) {
        beginUpdate();
        mClassBrowserType = newClassBrowserType;
        endUpdate();
    }
}

const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &ClassBrowserModel::colors() const
{
    return mColors;
}

void ClassBrowserModel::setColors(const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &newColors)
{
    mColors = newColors;
}

const QString &ClassBrowserModel::currentFile() const
{
    return mCurrentFile;
}

void ClassBrowserModel::setCurrentFile(const QString &newCurrentFile)
{
    mCurrentFile = newCurrentFile;
}

void ClassBrowserModel::beginUpdate()
{
    mUpdateCount++;
}

void ClassBrowserModel::endUpdate()
{
    mUpdateCount--;
    if (mUpdateCount == 0) {
        if (mParser && !mParser->parsing()) {
            fillStatements();
        }
    }
}
