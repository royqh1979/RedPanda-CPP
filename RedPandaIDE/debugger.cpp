#include "debugger.h"

Debugger::Debugger(QObject *parent) : QObject(parent)
{

}

DebugReader::DebugReader(QObject *parent) : QObject(parent)
{

}

void DebugReader::clearCmdQueue()
{
    QMutexLocker locker(&mMutex);
    mCmdQueue.clear();
    while (mUpdateCount>0) {
        //WatchView.Items.EndUpdate();
        mUpdateCount--;
    }
}

bool DebugReader::findAnnotation(AnnotationType annotation)
{
    AnnotationType NextAnnotation;
    do {
        NextAnnotation = getNextAnnotation();
        if (NextAnnotation == AnnotationType::TEOF)
            return false;
    } while (NextAnnotation != annotation);

    return true;
}

AnnotationType DebugReader::getAnnotation(const QString &s)
{
    if (s == "pre-prompt") {
        return AnnotationType::TPrePrompt;
    } else if (s == "prompt") {
        return AnnotationType::TPrompt;
    } else if (s == "post-prompt") {
      AnnotationType result = AnnotationType::TPostPrompt;

      int IndexBackup = mIndex;
      QString t = GetNextFilledLine();
      int mIndex = IndexBackup;

      //hack to catch local
      if ((mCurrentCmd) && (mCurrentCmd->command == "info locals")) {
          result = AnnotationType::TLocal;
      } else if ((mCurrentCmd) && (mCurrentCmd->command == "info args")) {
          //hack to catch params
          result = AnnotationType::TParam;
      } else if (t.startsWith("rax ") || t.startsWith("eax ")) {
          // Hack fix to catch register dump
          result = AnnotationType::TInfoReg;
      } else {
          // Another hack to catch assembler
          if (t.startsWith("Dump of assembler code for function "))
          result = AnnotationType::TInfoAsm;
      }
      return result;
    } else if (s == "error-begin") {
        return AnnotationType::TErrorBegin;
    } else if (s == "error-end") {
      return AnnotationType::TErrorEnd;
    } else if (s == "display-begin") {
      return AnnotationType::TDisplayBegin;
    } else if (s == "display-expression") {
      return AnnotationType::TDisplayExpression;
    } else if (s == "display-end") {
      return AnnotationType::TDisplayEnd;
    } else if (s == "frame-source-begin") {
      return AnnotationType::TFrameSourceBegin;
    } else if (s == "frame-source-file") {
      return AnnotationType::TFrameSourceFile;
    } else if (s == "frame-source-line") {
      return AnnotationType::TFrameSourceLine;
    } else if (s == "frame-function-name") {
      return AnnotationType::TFrameFunctionName;
    } else if (s == "frame-args") {
      return AnnotationType::TFrameArgs;
    } else if (s == "frame-begin") {
      return AnnotationType::TFrameBegin;
    } else if (s == "frame-end") {
      return AnnotationType::TFrameEnd;
    } else if (s == "frame-where") {
      return AnnotationType::TFrameWhere;
    } else if (s == "source") {
      return AnnotationType::TSource;
    } else if (s == "exited") {
      return AnnotationType::TExit;
    } else if (s == "arg-begin") {
      return AnnotationType::TArgBegin;
    } else if (s == "arg-name-end") {
      return AnnotationType::TArgNameEnd;
    } else if (s == "arg-value") {
      return AnnotationType::TArgValue;
    } else if (s == "arg-end") {
      return AnnotationType::TArgEnd;
    } else if (s == "array-section-begin") {
      return AnnotationType::TArrayBegin;
    } else if (s == "array-section-end") {
      return AnnotationType::TArrayEnd;
    } else if (s == "elt") {
      return AnnotationType::TElt;
    } else if (s == "elt-rep") {
      return AnnotationType::TEltRep;
    } else if (s == "elt-rep-end") {
      return AnnotationType::TEltRepEnd;
    } else if (s == "field-begin") {
      return AnnotationType::TFieldBegin;
    } else if (s == "field-name-end") {
      return AnnotationType::TFieldNameEnd;
    } else if (s == "field-value") {
      return AnnotationType::TFieldValue;
    } else if (s == "field-end") {
      return AnnotationType::TFieldEnd;
    } else if (s == "value-history-value") {
      return AnnotationType::TValueHistoryValue;
    } else if (s == "value-history-begin") {
      return AnnotationType::TValueHistoryBegin;
    } else if (s == "value-history-end") {
      return AnnotationType::TValueHistoryEnd;
    } else if (s == "signal") {
      return AnnotationType::TSignal;
    } else if (s == "signal-name") {
      return AnnotationType::TSignalName;
    } else if (s == "signal-name-end") {
      return AnnotationType::TSignalNameEnd;
    } else if (s == "signal-string") {
      return AnnotationType::TSignalString;
    } else if (s == "signal-string-end") {
      return AnnotationType::TSignalStringEnd;
    } else if (mOutput[mIndex] == 0) {
      return AnnotationType::TEOF;
    } else {
      return AnnotationType::TUnknown;;
    }
}

