#include "shortcutmanager.h"
#include "systemconsts.h"
#include "settings.h"
#include <QAction>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>

ShortcutManager::ShortcutManager(QObject *parent) : QObject(parent)
{

}

void ShortcutManager::load()
{
    //if config file not exists, copy it from data
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + DEV_SHORTCUT_FILE;
    if (!fileExists(filename))
        return;
    //read config file
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(nullptr,
                              tr("Read shortcut config failed"),
                              tr("Can't open shortcut config file '%1' for read.")
                              .arg(filename));
        return;
    }

    QByteArray json = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json,&error);
    if (error.error != QJsonParseError::NoError) {
        QMessageBox::critical(nullptr,
                              tr("Read shortcut config failed"),
                              tr("Read shortcut config file '%1' failed:%2")
                              .arg(filename)
                              .arg(error.errorString()));
        return;
    }
    mShortcuts.clear();
    QJsonArray array = doc.array();
    foreach (const QJsonValue& value,array) {
        QJsonObject object = value.toObject();
        PEnvironmentShortcut shortcut = std::make_shared<EnvironmentShortcut>();
        shortcut->name = object["name"].toString();
        if (shortcut->name.isEmpty())
            continue;
        shortcut->shortcut = object["shortcut"].toString();
        mShortcuts.insert(shortcut->name,shortcut);
    }
}

void ShortcutManager::save()
{
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + DEV_SHORTCUT_FILE;
    QFile file(filename);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        QMessageBox::critical(nullptr,
                              tr("Save shortcut config failed"),
                              tr("Can't open shortcut config file '%1' for write.")
                              .arg(filename));
        return;
    }
    QJsonArray array;
    foreach (const PEnvironmentShortcut& shortcut,mShortcuts) {
        QJsonObject object;
        object["name"]=shortcut->name;
        object["shortcut"]=shortcut->shortcut;
        array.append(object);
    }
    QJsonDocument doc;
    doc.setArray(array);
    if (file.write(doc.toJson())<0) {
        QMessageBox::critical(nullptr,
                              tr("Save shortcut config failed"),
                              tr("Write to shortcut config file '%1' failed.")
                              .arg(filename));
        return;
    }
}

void ShortcutManager::setShortcuts(QList<PEnvironmentShortcut> shortcuts)
{
    mShortcuts.clear();
    foreach(PEnvironmentShortcut shortcut, shortcuts) {
        if (shortcut->name.isEmpty())
            continue;
        mShortcuts.insert(shortcut->name,shortcut);
    }
}

void ShortcutManager::applyTo(QList<QAction *> actions)
{
    foreach (QAction* action ,actions) {
        applyTo(action);
    }
}

void ShortcutManager::applyTo(QAction *action)
{
    PEnvironmentShortcut item = mShortcuts.value(action->objectName(), PEnvironmentShortcut());
    if (item) {
        action->setShortcut(QKeySequence::fromString(item->shortcut));
    }
}
