#ifndef EXECUTABLERUNNER_H
#define EXECUTABLERUNNER_H

#include <QThread>

class ExecutableRunner : public QThread
{
    Q_OBJECT
public:
    ExecutableRunner(const QString& filename, const QString& arguments, const QString& workDir);

    const QString &redirectInputFilename() const;
    void setRedirectInputFilename(const QString &newDataFile);

    bool redirectInput() const;
    void setRedirectInput(bool isRedirect);

    bool startConsole() const;
    void setStartConsole(bool newStartConsole);

signals:
    void started();
    void terminated();
    void runErrorOccurred(const QString& reason);

public slots:
    void stop();

private:
    QString mFilename;
    QString mArguments;
    QString mWorkDir;
    bool mStop;
    QString mRedirectInputFilename;
    bool mRedirectInput;
    bool mStartConsole;

    // QThread interface
protected:
    void run() override;
};

#endif // EXECUTABLERUNNER_H
