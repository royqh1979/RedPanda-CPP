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
#ifndef RUNNER_H
#define RUNNER_H

#include <QThread>

class Runner : public QThread
{
    Q_OBJECT
public:
    explicit Runner(const QString& filename, const QStringList& arguments, const QString& workDir, QObject *parent = nullptr);
    Runner(const Runner&)=delete;
    Runner operator=(const Runner&)=delete;

    bool pausing() const;

    // time (in milliseconds) waiting for process finished in each processing loop
    int waitForFinishTime() const;
    void setWaitForFinishTime(int newWaitForFinishTime);

signals:
    void started();
    void terminated();
    void runErrorOccurred(const QString& reason);
    void pausingForFinish(); // finish but pausing

public slots:
    void stop();
protected:
    virtual void doStop();
    void setPausing(bool newCanFinish);
protected:
    bool mPausing;
    bool mStop;
    QString mFilename;
    QStringList mArguments; // without argv[0]
    QString mWorkDir;
    int mWaitForFinishTime;
};

#endif // RUNNER_H
