#ifndef EXECUTABLERUNNER_H
#define EXECUTABLERUNNER_H

#include "runner.h"

class ExecutableRunner : public Runner
{
    Q_OBJECT
public:
    ExecutableRunner(const QString& filename, const QString& arguments, const QString& workDir,
                     QObject* parent = nullptr);

    const QString &redirectInputFilename() const;
    void setRedirectInputFilename(const QString &newDataFile);

    bool redirectInput() const;
    void setRedirectInput(bool isRedirect);

    bool startConsole() const;
    void setStartConsole(bool newStartConsole);

private:
    QString mRedirectInputFilename;
    bool mRedirectInput;
    bool mStartConsole;

    // QThread interface
protected:
    void run() override;
};

#endif // EXECUTABLERUNNER_H
