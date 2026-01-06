#ifndef EXECUTOR_SETTINGS_H
#define EXECUTOR_SETTINGS_H
#include "basesettings.h"
#include "../utils.h"

#define SETTING_EXECUTOR "Executor"

class ExecutorSettings: public BaseSettings {
public:
    explicit ExecutorSettings(SettingsPersistor * persistor);

    bool pauseConsole() const;
    void setPauseConsole(bool pauseConsole);

    bool minimizeOnRun() const;
    void setMinimizeOnRun(bool minimizeOnRun);

    bool useParams() const;
    void setUseParams(bool newUseParams);
    const QString &params() const;
    void setParams(const QString &newParams);
    bool redirectInput() const;
    void setRedirectInput(bool newRedirectInput);
    const QString &inputFilename() const;
    void setInputFilename(const QString &newInputFilename);

    bool enableProblemSet() const;
    void setEnableProblemSet(bool newEnableProblemSet);

    bool enableCompetitiveCompanion() const;
    void setEnableCompetitiveCompanion(bool newEnableCompetitiveCompanion);

    int competivieCompanionPort() const;
    void setCompetivieCompanionPort(int newCompetivieCompanionPort);

    const QString &caseEditorFontName() const;
    void setCaseEditorFontName(const QString &newCaseEditorFontName);

    int caseEditorFontSize() const;
    void setCaseEditorFontSize(int newCaseEditorFontSize);

    bool caseEditorFontOnlyMonospaced() const;
    void setCaseEditorFontOnlyMonospaced(bool newCaseEditorFontOnlyMonospaced);

    bool enableCaseLimit() const;
    void setEnableCaseLimit(bool newValue);

    size_t caseTimeout() const;
    void setCaseTimeout(size_t newCaseTimeout);

    size_t caseMemoryLimit() const;
    void setCaseMemoryLimit(size_t newCaseMemoryLimit);

    bool convertHTMLToTextForInput() const;
    void setConvertHTMLToTextForInput(bool newConvertHTMLToTextForInput);

    bool convertHTMLToTextForExpected() const;
    void setConvertHTMLToTextForExpected(bool newConvertHTMLToTextForExpected);

    bool redirectStderrToToolLog() const;
    void setRedirectStderrToToolLog(bool newRedirectStderrToToolLog);

    ProblemCaseValidateType problemCaseValidateType() const;
    void setProblemCaseValidateType(ProblemCaseValidateType newProblemCaseValidateType);

    bool enableVirualTerminalSequence() const;
    void setEnableVirualTerminalSequence(bool newEnableVirualTerminalSequence);
    qint64 maxCaseInputFileSize() const;
    void setMaxCaseInputFileSize(qint64 newMaxCaseInputFileSize);

private:
    // general
    bool mPauseConsole;
    bool mMinimizeOnRun;
    bool mUseParams;
    QString mParams;
    bool mRedirectInput;
    QString mInputFilename;
    bool mEnableVirualTerminalSequence;

    //Problem Set
    bool mEnableProblemSet;
    bool mEnableCompetitiveCompanion;
    int mCompetivieCompanionPort;
    bool mConvertHTMLToTextForInput;
    bool mConvertHTMLToTextForExpected;
    bool mIgnoreSpacesWhenValidatingCases;
    ProblemCaseValidateType mProblemCaseValidateType;
    bool mRedirectStderrToToolLog;
    QString mCaseEditorFontName;
    int mCaseEditorFontSize;
    bool mCaseEditorFontOnlyMonospaced;
    bool mEnableCaseLimit;
    qulonglong mCaseTimeout; //ms
    qulonglong mCaseMemoryLimit; //kb
    qint64 mMaxCaseInputFileSize; // mb

protected:
    void doSave() override;
    void doLoad() override;
};

#endif
//EXECUTOR_SETTINGS_H
