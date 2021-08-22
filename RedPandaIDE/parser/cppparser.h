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
    ~CppParser();

    void addHardDefineByLine(const QString& line);
    void addFileToScan(QString value, bool inProject = false);
    void addIncludePath(const QString& value);
    void addProjectIncludePath(const QString& value);
    void clearIncludePaths();
    void clearProjectIncludePaths();
    void clearProjectFiles();
    void fillListOfFunctions(const QString& fileName,
                             const QString& phrase,
                             int line,
                             QStringList& list);
    PStatement findAndScanBlockAt(const QString& filename, int line);
    PFileIncludes findFileIncludes(const QString &filename, bool deleteIt = false);
    QString findFirstTemplateParamOf(const QString& fileName,
                                     const QString& phrase,
                                     PStatement currentScope);
    PStatement findFunctionAt(const QString& fileName,
                            int line);
    int findLastOperator(const QString& phrase) const;
    PStatementList findNamespace(const QString& name); // return a list of PSTATEMENTS (of the namespace)
    PStatement findStatementOf(const QString& fileName,
                               const QString& phrase,
                               int line);
    PStatement findStatementOf(const QString& fileName,
                               const QString& phrase,
                               PStatement currentClass,
                               PStatement& currentClassType,
                               bool force = false);
    PStatement findStatementOf(const QString& fileName,
                               const QString& phrase,
                               PStatement currentClass,
                               bool force = false);

    StatementKind getKindOfStatement(PStatement statement);


    void parseHardDefines();
    void invalidateFile(const QString& fileName);
    QStringList getFileDirectIncludes(const QString& filename) const;
    const QList<QString>& getFileIncludes(const QString& filename) const;
    const QSet<QString>& getFileUsings(const QString& filename) &;

    bool isSystemHeaderFile(const QString& fileName);
    bool isProjectHeaderFile(const QString& fileName);
    void getSourcePair(const QString& fName, QString& CFile, QString& HFile);
    void getClassesList(QStringList& list);
    int suggestMemberInsertionLine(PStatement parentStatement,
                                   StatementClassScope Scope,
                                   bool addScopeStr);
//      {
//    function GetSystemHeaderFileName(const FileName: AnsiString): AnsiString; // <file.h>
//    function GetProjectHeaderFileName(const FileName: AnsiString): AnsiString; // <file.h>
//    function GetLocalHeaderFileName(const RelativeTo, FileName: AnsiString): AnsiString; // "file.h"
//    }
    QString getHeaderFileName(const QString& relativeTo, const QString& line) const;// both
    bool isIncludeLine(const QString &line);
    void parseFileList(bool updateView = true);
    void parseFile(const QString& fileName, bool inProject,
                   bool onlyIfNotParsed = false, bool updateView = true);
    QString statementKindStr(StatementKind value);
    QString statementClassScopeStr(StatementClassScope value);
    void reset();

    QString prettyPrintStatement(PStatement statement, int line = -1);



    StatementKind findKindOfStatementOf(const QString& fileName,
                                     const QString& phrase,
                                     int line);
    QString getHintFromStatement(const QString& fileName,
                                 const QString& phrase,
                                 int line);
    //{Find statement starting from startScope}
    PStatement findStatementStartingFrom(const QString& fileName,
                                         const QString& phrase,
                                         PStatement startScope,
                                         bool force = false);
    PStatement findTypeDefinitionOf(const QString& fileName,
                                    const QString& phrase,
                                    PStatement currentClass);
    bool freeze();  // Freeze/Lock (stop reparse while searching)
    bool freeze(const QString& serialId);  // Freeze/Lock (stop reparse while searching)
    void unFreeze(); // UnFree/UnLock (reparse while searching)
    bool getParsing();


    bool enabled() const;
    void setEnabled(bool newEnabled);

    const QSet<QString> &filesToScan() const;
    void setFilesToScan(const QSet<QString> &newFilesToScan);

signals:
    void onProgress(const QString& fileName, int total, int current);
    void onBusy();
    void onStartParsing();
    void onEndParsing(int total, int updateView);
