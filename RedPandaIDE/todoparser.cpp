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
#include "todoparser.h"
#include "mainwindow.h"
#include "editor.h"
#include "editorlist.h"

#include <QRegularExpression>


static QRegularExpression todoReg("\\b(todo|fixme)\\b", QRegularExpression::CaseInsensitiveOption);
TodoParser::TodoParser(QObject *parent) : QObject(parent),
    mMutex()
{
    mThread = nullptr;
}

void TodoParser::parseFile(const QString &filename,bool isForProject)
{
    QMutexLocker locker(&mMutex);
    if (mThread) {
        return;
    }
    mThread = new TodoThread(filename);
    connect(mThread,&QThread::finished,
            [this] {
        QMutexLocker locker(&mMutex);
        if (mThread) {
            mThread->deleteLater();
            mThread = nullptr;
        }
    });
    if (!isForProject) {
        connect(mThread, &TodoThread::parseStarted,
            pMainWindow, &MainWindow::onTodoParseStarted);
    }
    connect(mThread, &TodoThread::parsingFile,
            pMainWindow, &MainWindow::onTodoParsingFile);
    connect(mThread, &TodoThread::todoFound,
            pMainWindow, &MainWindow::onTodoFound);
    connect(mThread, &TodoThread::parseFinished,
            pMainWindow, &MainWindow::onTodoParseFinished);
    mThread->start();
}

void TodoParser::parseFiles(const QStringList &files)
{
    QMutexLocker locker(&mMutex);
    if (mThread) {
        return;
    }
    mThread = new TodoThread(files);
    connect(mThread,&QThread::finished,
            [this] {
        QMutexLocker locker(&mMutex);
        if (mThread) {
            mThread->deleteLater();
            mThread = nullptr;
        }
    });
    connect(mThread, &TodoThread::parseStarted,
            pMainWindow, &MainWindow::onTodoParseStarted);
    connect(mThread, &TodoThread::parsingFile,
            pMainWindow, &MainWindow::onTodoParsingFile);
    connect(mThread, &TodoThread::todoFound,
            pMainWindow, &MainWindow::onTodoFound);
    connect(mThread, &TodoThread::parseFinished,
            pMainWindow, &MainWindow::onTodoParseFinished);
    mThread->start();
}

bool TodoParser::parsing() const
{
    return (mThread!=nullptr);
}

TodoThread::TodoThread(const QString &filename, QObject *parent): QThread(parent)
{
    mFilename = filename;
    mParseFiles = false;
}

TodoThread::TodoThread(const QStringList &files, QObject *parent): QThread(parent)
{
    mFiles = files;
    mParseFiles = true;
}

void TodoThread::parseFile()
{
    QSynedit::PSyntaxer syntaxer = syntaxerManager.getSyntaxer(QSynedit::ProgrammingLanguage::CPP);
    emit parseStarted();
    doParseFile(mFilename,syntaxer);
    emit parseFinished();
}

void TodoThread::parseFiles()
{
    QSynedit::PSyntaxer highlighter = syntaxerManager.getSyntaxer(QSynedit::ProgrammingLanguage::CPP);
    emit parseStarted();
    foreach(const QString& filename,mFiles) {
        doParseFile(filename,highlighter);
    }
    emit parseFinished();
}

void TodoThread::doParseFile(const QString &filename, QSynedit::PSyntaxer syntaxer)
{
    emit parsingFile(filename);
    QStringList lines;
    if (!pMainWindow->editorList()->getContentFromOpenedEditor(filename,lines)) {
        lines = readFileToLines(filename);
    }
    syntaxer->resetState();
    for (int i =0;i<lines.count();i++) {
        syntaxer->setLine(lines[i],i);
        while (!syntaxer->eol()) {
            QSynedit::PTokenAttribute attr;
            attr = syntaxer->getTokenAttribute();
            if (attr && attr->tokenType() == QSynedit::TokenType::Comment) {
                QString token = syntaxer->getToken();
                int pos = token.indexOf(todoReg);
                if (pos>=0) {
                    emit todoFound(
                                filename,
                                i+1,
                                pos+syntaxer->getTokenPos(),
                                lines[i].trimmed()
                                );
                    break;
                }
            }
            syntaxer->next();
        }
    }

}

void TodoThread::run()
{
    if (mParseFiles) {
        parseFiles();
    } else {
        parseFile();
    }
}

TodoModel::TodoModel(QObject *parent) : QAbstractListModel(parent)
{
    mIsForProject=false;
}

void TodoModel::addItem(const QString &filename, int lineNo, int ch, const QString &line)
{
    QList<PTodoItem> &items=getItems(mIsForProject);
    int pos=-1;
    for (int i=0;i<items.count();i++) {
        int comp=QString::compare(filename,items[i]->filename);
        if (comp<0) {
            pos=i;
            break;
        } else if (comp==0) {
            if (lineNo<items[i]->lineNo)  {
                pos=i;
                break;
            }
        }
    }
    if (pos<0) {
        pos=items.count();
    }
    beginInsertRows(QModelIndex(),pos,pos);
    PTodoItem item = std::make_shared<TodoItem>();
    item->filename = filename;
    item->lineNo = lineNo;
    item->ch = ch;
    item->line = line;
    items.insert(pos,item);
    endInsertRows();
}

void TodoModel::removeTodosForFile(const QString &filename)
{
    QList<PTodoItem> &items=getItems(mIsForProject);
    for(int i=items.count()-1;i>=0;i--) {
        PTodoItem item = items[i];
        if (item->filename==filename) {
            beginRemoveRows(QModelIndex(),i,i);
            items.removeAt(i);
            endRemoveRows();
        }
    }
}

void TodoModel::clear()
{
    beginResetModel();
    QList<PTodoItem> &items=getItems(mIsForProject);
    items.clear();
    endResetModel();
}

void TodoModel::clear(bool forProject)
{
    if (mIsForProject == forProject)
        beginResetModel();
    QList<PTodoItem> &items=getItems(forProject);
    items.clear();
    if (mIsForProject == forProject)
        endResetModel();
}

PTodoItem TodoModel::getItem(const QModelIndex &index)
{
    if (!index.isValid())
        return PTodoItem();
    return getItems(mIsForProject)[index.row()];
}

QList<PTodoItem> &TodoModel::getItems(bool forProject)
{
    return forProject?mProjectItems:mItems;
}

const QList<PTodoItem> &TodoModel::getConstItems(bool forProject) const
{
    return forProject?mProjectItems:mItems;
}

bool TodoModel::isForProject() const
{
    return mIsForProject;
}

void TodoModel::setIsForProject(bool newIsForProject)
{
    if (mIsForProject!=newIsForProject) {
        beginResetModel();
        mIsForProject = newIsForProject;
        endResetModel();
    }
}

int TodoModel::rowCount(const QModelIndex &) const
{
    const QList<PTodoItem> &items=getConstItems(mIsForProject);
    return items.count();
}

QVariant TodoModel::data(const QModelIndex &index, int role) const
{
    const QList<PTodoItem> &items=getConstItems(mIsForProject);
    if (!index.isValid())
        return QVariant();
    if (role==Qt::DisplayRole) {
        PTodoItem item = items[index.row()];
        switch(index.column()) {
        case 0:
            return item->filename;
        case 1:
            return item->lineNo;
        case 2:
            return item->line;
        }
    }
    return QVariant();
}

QVariant TodoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Filename");
        case 1:
            return tr("Line");
        case 2:
            return tr("Content");
        }
    }
    return QVariant();
}

int TodoModel::columnCount(const QModelIndex &) const
{
    return 3;
}
