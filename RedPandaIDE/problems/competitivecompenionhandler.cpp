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
    QObject(parent),
    mThread(nullptr)
{
}

void CompetitiveCompanionHandler::start()
{
    if (!pSettings->executor().enableProblemSet())
        return;
    if (!pSettings->executor().enableCompetitiveCompanion())
        return;
    mThread = new CompetitiveCompanionThread(this);
    if (!mThread->listen()) {
        delete mThread;
        mThread = nullptr;
    } else {
        connect(mThread,
                &CompetitiveCompanionThread::newProblemReceived,
                this, &CompetitiveCompanionHandler::onNewProblemReceived);
        mThread->start();
    }
}

void CompetitiveCompanionHandler::stop()
{
    if (!mThread)
        return;
    connect(mThread, &QThread::finished,
            mThread, &QObject::deleteLater);
    mThread->stop();
    mThread=nullptr;
}

void CompetitiveCompanionHandler::onNewProblemReceived(int num, int total, POJProblem newProblem)
{
    emit newProblemReceived(num, total, newProblem);
}

void CompetitiveCompanionThread::onNewProblemConnection()
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
    //qDebug()<<obj;
    QJsonObject batchObj = obj["batch"].toObject();
    QString batchId = batchObj["id"].toString();
    if (mBatchId!=batchId) {
        mBatchId = batchId;
        mBatchCount = batchObj["size"].toInt();
        mBatchProblemsRecieved = 0;
        //emit newBatchReceived(mBatchCount);
    }

    QString name = obj["name"].toString();

    POJProblem problem = std::make_shared<OJProblem>();
    problem->name = name;
    problem->url = obj["url"].toString();
    if (obj.contains("timeLimit")) {
        problem->timeLimit = obj["timeLimit"].toInt();
        problem->timeLimitUnit = ProblemTimeLimitUnit::Milliseconds;
    }
    if (obj.contains("memoryLimit")) {
        problem->memoryLimit = obj["memoryLimit"].toInt();
        problem->memoryLimitUnit = ProblemMemoryLimitUnit::MB;
    }
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
    mBatchProblemsRecieved++;
    emit newProblemReceived(mBatchProblemsRecieved, mBatchCount, problem);
    // if (mBatchProblemsRecieved == mBatchCount) {
    //     emit batchFinished(mBatchCount);
    // }
}

CompetitiveCompanionThread::CompetitiveCompanionThread(QObject *parent):
    QThread{parent},
    mStop{false},
    mBatchProblemsRecieved{0}
{
    connect(&mTcpServer,&QTcpServer::newConnection,
            this, &CompetitiveCompanionThread::onNewProblemConnection);
}

void CompetitiveCompanionThread::stop()
{
    mStop = true;
}

bool CompetitiveCompanionThread::listen()
{
    if (mTcpServer.listen(QHostAddress::LocalHost,pSettings->executor().competivieCompanionPort())) {
        return true;
    }
    return false;
}

void CompetitiveCompanionThread::run()
{
    while(!mStop) {
        QThread::msleep(100);
    }
    mTcpServer.close();
}
