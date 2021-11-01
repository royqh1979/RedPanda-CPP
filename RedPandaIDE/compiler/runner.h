#ifndef RUNNER_H
#define RUNNER_H

#include <QThread>

class Runner : public QThread
{
    Q_OBJECT
public:
    explicit Runner(const QString& filename, const QString& arguments, const QString& workDir, QObject *parent = nullptr);

signals:
    void started();
    void terminated();
    void runErrorOccurred(const QString& reason);

public slots:
    void stop();

protected:
    bool mStop;
    QString mFilename;
    QString mArguments;
    QString mWorkDir;
};

#endif // RUNNER_H
