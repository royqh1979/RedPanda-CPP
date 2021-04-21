#ifndef EXECUTABLERUNNER_H
#define EXECUTABLERUNNER_H

#include <QThread>

class ExecutableRunner : public QThread
{
    Q_OBJECT
public:
    ExecutableRunner(const QString& filename, const QString& arguments, const QString& workDir);

signals:
    void started();
    void terminated();

public slots:
    void stop();

private:
    QString mFilename;
    QString mArguments;
    QString mWorkDir;
    bool mStop;

    // QThread interface
protected:
    void run() override;
};

#endif // EXECUTABLERUNNER_H
