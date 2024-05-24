/*
 * Copyright (C) 2020-2024 Roy Qu (royqh1979@gmail.com)
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
#include "competitivecompenionhandler.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QTextDocument>
#include "ojproblemset.h"
#include "../settings.h"
#include "../mainwindow.h"

CompetitiveCompanionHandler::CompetitiveCompanionHandler(QObject *parent):
    QObject(parent)
{
    connect(&mTcpServer,&QTcpServer::newConnection,
            this, &CompetitiveCompanionHandler::onNewProblemConnection);
}

void CompetitiveCompanionHandler::start()
{
    if (pSettings->executor().enableProblemSet()) {
        if (pSettings->executor().enableCompetitiveCompanion()) {
            if (!mTcpServer.listen(QHostAddress::LocalHost,pSettings->executor().competivieCompanionPort())) {
               // QMessageBox::critical(nullptr,
               //                       tr("Listen failed"),
               //                       tr("Can't listen to port %1 form Competitive Companion.").arg(pSettings->executor().competivieCompanionPort())
               //                       + "<BR/>"
               //                       +tr("You can turn off competitive companion support in the Problem Set options.")
               //                       + "<BR/>"
               //                       +tr("Or You can choose a different port number and try again."));
            }
        }
    }

}

void CompetitiveCompanionHandler::stop()
{
    mTcpServer.close();
}

void CompetitiveCompanionHandler::onNewProblemConnection()
{
    QTcpSocket* clientConnection = mTcpServer.nextPendingConnection();

    connect(clientConnection, &QAbstractSocket::disconnected,
            clientConnection, &QObject::deleteLater);
    QByteArray content;
    int unreadCount = 0;
    while (clientConnection->state() == QTcpSocket::ConnectedState) {
        clientConnection->waitForReadyRead(100);
        QByteArray readed = clientConnection->readAll();
        if (readed.isEmpty()) {
            unreadCount ++;
            if (!content.isEmpty() || unreadCount>30)
                break;
        } else {
            unreadCount = 0;
        }
        content += readed;
    }
    content += clientConnection->readAll();
    clientConnection->write("HTTP/1.1 200 OK");
    clientConnection->disconnectFromHost();
//    qDebug()<<"---------";
//    qDebug()<<content;
    content = getHTTPBody(content);
//    qDebug()<<"*********";
//    qDebug()<<content;
    if (content.isEmpty()) {
        return;
    }
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(content,&error);
    if (error.error!=QJsonParseError::NoError) {
        qDebug()<<"Read http content failed!";
        qDebug()<<error.errorString();
        return;
    }
    QJsonObject obj=doc.object();
    QString name = obj["name"].toString();

    POJProblem problem = std::make_shared<OJProblem>();
    problem->name = name;
    problem->url = obj["url"].toString();
    QJsonArray caseArray = obj["tests"].toArray();
    foreach ( const QJsonValue& val, caseArray) {
        QJsonObject caseObj = val.toObject();
        POJProblemCase problemCase = std::make_shared<OJProblemCase>();
        problemCase->testState = ProblemCaseTestState::NotTested;
        problemCase->name = tr("Problem Case %1").arg(problem->cases.count()+1);
        if (pSettings->executor().convertHTMLToTextForInput()) {
            QTextDocument doc;
            doc.setHtml(caseObj["input"].toString());
            problemCase->input = doc.toPlainText();
        } else
            problemCase->input = caseObj["input"].toString();
        if (pSettings->executor().convertHTMLToTextForExpected()) {
            QTextDocument doc;
            doc.setHtml(caseObj["output"].toString());
            problemCase->expected = doc.toPlainText();
        } else
            problemCase->expected = caseObj["output"].toString();
        problem->cases.append(problemCase);
    }
    emit newProblemReceived(problem);
}
