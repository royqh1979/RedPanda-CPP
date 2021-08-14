#ifndef CPPPARSER_H
#define CPPPARSER_H

#include <QMutex>
#include <QObject>
#include "statementmodel.h"
#include "cpptokenizer.h"
#include "cpppreprocessor.h"

class CppParser : public QObject
{
    Q_OBJECT

    using GetFileStreamCallBack = std::function<bool (const QString&, QStringList&)>;
public:
    explicit CppParser(QObject *parent = nullptr);

    void parseHardDefines();
    function FindFileIncludes(const Filename: AnsiString; DeleteIt: boolean = False): PFileIncludes;
    procedure AddHardDefineByLine(const Line: AnsiString);
    procedure InvalidateFile(const FileName: AnsiString);
    procedure GetFileDirectIncludes(const Filename: AnsiString; var List: TStringList);
    procedure GetFileIncludes(const Filename: AnsiString; var List: TStringList);
    procedure GetFileUsings(const Filename: AnsiString; var List: TDevStringList);

    function IsSystemHeaderFile(const FileName: AnsiString): boolean;
    function IsProjectHeaderFile(const FileName: AnsiString): boolean;
    procedure GetSourcePair(const FName: AnsiString; var CFile, HFile: AnsiString);
    procedure GetClassesList(var List: TStringList);
    function SuggestMemberInsertionLine(ParentStatement: PStatement; Scope: TStatementClassScope; var AddScopeStr:
      boolean):
      integer;
      {
    function GetSystemHeaderFileName(const FileName: AnsiString): AnsiString; // <file.h>
    function GetProjectHeaderFileName(const FileName: AnsiString): AnsiString; // <file.h>
    function GetLocalHeaderFileName(const RelativeTo, FileName: AnsiString): AnsiString; // "file.h"
    }
    function GetHeaderFileName(const RelativeTo, Line: AnsiString): AnsiString; // both
    function IsIncludeLine(const Line: AnsiString): boolean;
    constructor Create(wnd:HWND);
    destructor Destroy; override;
    procedure ParseFileList(UpdateView:boolean = True);
    procedure ParseFile(const FileName: AnsiString; InProject: boolean; OnlyIfNotParsed: boolean = False; UpdateView:
      boolean = True);
    function StatementKindStr(Value: TStatementKind): AnsiString;
    function StatementClassScopeStr(Value: TStatementClassScope): AnsiString;
    procedure Reset;
    procedure ClearIncludePaths;
    procedure ClearProjectIncludePaths;
    procedure ClearProjectFiles;
    procedure AddIncludePath(const Value: AnsiString);
    procedure AddProjectIncludePath(const Value: AnsiString);
    procedure AddFileToScan(Value: AnsiString; InProject: boolean = False);
    function PrettyPrintStatement(Statement: PStatement; line:integer = -1): AnsiString;
    procedure FillListOfFunctions(const FileName, Phrase: AnsiString; const Line: integer;  List: TStringList);
    function FindAndScanBlockAt(const Filename: AnsiString; Line: integer): PStatement;
    function FindStatementOf(FileName, Phrase: AnsiString; Line: integer): PStatement; overload;
    function FindStatementOf(FileName, Phrase: AnsiString; CurrentClass: PStatement;
      var CurrentClassType: PStatement ; force:boolean = False): PStatement; overload;
    function FindStatementOf(FileName, Phrase: AnsiString; CurrentClass: PStatement; force:boolean = False): PStatement; overload;

    function FindKindOfStatementOf(FileName, Phrase: AnsiString; Line: integer): TStatementKind;
    function GetHintFromStatement(FileName, Phrase: AnsiString; Line: integer):AnsiString;
    {Find statement starting from startScope}
    function FindStatementStartingFrom(const FileName, Phrase: AnsiString; startScope: PStatement; force:boolean = False): PStatement;
    function FindTypeDefinitionOf(const FileName: AnsiString;const aType: AnsiString; CurrentClass: PStatement): PStatement;
    function FindFirstTemplateParamOf(const FileName: AnsiString;const aPhrase: AnsiString; currentClass: PStatement): String;
    function FindLastOperator(const Phrase: AnsiString): integer;
    function FindNamespace(const name:AnsiString):TList; // return a list of PSTATEMENTS (of the namespace)
    function Freeze:boolean; overload;  // Freeze/Lock (stop reparse while searching)
    function Freeze(serialId:String):boolean; overload;  // Freeze/Lock (stop reparse while searching)
    procedure UnFreeze; // UnFree/UnLock (reparse while searching)
    function GetParsing: boolean;

    function FindFunctionDoc(const FileName:AnsiString; const Line: integer;
      params:TStringList; var isVoid:boolean): AnsiString;
signals:
private:
    PStatement addInheritedStatement(
            PStatement derived,
            PStatement inherit,
            StatementClassScope access);

