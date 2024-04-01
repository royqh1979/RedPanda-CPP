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
#include "codesnippetsmanager.h"
#include "settings.h"
#include "systemconsts.h"
#include <QMessageBox>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

CodeSnippetsManager::CodeSnippetsManager(QObject *parent) : QObject(parent)
{

}

void CodeSnippetsManager::load()
{
    loadSnippets();
    mNewCppFileTemplate =  loadNewFileTemplate(DEV_NEWFILETEMPLATES_FILE);
    mNewCFileTemplate =  loadNewFileTemplate(DEV_NEWCFILETEMPLATES_FILE);
    mNewGASFileTemplate =  loadNewFileTemplate(DEV_NEWGASFILETEMPLATES_FILE);
}

void CodeSnippetsManager::save()
{
    saveSnippets();
    saveNewFileTemplate(DEV_NEWFILETEMPLATES_FILE, mNewCppFileTemplate);
    saveNewFileTemplate(DEV_NEWCFILETEMPLATES_FILE, mNewCFileTemplate);
    saveNewFileTemplate(DEV_NEWGASFILETEMPLATES_FILE, mNewGASFileTemplate);
}

void CodeSnippetsManager::loadSnippets()
{
    //if config file not exists, copy it from data
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + DEV_CODESNIPPET_FILE;
    if (!fileExists(filename)) {
        QString preFileName = ":/config/codesnippets.json";
        QFile preFile(preFileName);
        if (!preFile.open(QFile::ReadOnly)) {
            QMessageBox::critical(nullptr,
                                  tr("Load default code snippets failed"),
                                  tr("Can't copy default code snippets '%1' to '%2'.")
                                  .arg(preFileName)
                                  .arg(filename));
            return;
        }
        QByteArray content=preFile.readAll();
        QFile file(filename);
        if (!file.open(QFile::WriteOnly|QFile::Truncate)) {
            QMessageBox::critical(nullptr,
                                  tr("Load default code snippets failed"),
                                  tr("Can't copy default code snippets '%1' to '%2'.")
                                  .arg(preFileName)
                                  .arg(filename));
            return;
        }
        file.write(content);
    }
    //read config file
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(nullptr,
                              tr("Read code snippets failed"),
                              tr("Can't open code snippet file '%1' for read.")
                              .arg(filename));
        return;
    }

    QByteArray json = file.readAll().trimmed();
    if (json.isEmpty())
        return;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json,&error);
    if (error.error != QJsonParseError::NoError) {
        QMessageBox::critical(nullptr,
                              tr("Read code snippets failed"),
                              tr("Read code snippet file '%1' failed:%2")
                              .arg(filename)
                              .arg(error.errorString()));
        return;
    }
    mSnippets.clear();
    QJsonArray array = doc.array();
    foreach (const QJsonValue& value,array) {
        QJsonObject object = value.toObject();
        PCodeSnippet snippet = std::make_shared<CodeSnippet>();
        snippet->caption = object["caption"].toString();
        snippet->prefix = object["prefix"].toString();
        snippet->code = object["code"].toString();
        snippet->desc = object["description"].toString();
        snippet->section = object["section"].toInt();
        mSnippets.append(snippet);
    }
}

void CodeSnippetsManager::saveSnippets()
{
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + DEV_CODESNIPPET_FILE;
    QFile file(filename);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        QMessageBox::critical(nullptr,
                              tr("Save code snippets failed"),
                              tr("Can't open code snippet file '%1' for write.")
                              .arg(filename));
        return;
    }
    QJsonArray array;
    foreach (const PCodeSnippet& snippet,mSnippets) {
        QJsonObject object;
        object["caption"]=snippet->caption;
        object["prefix"]=snippet->prefix;
        object["code"]=snippet->code;
        object["description"]=snippet->desc;
        object["section"]=snippet->section;
        array.append(object);
    }
    QJsonDocument doc;
    doc.setArray(array);
    if (file.write(doc.toJson())<0) {
        QMessageBox::critical(nullptr,
                              tr("Save code snippets failed"),
                              tr("Write to code snippet file '%1' failed.")
                              .arg(filename));
        return;
    }
}

