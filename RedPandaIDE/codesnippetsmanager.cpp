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
   //if config file not exists, copy it from data
    //read config file
}

void CodeSnippetsManager::save()
{
    QString filename = pSettings->dirs().config() + DEV_CODESNIPPET_FILE;
    QFile file(filename);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        QMessageBox::critical(nullptr,
                              tr("Save code snippets failed"),
                              tr("Can't open code snippet file '%1' for write.")
                              .arg(filename));
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
    }
}

void CodeSnippetsManager::addSnippet(const QString &caption, const QString &prefix, const QString &code, const QString &description, int menuSection)
{
    PCodeSnippet snippet = std::make_shared<CodeSnippet>();
    snippet->caption = caption;
    snippet->prefix = prefix;
    snippet->code = code;
    snippet->desc = description;
    snippet->section = menuSection;
    mSnippets.append(snippet);
}

void CodeSnippetsManager::remove(int index)
{
    Q_ASSERT(index>=0 && index<mSnippets.count());
    mSnippets.removeAt(index);
}

void CodeSnippetsManager::clear()
{
    mSnippets.clear();
}

const QList<PCodeSnippet> &CodeSnippetsManager::snippets() const
{
    return mSnippets;
}

int CodeSnippetsModel::rowCount(const QModelIndex &parent) const
{
    return mSnippets.count();
}

int CodeSnippetsModel::columnCount(const QModelIndex &parent) const
{
    return 4;
}

bool CodeSnippetsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{

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
