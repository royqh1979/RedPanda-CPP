/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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

static bool isAnonymousType(const PStatement &statement)
{
    return statement
            && statement->command.startsWith("__RED_PANDA_IDE__")
            && (statement->kind == StatementKind::Class
                || statement->kind == StatementKind::EnumType
                || statement->kind == StatementKind::EnumClassType);
}

ClassBrowserModel::ClassBrowserModel(QObject *parent):QAbstractItemModel(parent),
    mMutex()
{
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
            if (isAnonymousType(node->statement))
                return tr("anonymous");
            if (node->statement->kind == StatementKind::Namespace
                    || node->statement->kind == StatementKind::InlineAlias)
                return node->statement->command;
            if (!(statementType(node->statement).isEmpty())) {
                if ( (node->statement->kind == StatementKind::Variable)
                     || (node->statement->kind == StatementKind::Typedef) ) {
                    if (std::dynamic_pointer_cast<Statement>(node->statement)->args.isEmpty())
                        return node->statement->command + " : " + statementType(node->statement);
                    else if (std::dynamic_pointer_cast<Statement>(node->statement)->args.startsWith("("))
                        return QString("%1 : %2 (*)%3").arg(node->statement->command,
                                                            statementType(node->statement),
                                                            std::dynamic_pointer_cast<Statement>(node->statement)->args);
                    else
                        return node->statement->command + " : " + statementType(node->statement) + std::dynamic_pointer_cast<Statement>(node->statement)->args;
                } else if (node->statement->kind == StatementKind::Function) {
                    return node->statement->command + std::dynamic_pointer_cast<Statement>(node->statement)->args + " : " + statementType(node->statement);
                } else if (node->statement->kind == StatementKind::OverloadedOperator) {
                    if (!CppParser::isIdentifier(node->statement->command)) {
                        return "operator"+node->statement->command + std::dynamic_pointer_cast<Statement>(node->statement)->args + " : " + statementType(node->statement);
                    } else if (node->statement->command == "new"
                               || node->statement->command == "delete") {
                        return "operator "+node->statement->command + std::dynamic_pointer_cast<Statement>(node->statement)->args + " : " + statementType(node->statement);
                    } else {
                        return "operator "+node->statement->command + std::dynamic_pointer_cast<Statement>(node->statement)->args + " : " + statementType(node->statement) + " " + node->statement->command;
                    }
                } else if (node->statement->kind == StatementKind::LiteralOperator) {
                    return "operator \"\""+node->statement->command + std::dynamic_pointer_cast<Statement>(node->statement)->args + " : " + statementType(node->statement);
                }
            }
            if (node->statement->kind == StatementKind::Enum) {
                if (!std::dynamic_pointer_cast<Statement>(node->statement)->value.isEmpty())
                    return node->statement->command + std::dynamic_pointer_cast<Statement>(node->statement)->args + QString("(%1)").arg(std::dynamic_pointer_cast<Statement>(node->statement)->value);
                else
                    return node->statement->command;
            }
            if (node->statement->command == "{") {
                return "anonymous";
            }
            return node->statement->command + std::dynamic_pointer_cast<Statement>(node->statement)->args;
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
            return pMainWindow->iconsManager()->getPixmapForStatement(node->statement);
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
                   &CppParser::parseFinished,
                   this,
                   &ClassBrowserModel::fillStatements);
        disconnect(mParser.get(),
                   &CppParser::parseStarted,
                   this,
                   &ClassBrowserModel::refreshStarted);
    }
    mParser = newCppParser;
    if (mParser) {
        connect(mParser.get(),
                   &CppParser::parseFinished,
                   this,
                   &ClassBrowserModel::fillStatements);
        connect(mParser.get(),
                   &CppParser::parseStarted,
                   this,
                   &ClassBrowserModel::refreshStarted);
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
    mTopLevelStatements.clear();
    mNestedStatements.clear();
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
    {
        auto action = finally([this]{
            mUpdating = false;
            emit refreshEnd();
        });
        if (!mParser || !mParser->enabled()) {
            clear();
            return;
        }
        if (!mParser->freeze())
            return;
        buildStatementTree();
        mParser->unFreeze();
    }
}

