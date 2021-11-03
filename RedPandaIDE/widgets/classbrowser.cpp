#include "classbrowser.h"
#include "../utils.h"
#include <QDebug>
#include <QColor>
#include <QPalette>
#include "../mainwindow.h"
#include "../settings.h"
#include "../colorscheme.h"
#include "../utils.h"

ClassBrowserModel::ClassBrowserModel(QObject *parent):QAbstractItemModel(parent)
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
            return node->statement->command + node->statement->args;
        }
    } else if (role == Qt::ForegroundRole) {
        if (mColors && node->statement) {
            PStatement statement = (node->statement);
            StatementKind kind;
            if (mParser) {
                kind = mParser->getKindOfStatement(statement);
            } else {
                kind = statement->kind;
            }
            if (kind == StatementKind::skKeyword) {
                if (statement->command.startsWith('#'))
                    kind = StatementKind::skPreprocessor;
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
            PStatement statement = (node->statement);
            StatementKind kind;
            if (mParser) {
                kind = mParser->getKindOfStatement(statement);
            } else {
                kind = statement->kind;
            }
            switch (kind) {
            case StatementKind::skTypedef:
                return QIcon(":/icons/images/classparser/type.ico");
            case StatementKind::skClass:
                return QIcon(":/icons/images/classparser/class.ico");
            case StatementKind::skNamespace:
            case StatementKind::skNamespaceAlias:
                return QIcon(":/icons/images/classparser/namespace.ico");
            case StatementKind::skPreprocessor:
                return QIcon(":/icons/images/classparser/define.ico");
            case StatementKind::skEnumClassType:
            case StatementKind::skEnumType:
            case StatementKind::skEnum:
                return QIcon(":/icons/images/classparser/enum.ico");
            case StatementKind::skFunction:
            case StatementKind::skConstructor:
            case StatementKind::skDestructor:
                if (statement->scope == StatementScope::ssGlobal)
                    return QIcon(":/icons/images/classparser/global_method.ico");
                if (statement->isInherited) {
                    if (statement->classScope == StatementClassScope::scsProtected) {
                        return QIcon(":/icons/images/classparser/method_inherited_protected.ico");
                    } else if (statement->classScope == StatementClassScope::scsPublic) {
                        return QIcon(":/icons/images/classparser/method_inherited.ico");
                    }
                } else {
                    if (statement->classScope == StatementClassScope::scsProtected) {
                        return QIcon(":/icons/images/classparser/method_protected.ico");
                    } else if (statement->classScope == StatementClassScope::scsPublic) {
                        return QIcon(":/icons/images/classparser/method_public.ico");
                    } else {
                        return QIcon(":/icons/images/classparser/method_private.ico");
                    }
                }
                break;
            case StatementKind::skGlobalVariable:
                return QIcon(":/icons/images/classparser/global.ico");
            case StatementKind::skVariable:
//                if (statement->scope == StatementScope::ssGlobal)
//                    return QIcon(":/icons/images/classparser/global.ico");
                if (statement->isInherited) {
                    if (statement->classScope == StatementClassScope::scsProtected) {
                        return QIcon(":/icons/images/classparser/var_inherited_protected.ico");
                    } else if (statement->classScope == StatementClassScope::scsPublic) {
                        return QIcon(":/icons/images/classparser/var_inherited.ico");
                    }
                } else {
                    if (statement->classScope == StatementClassScope::scsProtected) {
                        return QIcon(":/icons/images/classparser/var_protected.ico");
                    } else if (statement->classScope == StatementClassScope::scsPublic) {
                        return QIcon(":/icons/images/classparser/var_public.ico");
                    } else {
                        return QIcon(":/icons/images/classparser/var_private.ico");
                    }
                }
                break;
            default:
                    break;
            }
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
    }
}

void ClassBrowserModel::clear()
{
    beginResetModel();
    mRoot->children.clear();
    mNodes.clear();
    mDummyStatements.clear();
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
    beginResetModel();
    clear();
    {
        auto action = finally([this]{
            endResetModel();
            mUpdating = false;
        });
        if (!mParser)
            return;
        if (!mParser->enabled())
            return;
        if (!mParser->freeze())
            return;
        {
            auto action2 = finally([this]{
                mParser->unFreeze();
            });
            QString mParserSerialId = mParser->serialId();
            if (!mCurrentFile.isEmpty()) {
                // QSet<QString> includedFiles = mParser->getFileIncludes(mCurrentFile);

                addMembers();
                // Remember selection
//                  if fLastSelection <> '' then
//                    ReSelect;
            }

        }
    }
}

void ClassBrowserModel::addChild(ClassBrowserNode *node, PStatement statement)
{
    PClassBrowserNode newNode = std::make_shared<ClassBrowserNode>();
    newNode->parent = node;
    newNode->statement = statement;
//    newNode->childrenFetched = false;
    node->children.append(newNode.get());
    mNodes.append(newNode);
    //don't show enum type's children values (they are displayed in parent scope)
    if (statement->kind != StatementKind::skEnumType)
        filterChildren(newNode.get(), statement->children);
}

void ClassBrowserModel::addMembers()
{
    // show statements in the file
    PFileIncludes p = mParser->findFileIncludes(mCurrentFile);
    if (!p)
        return;
    filterChildren(mRoot,p->statements);
}

void ClassBrowserModel::filterChildren(ClassBrowserNode *node, const StatementMap &statements)
{
    for (PStatement statement:statements) {
        if (statement->kind == StatementKind::skBlock)
            continue;
        if (statement->isInherited && !pSettings->ui().classBrowserShowInherited())
            continue;

        if (statement == node->statement) // prevent infinite recursion
            continue;

        if (statement->scope == StatementScope::ssLocal)
            continue;


//        if (fStatementsType = cbstProject) then begin
//          if not Statement^._InProject then
//            Continue;
//          if Statement^._Static and not SameText(Statement^._FileName,fCurrentFile)
//            and not SameText(Statement^._FileName,fCurrentFile) then
//            Continue;
//        end;

        // we only test and handle orphan statements in the top level (node->statement is null)
        PStatement parentScope = statement->parentScope.lock();
        if ((parentScope!=node->statement) && (!node->statement)) {

//          // we only handle orphan statements when type is cbstFile
//          if fStatementsType <> cbstFile then
//            Continue;

//          //should not happend, just in case of error
            if (!parentScope)
                continue;

            // Processing the orphan statement
            while (statement) {
                //the statement's parent is in this file, so it's not a real orphan
                if ((parentScope->fileName==mCurrentFile)
                        ||(parentScope->definitionFileName==mCurrentFile))
                    break;

                PStatement dummyParent = mDummyStatements.value(parentScope->fullName,PStatement());
                if (dummyParent) {
                    dummyParent->children.insert(statement->command,statement);
                    break;
                }
                dummyParent = createDummy(parentScope);
                dummyParent->children.insert(statement->command,statement);
                //we are adding an orphan statement, just add it
                statement = dummyParent;
                parentScope = statement->parentScope.lock();
                if (!parentScope) {
                    addChild(node,statement);

                    break;
                }
            }
        } else if (statement->kind == StatementKind::skNamespace) {
            PStatement dummy = mDummyStatements.value(statement->fullName,PStatement());
            if (dummy) {
                for (PStatement child: statement->children) {
                    dummy->children.insert(child->command,child);
                }
                continue;
            }
            dummy = createDummy(statement);
            dummy->children = statement->children;
            addChild(node,dummy);
        } else {
            addChild(node,statement);
        }
    }
    if (pSettings->ui().classBrowserSortAlpha()
            && pSettings->ui().classBrowserSortType()) {
        std::sort(node->children.begin(),node->children.end(),
                  [](ClassBrowserNode* node1,ClassBrowserNode* node2) {
            if (node1->statement->kind < node2->statement->kind) {
                return true;
            } else if (node1->statement->kind == node2->statement->kind) {
                return node1->statement->command < node2->statement->command;
            } else {
                return false;
            }
        });
    } else if (pSettings->ui().classBrowserSortAlpha()) {
        std::sort(node->children.begin(),node->children.end(),
                  [](ClassBrowserNode* node1,ClassBrowserNode* node2) {
            return node1->statement->command < node2->statement->command;
        });
    } else if (pSettings->ui().classBrowserSortType()) {
        std::sort(node->children.begin(),node->children.end(),
                  [](ClassBrowserNode* node1,ClassBrowserNode* node2) {
            return node1->statement->kind < node2->statement->kind;
        });
    }
}

PStatement ClassBrowserModel::createDummy(PStatement statement)
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
    result->classScope = statement->classScope;
    result->inProject = statement->inProject;
    result->inSystemHeader = statement->inSystemHeader;
    result->isStatic = statement->isStatic;
    result->isInherited = statement->isInherited;
    result->fileName = mCurrentFile;
    result->definitionFileName = mCurrentFile;
    result->line = 0;
    result->definitionLine = 0;
    mDummyStatements.insert(result->fullName,result);
    return result;
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
            this->fillStatements();
        }
    }
}
