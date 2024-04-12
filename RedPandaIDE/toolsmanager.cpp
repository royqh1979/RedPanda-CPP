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
#include "toolsmanager.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QUuid>
#include "settings.h"
#include "systemconsts.h"

ToolsManager::ToolsManager(QObject *parent) : QObject(parent)
{

}

void ToolsManager::load()
{
    //if config file not exists, copy it from data
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + DEV_TOOLS_FILE;
    if (!fileExists(filename)) {
        mTools.clear();
        PToolItem item = std::make_shared<ToolItem>();
        item->id = QUuid::createUuid().toString();
        item->title = tr("Remove Compiled");
#ifdef Q_OS_WIN
        item->program = "del";
#else
        item->program = "rm";
#endif
        item->workingDirectory = "<SOURCEPATH>";
#ifdef Q_OS_WIN
        item->parameters = "/q /f <EXENAME>";
#else
        item->parameters = "-f <EXENAME>";
#endif
        item->inputOrigin = ToolItemInputOrigin::None;
        item->outputTarget = ToolItemOutputTarget::RedirectToToolsOutputPanel;
        item->isUTF8 = false;
        mTools.append(item);
//#ifdef Q_OS_WIN
//        item = std::make_shared<ToolItem>();
//        item->title = tr("Open compiled in explorer");
//        item->program = "explorer.exe";
//        item->workingDirectory = "<SOURCEPATH>";
//        item->parameters = " /n, /select, <EXENAME>";
//        item->pauseAfterExit = false;
//        mTools.append(item);
//#endif
        save();
        return;
    }
    //read config file
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(nullptr,
                              tr("Read tools config failed"),
                              tr("Can't open tools config file '%1' for read.")
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
                              tr("Read tools config failed"),
                              tr("Read tools config file '%1' failed:%2")
                              .arg(filename,error.errorString()));
        return;
    }
    mTools.clear();
    QJsonArray array = doc.array();
    foreach (const QJsonValue& value,array) {
        QJsonObject object = value.toObject();
        PToolItem item = std::make_shared<ToolItem>();
        if (!object.contains("id"))
            item->id = QUuid::createUuid().toString();
        else
            item->id = object["id"].toString();
        item->title = object["title"].toString();
        if (item->title.isEmpty())
            continue;
        item->program = object["program"].toString();
        item->workingDirectory = object["workingDirectory"].toString();
        item->parameters = object["parameters"].toString();
        item->outputTarget = static_cast<ToolItemOutputTarget>(object["outputTarget"].toInt(0));
        item->inputOrigin= static_cast<ToolItemInputOrigin>(object["inputOrigin"].toInt(0));
        item->isUTF8 = object["isUTF8"].toBool(true);
        mTools.append(item);
    }
}

void ToolsManager::save()
{
    QString filename = includeTrailingPathDelimiter(pSettings->dirs().config()) + DEV_TOOLS_FILE;
    QFile file(filename);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        QMessageBox::critical(nullptr,
                              tr("Save tools config failed"),
                              tr("Can't open tools config file '%1' for write.")
                              .arg(filename));
        return;
    }
    QJsonArray array;
    foreach (const PToolItem& tool,mTools) {
        QJsonObject object;
        object["id"]=tool->id;
        object["title"]=tool->title;
        object["program"]=tool->program;
        object["workingDirectory"] = tool->workingDirectory;
        object["parameters"]=tool->parameters;
        object["outputTarget"]=static_cast<int>(tool->outputTarget);
        object["inputOrigin"]=static_cast<int>(tool->inputOrigin);
        object["isUTF8"]=tool->isUTF8;
        array.append(object);
    }
    QJsonDocument doc;
    doc.setArray(array);
    if (file.write(doc.toJson())<0) {
        QMessageBox::critical(nullptr,
                              tr("Save tools config failed"),
                              tr("Write to tools config file '%1' failed.")
                              .arg(filename));
        return;
    }

}

const QList<PToolItem> &ToolsManager::tools() const
{
    return mTools;
}

PToolItem ToolsManager::findTool(const QString &title)
{
    for (int i=0;i<mTools.count();i++) {
        PToolItem item = mTools[i];
        if (title == item->title) {
            return item;
        }
    }
    return PToolItem();
}

void ToolsManager::setTools(const QList<PToolItem> &newTools)
{
    mTools = newTools;
}
