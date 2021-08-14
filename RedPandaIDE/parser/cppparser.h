#ifndef CPPPARSER_H
#define CPPPARSER_H

#include <QObject>
#include "statementmodel.h"
#include "cpptokenizer.h"
#include "cpppreprocessor.h"

class CppParser : public QObject
{
    Q_OBJECT
public:
    explicit CppParser(QObject *parent = nullptr);

signals:
private:
    int mParserId;
    int mSerialCount;
    int mSerialId;
    int mUniqId;
    bool mEnabled;
    int mIndex;
    bool mIsHeader;
    bool mIsSystemHeader;
    QString mCurrentFile;
//  stack list , each element is a list of one/many scopes(like intypedef struct  s1,s2;
//  It's used for store scope nesting infos
    QVector<PStatement> mCurrentScope;
    QVector<StatementClassScope> mCurrentClassScope;

//  the start index in tokens to skip to ; when parsing typedef struct we need to skip
//   the names after the closing bracket because we have processed it
    QVector<int> mSkipList; // TList<Integer>
    StatementClassScope mClassScope;
    StatementModel mStatementList;
    std::shared_ptr<QHash<QString,PFileIncludes>> mIncludesList; //List of scaned files and it's infos
    //It's used in preprocessor, so we can't use fIncludeList instead
    std::shared_ptr<QSet<QString>> mScannedFiles; // List of scaned file names
    CppTokenizer mTokenizer;
    CppPreprocessor mPreprocessor;
    { List of current compiler set's include path}
    fIncludePaths: TStringList;
    { List of current project's include path }
    fProjectIncludePaths: TStringList;
    { List of current project's include path }
    fProjectFiles: TStringList;
    fBlockBeginSkips: TIntList; //list of for/catch block begin token index;
    fBlockEndSkips: TIntList; //list of for/catch block end token index;
    fInlineNamespaceEndSkips: TIntList; // list for inline namespace end token index;
    fFilesToScan: TStringList; // list of base files to scan
    fFilesScannedCount: Integer; // count of files that have been scanned
    fFilesToScanCount: Integer; // count of files and files included in files that have to be scanned
    fParseLocalHeaders: boolean;
    fParseGlobalHeaders: boolean;
    fIsProjectFile: boolean;
    //fMacroDefines : TList;
    fLockCount: integer; // lock(don't reparse) when we need to find statements in a batch
    fParsing: boolean;
    fNamespaces :TDevStringList;  //TStringList<String,List<Statement>> namespace and the statements in its scope
    //fRemovedStatements: THashedStringList; //THashedStringList<String,PRemovedStatements>
    fCriticalSection: TCriticalSection;
    fOnGetFileStream : TGetFileStreamEvent;
};

#endif // CPPPARSER_H