QString ClassBrowserModel::generateStatementKey(const PStatement& statement) const
{
    QString normalizedArgs;
    if (statement->kind != StatementKind::Namespace
            && statement->kind != StatementKind::InlineAlias)
        normalizedArgs = std::dynamic_pointer_cast<Statement>(statement)->normalizedArgs;

    return QString("%1+%2+%3")
                .arg(statement->fullName, normalizedArgs)
                .arg((int)statement->kind);
}

PClassBrowserNode ClassBrowserModel::addChild(ClassBrowserNode *node, const PStatement& statement)
{
    PClassBrowserNode newNode = std::make_shared<ClassBrowserNode>();
    newNode->parent = node;
    newNode->statement = statement;
    node->children.append(newNode.get());
    mNodes.append(newNode);
    mNodeIndex.insert(generateStatementKey(statement),newNode);
    return newNode;
}

void ClassBrowserModel::removeNode(ClassBrowserNode *node)
{
    const QVector<ClassBrowserNode *> children = node->children;
    for (ClassBrowserNode *child : children)
        removeNode(child);
    mNodeIndex.remove(generateStatementKey(node->statement));
    mNodes.removeOne(node->shared_from_this());
}

void ClassBrowserModel::buildStatementTree()
{
    mTopLevelStatements.clear();
    mNestedStatements.clear();
    QList<PParsedFileInfo> fileInfos;
    PParsedFileInfo current = mParser->findFileInfo(mCurrentFile);
    if (current)
        fileInfos.append(current);

    for (const PParsedFileInfo &fileInfo : fileInfos) {
        for (const std::weak_ptr<AbstractStatement> &weakStatement : fileInfo->statements()) {
            PStatement statement = weakStatement.lock();
            while (statement) {
                QSet<PStatement> &statements = statement->parentScope.lock()
                        ? mNestedStatements : mTopLevelStatements;
                if (statements.contains(statement))
                    break;
                statements.insert(statement);
                statement = statement->parentScope.lock();
            }
        }
    }
    updateChildren(mRoot, mTopLevelStatements.values());
    emit layoutAboutToBeChanged();
    const QList<QModelIndex> oldIndexes = persistentIndexList();
    sortNode(mRoot);
    for (const QModelIndex &oldIndex : oldIndexes) {
        if (oldIndex.isValid()) {
            auto node = static_cast<ClassBrowserNode *>(oldIndex.internalPointer());
            changePersistentIndex(oldIndex, indexForNode(node));
        }
    }
    emit layoutChanged();
}

void ClassBrowserModel::sortNode(ClassBrowserNode *node)
{
    const bool sortAlpha = pSettings->ui().classBrowserSortAlpha();
    const bool sortType = pSettings->ui().classBrowserSortType();
    std::sort(node->children.begin(),node->children.end(),
              [this, sortAlpha, sortType](ClassBrowserNode* node1,ClassBrowserNode* node2) {
        if (sortType && node1->statement->kind != node2->statement->kind)
            return node1->statement->kind < node2->statement->kind;
        if (sortType) {
            bool firstAnonymous = isAnonymousType(node1->statement);
            bool secondAnonymous = isAnonymousType(node2->statement);
            if (firstAnonymous != secondAnonymous)
                return firstAnonymous;
            if (firstAnonymous) {
                StatementOccurrence first = statementDefinitionForNavigation(node1->statement);
                StatementOccurrence second = statementDefinitionForNavigation(node2->statement);
                return first.range.start.line < second.range.start.line;
            }
        }
        if (sortAlpha)
            return node1->statement->command.toLower() < node2->statement->command.toLower();
        if (sortType)
            return false;
        StatementOccurrence first = statementDeclarationForNavigation(
                    node1->statement, mCurrentFile);
        StatementOccurrence second = statementDeclarationForNavigation(
                    node2->statement, mCurrentFile);
        return first.range.start.line < second.range.start.line;
    });
    foreach(ClassBrowserNode* child,node->children) {
        sortNode(child);
    }
}

