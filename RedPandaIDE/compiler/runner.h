#ifndef RUNNER_H
#define RUNNER_H

#include <QThread>

class Runner : public QThread
{
    Q_OBJECT
public:
    explicit Runner(const QString& filename, const QString& arguments, const QString& workDir, QObject *parent = nullptr);

    bool pausing() const;


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
    QString mArguments;
    QString mWorkDir;
};

#endif // RUNNER_H