QString CodeSnippetsManager::loadNewFileTemplate(const QString &fn)
{
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + fn;
    QFile file(filename);
    if (!file.exists()) {
        return "";
    }
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(nullptr,
                              tr("Load new file template failed"),
                              tr("Can't open new file template file '%1' for read.")
                              .arg(filename));
        return "";
    }
    return QString::fromUtf8(file.readAll());
}

void CodeSnippetsManager::saveNewFileTemplate(const QString &fn, const QString &templateContent)
{
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + fn;
    QFile file(filename);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        QMessageBox::critical(nullptr,
                              tr("Save new file template failed"),
                              tr("Can't open new file template file '%1' for write.")
                              .arg(filename));
        return;
    }
    file.write(templateContent.toUtf8());
}

QString CodeSnippetsManager::newGASFileTemplate() const
{
    return mNewGASFileTemplate;
}

void CodeSnippetsManager::setNewGASFileTemplate(const QString &newContent)
{
    mNewGASFileTemplate = newContent;
}

QString CodeSnippetsManager::newCFileTemplate() const
{
    return mNewCFileTemplate;
}

void CodeSnippetsManager::setNewCFileTemplate(const QString &newContent)
{
    mNewCFileTemplate = newContent;
}

const QList<PCodeSnippet> &CodeSnippetsManager::snippets() const
{
    return mSnippets;
}

void CodeSnippetsManager::setSnippets(const QList<PCodeSnippet> &newSnippets)
{
    mSnippets = newSnippets;
}

const QString &CodeSnippetsManager::newCppFileTemplate() const
{
    return mNewCppFileTemplate;
}

void CodeSnippetsManager::setNewCppFileTemplate(const QString &content)
{
    mNewCppFileTemplate = content;
}

void CodeSnippetsModel::addSnippet(const QString &caption, const QString &prefix, const QString &code, const QString &description, int menuSection)
{
    beginInsertRows(QModelIndex(),mSnippets.count(),mSnippets.count());
    PCodeSnippet snippet = std::make_shared<CodeSnippet>();
    snippet->caption = caption;
    snippet->prefix = prefix;
    snippet->code = code;
    snippet->desc = description;
    snippet->section = menuSection;
    mSnippets.append(snippet);
    endInsertRows();
}

void CodeSnippetsModel::remove(int index)
{
    Q_ASSERT(index>=0 && index<mSnippets.count());
    beginRemoveRows(QModelIndex(),index,index);
    mSnippets.removeAt(index);
    endRemoveRows();
}

void CodeSnippetsModel::clear()
{
    beginResetModel();
    mSnippets.clear();
    endResetModel();
}

QModelIndex CodeSnippetsModel::lastSnippetCaption()
{
    Q_ASSERT(mSnippets.count()>0);
    return createIndex(mSnippets.count()-1,0);
}

int CodeSnippetsModel::rowCount(const QModelIndex &) const
{
    return mSnippets.count();
}

int CodeSnippetsModel::columnCount(const QModelIndex &) const
{
    return 4;
}

QVariant CodeSnippetsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (role==Qt::DisplayRole
            || role == Qt::EditRole) {
        int row = index.row();
        PCodeSnippet snippet = mSnippets[row];
        switch(index.column()) {
        case 0:
            return snippet->caption;
        case 1:
            return snippet->prefix;
        case 2:
            return snippet->desc;
        case 3:
            return snippet->section;
        }
    }
    return QVariant();
}

bool CodeSnippetsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }
    if (role==Qt::EditRole) {
        int row = index.row();
        PCodeSnippet snippet = mSnippets[row];
        switch(index.column()) {
        case 0:
            snippet->caption = value.toString();
            return true;
        case 1:
            snippet->prefix = value.toString();
            return true;
        case 2:
            snippet->desc = value.toString();
            return true;
        case 3:
            snippet->section = value.toInt();
            return true;
        }
    }
    return false;
}

QVariant CodeSnippetsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role ==  Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Caption");
        case 1:
            return tr("Completion Prefix");
        case 2:
            return tr("Description");
        case 3:
            return tr("Menu Section");
        }
    }
    return QVariant();
}

Qt::ItemFlags CodeSnippetsModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

const QList<PCodeSnippet> &CodeSnippetsModel::snippets() const
{
    return mSnippets;
}

void CodeSnippetsModel::updateSnippets(const QList<PCodeSnippet> &snippets)
{
    beginResetModel();
    mSnippets = snippets;
    endResetModel();
}
