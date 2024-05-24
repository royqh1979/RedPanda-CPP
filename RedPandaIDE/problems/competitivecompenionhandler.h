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
#include <QTcpServer>
#include <memory>

class OJProblem;
using POJProblem = std::shared_ptr<OJProblem>;

class CompetitiveCompanionHandler: public QObject {
    Q_OBJECT
public:
    explicit CompetitiveCompanionHandler(QObject* parent=nullptr);
    void start();
    void stop();
signals:
    void newProblemReceived(POJProblem newProblem);
private slots:
    void onNewProblemConnection();
private:
    QTcpServer mTcpServer;
};

#endif // COMPETITIVECOMPANIONHANDLER_H