bool ClassBrowserModel::shouldShowStatement(ClassBrowserNode *node, const PStatement &statement) const
{
    if (!statement)
        return false;
    PStatement parentScope = statement->parentScope.lock();
    const QSet<PStatement> &statementsToShow = parentScope
            ? mNestedStatements : mTopLevelStatements;
    if (!statementsToShow.contains(statement))
        return false;
    if (statement->kind == StatementKind::Enum
            && (!parentScope
                || (parentScope->kind != StatementKind::EnumType
                    && parentScope->kind != StatementKind::EnumClassType)))
        return false;
    if (statement->kind == StatementKind::Block
            || statement->kind == StatementKind::Lambda
            || statement == node->statement
            || statement->scope == StatementScope::Local)
        return false;

    StatementOccurrence declaration = statementDeclarationForNavigation(statement, mCurrentFile);
    StatementOccurrence definition = statementDefinitionForNavigation(statement);
    if (pSettings->codeCompletion().hideSymbolsStartsWithTwoUnderLine()
            && statement->command.startsWith("__")
            && declaration.fileName != mCurrentFile
            && definition.fileName != mCurrentFile)
        return false;
    if (pSettings->codeCompletion().hideSymbolsStartsWithUnderLine()
            && statement->kind != StatementKind::LiteralOperator
            && statement->command.startsWith('_')
            && declaration.fileName != mCurrentFile
            && definition.fileName != mCurrentFile)
        return false;

    return parentScope == node->statement
            || (parentScope && node->statement
                && parentScope->fullName == node->statement->fullName);
}

void ClassBrowserModel::updateChildren(ClassBrowserNode *node, const StatementList &statements)
{
    QStringList orderedKeys;
    QHash<QString, PStatement> statementsByKey;
    for (const PStatement &statement : statements) {
        if (!shouldShowStatement(node, statement))
            continue;
        QString key = generateStatementKey(statement);
        if (!statementsByKey.contains(key))
            orderedKeys.append(key);
        statementsByKey.insert(key, statement);
    }

    QHash<QString, PClassBrowserNode> nodesByKey;
    for (ClassBrowserNode *child : node->children)
        nodesByKey.insert(generateStatementKey(child->statement), child->shared_from_this());

    QModelIndex parentIndex = indexForNode(node);
    for (int row = node->children.count() - 1; row >= 0; --row) {
        PClassBrowserNode child = node->children[row]->shared_from_this();
        if (statementsByKey.contains(generateStatementKey(child->statement)))
            continue;
        beginRemoveRows(parentIndex, row, row);
        node->children.remove(row, 1);
        removeNode(child.get());
        endRemoveRows();
    }

    for (const QString &key : orderedKeys) {
        PStatement statement = statementsByKey.value(key);
        PClassBrowserNode child = nodesByKey.value(key);
        if (child) {
            int row = node->children.indexOf(child.get());
            QModelIndex childIndex = index(row, 0, parentIndex);
            child->statement = statement;
            mNodeIndex.insert(key, child);
            emit dataChanged(childIndex, childIndex);
        } else {
            int row = node->children.count();
            beginInsertRows(parentIndex, row, row);
            child = addChild(node, statement);
            endInsertRows();
        }
        updateChildren(child.get(), mParser->membersOfStatement(statement));
    }
}

QModelIndex ClassBrowserModel::indexForNode(ClassBrowserNode *node) const
{
    if (!node || node == mRoot)
        return QModelIndex();
    ClassBrowserNode *parentNode = node->parent;
    if (!parentNode)
        return QModelIndex();
    int row = parentNode->children.indexOf(node);
    if (row < 0)
        return QModelIndex();
    return createIndex(row, 0, node);
}

QModelIndex ClassBrowserModel::modelIndexForStatement(const QString &key) const
{
    QMutexLocker locker(&mMutex);
    if (mUpdating)
        return QModelIndex();
    return indexForNode(mNodeIndex.value(key).get());
}

QModelIndex ClassBrowserModel::modelIndexForStatement(const PStatement &statement) const
{
    if (!statement)
        return QModelIndex();
    QModelIndex index = modelIndexForStatement(generateStatementKey(statement));
    if (index.isValid() || statement->kind != StatementKind::Enum || !mParser)
        return index;
    StatementOccurrence definition = statementDefinitionForNavigation(statement);
    auto normal = std::dynamic_pointer_cast<Statement>(statement);
    if (!definition.range.start.isValid() || !normal)
        return QModelIndex();
    PStatement enumType = mParser->findStatement(normal->typeInfo.qualifiedName);
    if (!enumType)
        return QModelIndex();
    for (const PStatement &member : mParser->membersOfStatement(enumType)) {
        if (member->kind == StatementKind::Enum
                && member->definition.fileName == definition.fileName
                && member->definition.range == definition.range)
            return modelIndexForStatement(generateStatementKey(member));
    }
    return QModelIndex();
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
    if (mCurrentFile == newCurrentFile)
        return;
    clear();
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