    PStatement addChildStatement(
            // support for multiple parents (only typedef struct/union use multiple parents)
            PStatement Parent,
            const QString& fileName,
            const QString& hintText,
            const QString& aType, // "Type" is already in use
            const QString& command,
            const QString& args,
            const QString& value,
            int line,
            StatementKind kind,
            StatementScope scope,
            StatementClassScope classScope,
            bool isDefinition,
            bool isStatic); // TODO: InheritanceList not supported
    PStatement addStatement(
            PStatement parent,
            const QString &fileName,
            const QString &hintText,
            const QString &aType, // "Type" is already in use
            const QString &command,
            const QString &args,
            const QString& value,
            int line,
            StatementKind kind,
            StatementScope scope,
            StatementClassScope classScope,
            bool isDefinition,
            const QStringList& InheritanceList,
            bool isStatic);
    void setInheritance(int index, PStatement classStatement, bool isStruct);
    PStatement getCurrentScope(); // gets last item from last level
    PStatement isInCurrentScopeLevel(const QString& command);
    void addSoloScopeLevel(PStatement statement, int line); // adds new solo level
    void removeScopeLevel(int line); // removes level
    void checkForSkipStatement();
    int skipBraces(int startAt);
    int skipBracket(int startAt);
    bool checkForPreprocessor();
    bool checkForKeyword();
    bool checkForNamespace();
    bool checkForUsing();

    bool checkForTypedef();
    bool checkForTypedefEnum();
    bool checkForTypedefStruct();
    bool CheckForStructs();
    bool checkForMethod(QString &sType, QString &sName, QString &sArgs,
                        bool &isStatic, bool &isFriend); // caching of results
    bool checkForScope();
    bool checkForVar();
    bool checkForEnum();
    bool checkForForBlock();
    bool checkForCatchBlock();
    StatementScope  getScope();
    int getCurrentBlockEndSkip();
    int getCurrentBlockBeginSkip();
    int getCurrentInlineNamespaceEndSkip();
    void handlePreprocessor();
    void handleOtherTypedefs();
    void handleStructs(bool isTypedef = false);
    void HandleMethod(
            const QString& sType,
            const QString& sName,
            const QString& sArgs,
            bool isStatic,
            bool isFriend);
    void scanMethodArgs(
            PStatement functionStatement,
            const QString& argStr);
    void handleScope();
    void handleKeyword();
    void handleVar();
    void handleEnum();
    void handleNamespace();
    void handleUsing();
    void handleForBlock();
    void handleCatchBlock();
    bool handleStatement();
    void internalParse(
            const QString& fileName,
            bool manualUpdate = false);
//    function FindMacroDefine(const Command: AnsiString): PStatement;
    QString expandMacroType(const QString& name);
    void inheritClassStatement(
            PStatement derived,
            bool isStruct,
            PStatement base,
            StatementClassScope access);
    PStatement getIncompleteClass(
            const QString& command,
            PStatement parentScope);
    QString getFullStatementName(
            const QString& command,
            PStatement parent);
    //{procedure ResetDefines;}
    PStatement findMemberOfStatement(
            const QString& phrase,
            PStatement scopeStatement);
    void getFullNameSpace(
            const QString& phrase,
            QString& sNamespace,
            QString& member);
    PStatement findStatementInScope(
            const QString& name,
            const QString& noNameArgs,
            StatementKind kind,
            PStatement scope);
    void internalInvalidateFile(const QString& fileName);
    void internalInvalidateFiles(const QStringList& files);
    void calculateFilesToBeReparsed(const QString& fileName,
                                    QStringList& files);
//    {
//    function GetClass(const Phrase: AnsiString): AnsiString;
//    function GetMember(const Phrase: AnsiString): AnsiString;
//    function GetOperator(const Phrase: AnsiString): AnsiString;
//    function GetRemainder(const Phrase: AnsiString): AnsiString;
//    }
    QString splitPhrase(const QString& phrase, QString& sClazz, QString &sMember,
                QString& sOperator);
    QString getStatementKey(const QString& sName,
                            const QString& sType,
                            const QString& sNoNameArgs);
    void onProgress(const QString& fileName, int total, int current);
    void onBusy();
    void onStartParsing();
    void onEndParsing(int total, int updateView);
    void updateSerialId();
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
    //{ List of current compiler set's include path}
    QStringList mIncludePaths;
    //{ List of current project's include path }
    QStringList mProjectIncludePaths;
    //{ List of current project's include path }
    QSet<QString> mProjectFiles;
    QVector<int> mBlockBeginSkips; //list of for/catch block begin token index;
    QVector<int> mBlockEndSkips; //list of for/catch block end token index;
    QVector<int> mInlineNamespaceEndSkips; // list for inline namespace end token index;
    QStringList mFilesToScan; // list of base files to scan
    int mFilesScannedCount; // count of files that have been scanned
    int mFilesToScanCount; // count of files and files included in files that have to be scanned
    bool mParseLocalHeaders;
    bool mParseGlobalHeaders;
    bool mIsProjectFile;
    //fMacroDefines : TList;
    int mLockCount; // lock(don't reparse) when we need to find statements in a batch
    bool mParsing;
    QHash<QString,PStatementList> mNamespaces;  //TStringList<String,List<Statement>> namespace and the statements in its scope
    //fRemovedStatements: THashedStringList; //THashedStringList<String,PRemovedStatements>

    QMutex mMutex;
    GetFileStreamCallBack mOnGetFileStream;
};

#endif // CPPPARSER_H
