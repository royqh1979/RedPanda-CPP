#include "executorsettings.h"
#include "../utils/font.h"

ExecutorSettings::ExecutorSettings(SettingsPersistor *persistor):
    BaseSettings{persistor, SETTING_EXECUTOR}
{

}

bool ExecutorSettings::minimizeOnRun() const
{
    return mMinimizeOnRun;
}

void ExecutorSettings::setMinimizeOnRun(bool minimizeOnRun)
{
    mMinimizeOnRun = minimizeOnRun;
}

bool ExecutorSettings::useParams() const
{
    return mUseParams;
}

void ExecutorSettings::setUseParams(bool newUseParams)
{
    mUseParams = newUseParams;
}

const QString &ExecutorSettings::params() const
{
    return mParams;
}

void ExecutorSettings::setParams(const QString &newParams)
{
    mParams = newParams;
}

bool ExecutorSettings::redirectInput() const
{
    return mRedirectInput;
}

void ExecutorSettings::setRedirectInput(bool newRedirectInput)
{
    mRedirectInput = newRedirectInput;
}

const QString &ExecutorSettings::inputFilename() const
{
    return mInputFilename;
}

void ExecutorSettings::setInputFilename(const QString &newInputFilename)
{
    mInputFilename = newInputFilename;
}

int ExecutorSettings::competivieCompanionPort() const
{
    return mCompetivieCompanionPort;
}

void ExecutorSettings::setCompetivieCompanionPort(int newCompetivieCompanionPort)
{
    mCompetivieCompanionPort = newCompetivieCompanionPort;
}

bool ExecutorSettings::caseEditorFontOnlyMonospaced() const
{
    return mCaseEditorFontOnlyMonospaced;
}

void ExecutorSettings::setCaseEditorFontOnlyMonospaced(bool newCaseEditorFontOnlyMonospaced)
{
    mCaseEditorFontOnlyMonospaced = newCaseEditorFontOnlyMonospaced;
}

size_t ExecutorSettings::caseTimeout() const
{
    return mCaseTimeout;
}

void ExecutorSettings::setCaseTimeout(size_t newCaseTimeout)
{
    mCaseTimeout = newCaseTimeout;
}

size_t ExecutorSettings::caseMemoryLimit() const
{
    return mCaseMemoryLimit;
}

void ExecutorSettings::setCaseMemoryLimit(size_t newCaseMemoryLimit)
{
    mCaseMemoryLimit = newCaseMemoryLimit;
}

bool ExecutorSettings::convertHTMLToTextForExpected() const
{
    return mConvertHTMLToTextForExpected;
}

void ExecutorSettings::setConvertHTMLToTextForExpected(bool newConvertHTMLToTextForExpected)
{
    mConvertHTMLToTextForExpected = newConvertHTMLToTextForExpected;
}

bool ExecutorSettings::redirectStderrToToolLog() const
{
    return mRedirectStderrToToolLog;
}

void ExecutorSettings::setRedirectStderrToToolLog(bool newRedirectStderrToToolLog)
{
    mRedirectStderrToToolLog = newRedirectStderrToToolLog;
}

ProblemCaseValidateType ExecutorSettings::problemCaseValidateType() const
{
    return mProblemCaseValidateType;
}

void ExecutorSettings::setProblemCaseValidateType(ProblemCaseValidateType newProblemCaseValidateType)
{
    mProblemCaseValidateType = newProblemCaseValidateType;
}

bool ExecutorSettings::enableVirualTerminalSequence() const
{
    return mEnableVirualTerminalSequence;
}

void ExecutorSettings::setEnableVirualTerminalSequence(bool newEnableVirualTerminalSequence)
{
    mEnableVirualTerminalSequence = newEnableVirualTerminalSequence;
}

qint64 ExecutorSettings::maxCaseInputFileSize() const
{
    return mMaxCaseInputFileSize;
}

void ExecutorSettings::setMaxCaseInputFileSize(qint64 newMaxCaseInputFileSize)
{
    mMaxCaseInputFileSize = newMaxCaseInputFileSize;
}

bool ExecutorSettings::convertHTMLToTextForInput() const
{
    return mConvertHTMLToTextForInput;
}

void ExecutorSettings::setConvertHTMLToTextForInput(bool newConvertHTMLToTextForInput)
{
    mConvertHTMLToTextForInput = newConvertHTMLToTextForInput;
}

bool ExecutorSettings::enableCaseLimit() const
{
    return mEnableCaseLimit;
}

void ExecutorSettings::setEnableCaseLimit(bool newValue)
{
    mEnableCaseLimit = newValue;
}

int ExecutorSettings::caseEditorFontSize() const
{
    return mCaseEditorFontSize;
}

