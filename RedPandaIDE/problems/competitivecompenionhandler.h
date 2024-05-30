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
#ifndef COMPETITIVECOMPANIONHANDLER_H
#define COMPETITIVECOMPANIONHANDLER_H
#include <QObject>
#include <memory>
#include <QThread>
#include <QSemaphore>

struct OJProblem;
using POJProblem = std::shared_ptr<OJProblem>;

class QTcpSocket;
class CompetitiveCompanionThread: public QThread {
    Q_OBJECT
public:
    explicit CompetitiveCompanionThread(QObject *parent=nullptr);
    CompetitiveCompanionThread(const CompetitiveCompanionThread&) = delete;
    CompetitiveCompanionThread &operator=(const CompetitiveCompanionThread&) = delete;
    void stop();
    bool waitStart();
    void waitStop();
signals:
    void newProblemReceived(int num, int total, POJProblem newProblem);
    // void newBatchReceived(int total);
    // void batchFinished(int total);
private slots:
    void onNewProblemConnection(QTcpSocket* connection);
    // QThread interface
protected:
    void run() override;
private:
    bool mStop;
    QString mBatchId;
    int mBatchCount;
    int mBatchProblemsRecieved;
    QSemaphore mStartSemaphore;
    QSemaphore mStopSemaphore;
    bool mStartOk;
};


class CompetitiveCompanionHandler: public QObject {
    Q_OBJECT
public:
    explicit CompetitiveCompanionHandler(QObject* parent=nullptr);
    CompetitiveCompanionHandler(const CompetitiveCompanionHandler&) = delete;
    CompetitiveCompanionHandler &operator =(const CompetitiveCompanionHandler&) = delete;
    void start();
    void stop();
signals:
    void newProblemReceived(int num, int total, POJProblem newProblem);
private slots:
    void onNewProblemReceived(int num, int total, POJProblem newProblem);
private:
    CompetitiveCompanionThread *mThread;
};


#endif // COMPETITIVECOMPANIONHANDLER_H