private:
    PStatement addInheritedStatement(
            PStatement derived,
            PStatement inherit,
            StatementClassScope access);

    PStatement addChildStatement(
            // support for multiple parents (only typedef struct/union use multiple parents)
            PStatement parent,
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
            bool isStatic);
    void setInheritance(int index, PStatement classStatement, bool isStruct);
    bool isCurrentScope(const QString& command);
    void addSoloScopeLevel(PStatement statement, int line); // adds new solo level
    void removeScopeLevel(int line); // removes level
    int skipBraces(int startAt);
    int skipBracket(int startAt);
    bool checkForCatchBlock();
    bool checkForEnum();
    bool checkForForBlock();
    bool checkForKeyword();
    bool checkForMethod(QString &sType, QString &sName, QString &sArgs,
                        bool &isStatic, bool &isFriend); // caching of results
    bool checkForNamespace();
    bool checkForPreprocessor();
    bool checkForScope();
    void checkForSkipStatement();
    bool checkForStructs();
    bool checkForTypedef();
    bool checkForTypedefEnum();
    bool checkForTypedefStruct();
    bool checkForUsing();
    bool checkForVar();
    QString expandMacroType(const QString& name);
    //{procedure ResetDefines;}
    void fillListOfFunctions(const QString& fileName, int line,PStatement statement, PStatement scopeStatement, QStringList& list);
    PStatement findMemberOfStatement(
            const QString& phrase,
            PStatement scopeStatement);
    PStatement findStatementInScope(
            const QString& name,
            const QString& noNameArgs,
            StatementKind kind,
            PStatement scope);
    StatementClassScope getClassScope(int index);
    int getCurrentBlockBeginSkip();
    int getCurrentBlockEndSkip();
    int getCurrentInlineNamespaceEndSkip();
    PStatement getCurrentScope(); // gets last item from last level
    QString getFirstTemplateParam(PStatement statement, const QString& filename,
                                  const QString& phrase, PStatement currentScope);
    int getFirstTemplateParamEnd(const QString& s, int startAt);
    void getFullNamespace(
            const QString& phrase,
            QString& sNamespace,
            QString& member);
    QString getFullStatementName(
            const QString& command,
            PStatement parent);
    PStatement getIncompleteClass(
            const QString& command,
            PStatement parentScope);
    StatementScope  getScope();
    QString getStatementKey(const QString& sName,
                            const QString& sType,
                            const QString& sNoNameArgs);
    void handleCatchBlock();
    void handleEnum();
    void handleForBlock();
    void handleKeyword();
    void handleMethod(
            const QString& sType,
            const QString& sName,
            const QString& sArgs,
            bool isStatic,
            bool isFriend);
    void handleNamespace();
    void handleOtherTypedefs();
    void handlePreprocessor();
    void handleScope();
    bool handleStatement();
    void handleStructs(bool isTypedef = false);
    void handleUsing();
    void handleVar();
    void internalParse(const QString& fileName);
//    function FindMacroDefine(const Command: AnsiString): PStatement;
    void inheritClassStatement(
            PStatement derived,
            bool isStruct,
            PStatement base,
            StatementClassScope access);
    PStatement doFindStatementInScope(const QString& name,
                                      const QString& noNameArgs,
                                      StatementKind kind,
                                      PStatement scope);
    void internalInvalidateFile(const QString& fileName);
    void internalInvalidateFiles(const QStringList& files);
    void calculateFilesToBeReparsed(const QString& fileName,
                                    QStringList& files);
    int calcKeyLenForStruct(const QString& word);
//    {
//    function GetClass(const Phrase: AnsiString): AnsiString;
//    function GetMember(const Phrase: AnsiString): AnsiString;
//    function GetOperator(const Phrase: AnsiString): AnsiString;
//    function GetRemainder(const Phrase: AnsiString): AnsiString;
//    }
    void scanMethodArgs(
            PStatement functionStatement,
            const QString& argStr);
    QString splitPhrase(const QString& phrase, QString& sClazz, QString &sMember,
                QString& sOperator);

    QString removeArgNames(const QString& args);

    bool isSpaceChar(const QChar& ch);

    bool isWordChar(const QChar& ch);

    bool isLetterChar(const QChar& ch);

    bool isDigitChar(const QChar& ch);

    /*'(', ';', ':', '{', '}', '#' */
    bool isSeperator(const QChar& ch);

    /*';', '{', '}'*/
    bool isblockChar(const QChar& ch);

    /* '#', ',', ';', ':', '{', '}', '!', '/', '+', '-', '<', '>' */
    bool isInvalidVarPrefixChar(const QChar& ch);

    /*'{', '}' */
    bool isBraceChar(const QChar& ch);

    bool isLineChar(const QChar& ch);

    bool isNotFuncArgs(const QString& args);

    /**
     * @brief Test if a statement is a class/struct/union/namespace/function
     * @param kind
     * @return
     */
    bool isNamedScope(StatementKind kind);

    /**
     * @brief Test if a statement is a class/struct/union/enum/enum class/typedef
     * @param kind
     * @return
     */
    bool isTypeStatement(StatementKind kind);

    void updateSerialId();


private:
    int mParserId;
    int mSerialCount;
    QString mSerialId;
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
    //{ List of current project's file }
    QSet<QString> mProjectFiles;
    QVector<int> mBlockBeginSkips; //list of for/catch block begin token index;
    QVector<int> mBlockEndSkips; //list of for/catch block end token index;
    QVector<int> mInlineNamespaceEndSkips; // list for inline namespace end token index;
    QSet<QString> mFilesToScan; // list of base files to scan
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

    QRecursiveMutex mMutex;
    GetFileStreamCallBack mOnGetFileStream;
};

#endif // CPPPARSER_H