void ExecutorSettings::setCaseEditorFontSize(int newCaseEditorFontSize)
{
    mCaseEditorFontSize = newCaseEditorFontSize;
}

const QString &ExecutorSettings::caseEditorFontName() const
{
    return mCaseEditorFontName;
}

void ExecutorSettings::setCaseEditorFontName(const QString &newCaseEditorFontName)
{
    mCaseEditorFontName = newCaseEditorFontName;
}

bool ExecutorSettings::enableCompetitiveCompanion() const
{
    return mEnableCompetitiveCompanion;
}

void ExecutorSettings::setEnableCompetitiveCompanion(bool newEnableCompetitiveCompanion)
{
    mEnableCompetitiveCompanion = newEnableCompetitiveCompanion;
}

bool ExecutorSettings::enableProblemSet() const
{
    return mEnableProblemSet;
}

void ExecutorSettings::setEnableProblemSet(bool newEnableProblemSet)
{
    mEnableProblemSet = newEnableProblemSet;
}

void ExecutorSettings::doSave()
{
    saveValue("pause_console", mPauseConsole);
#ifdef Q_OS_WIN
    saveValue("enable_virtual_terminal_sequence", mEnableVirualTerminalSequence);
#endif
    saveValue("minimize_on_run", mMinimizeOnRun);
    saveValue("use_params",mUseParams);
    saveValue("params",mParams);
    saveValue("redirect_input",mRedirectInput);
    saveValue("input_filename",mInputFilename);
    //problem set
    saveValue("enable_proble_set", mEnableProblemSet);
    saveValue("enable_competivie_companion", mEnableCompetitiveCompanion);
    saveValue("competitive_companion_port", mCompetivieCompanionPort);
    saveValue("input_convert_html", mConvertHTMLToTextForInput);
    saveValue("expected_convert_html", mConvertHTMLToTextForExpected);
    saveValue("problem_case_validate_type", (int)mProblemCaseValidateType);
    saveValue("redirect_stderr_to_toollog", mRedirectStderrToToolLog);
    saveValue("case_editor_font_name",mCaseEditorFontName);
    saveValue("case_editor_font_size",mCaseEditorFontSize);
    saveValue("case_editor_font_only_monospaced",mCaseEditorFontOnlyMonospaced);
    saveValue("case_timeout_ms", mCaseTimeout);
    saveValue("case_memory_limit",mCaseMemoryLimit);
    remove("case_timeout");
    saveValue("enable_case_limit", mEnableCaseLimit);
    saveValue("case_max_input_file_size",mMaxCaseInputFileSize);
}

bool ExecutorSettings::pauseConsole() const
{
    return mPauseConsole;
}

void ExecutorSettings::setPauseConsole(bool pauseConsole)
{
    mPauseConsole = pauseConsole;
}

void ExecutorSettings::doLoad()
{
    mPauseConsole = boolValue("pause_console",true);
#ifdef Q_OS_WIN
    mEnableVirualTerminalSequence = boolValue("enable_virtual_terminal_sequence", true);
#endif
    mMinimizeOnRun = boolValue("minimize_on_run",false);
    mUseParams = boolValue("use_params",false);
    mParams = stringValue("params", "");
    mRedirectInput = boolValue("redirect_input",false);
    mInputFilename = stringValue("input_filename","");

    mEnableProblemSet = boolValue("enable_proble_set",true);
    mEnableCompetitiveCompanion = boolValue("enable_competivie_companion",true);
    mCompetivieCompanionPort = intValue("competitive_companion_port",10045);
    mConvertHTMLToTextForInput = boolValue("input_convert_html", false);
    mConvertHTMLToTextForExpected = boolValue("expected_convert_html", false);
    mProblemCaseValidateType =(ProblemCaseValidateType)intValue("problem_case_validate_type", (int)ProblemCaseValidateType::Exact);
    mRedirectStderrToToolLog = boolValue("redirect_stderr_to_toollog", false);

    mCaseEditorFontName = stringValue("case_editor_font_name", defaultMonoFont());
    mCaseEditorFontSize = intValue("case_editor_font_size",11);
    mCaseEditorFontOnlyMonospaced = boolValue("case_editor_font_only_monospaced",true);
    int case_timeout = intValue("case_timeout", -1);
    if (case_timeout>0)
        mCaseTimeout = case_timeout*1000;
    else
        mCaseTimeout = uintValue("case_timeout_ms", 2000); //2000ms
    mCaseMemoryLimit = uintValue("case_memory_limit",0); // kb

    mEnableCaseLimit = boolValue("enable_case_limit", true);

    mMaxCaseInputFileSize = uintValue("case_max_input_file_size", 4); //4mb
}
