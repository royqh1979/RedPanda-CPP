/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CPPPARSER_H
#define CPPPARSER_H

#include <QMutex>
#include <QObject>
#include <QThread>
#include <QVector>
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
    void addFileToScan(const QString& value, bool inProject = false);
    void addIncludePath(const QString& value);
    void addProjectIncludePath(const QString& value);
    void clearIncludePaths();
    void clearProjectIncludePaths();
    void clearProjectFiles();
    QList<PStatement> getListOfFunctions(const QString& fileName,
                             const QString& phrase,
                             int line);
    PStatement findAndScanBlockAt(const QString& filename, int line) {
        QMutexLocker locker(&mMutex);
        if (mParsing) {
            return PStatement();
        }
        PFileIncludes fileIncludes = mPreprocessor.includesList().value(filename);
        if (!fileIncludes)
            return PStatement();

        PStatement statement = fileIncludes->scopes.findScopeAtLine(line);
        return statement;
    }
    PFileIncludes findFileIncludes(const QString &filename, bool deleteIt = false) {
        QMutexLocker locker(&mMutex);
        PFileIncludes fileIncludes = mPreprocessor.includesList().value(filename,PFileIncludes());
        if (deleteIt && fileIncludes)
            mPreprocessor.includesList().remove(filename);
        return fileIncludes;
    }
    QString findFirstTemplateParamOf(const QString& fileName,
                                     const QString& phrase,
                                     const PStatement& currentScope);
    PStatement findFunctionAt(const QString& fileName,
                            int line);
    int findLastOperator(const QString& phrase) const;
    PStatementList findNamespace(const QString& name); // return a list of PSTATEMENTS (of the namespace)
    PStatement findStatement(const QString& fullname);
    PStatement findStatementOf(const QString& fileName,
                               const QString& phrase,
                               int line);
    PStatement findStatementOf(const QString& fileName,
                               const QString& phrase,
                               const PStatement& currentScope,
                               PStatement& parentScopeType,
                               bool force = false);
    PStatement findStatementOf(const QString& fileName,
                               const QString& phrase,
                               const PStatement& currentClass,
                               bool force = false);

    PStatement findStatementOf(const QString& fileName,
                               const QStringList& expression,
                               const PStatement& currentScope);
    PStatement findStatementOf(const QString& fileName,
                               const QStringList& expression,
                               int line);
    PStatement findAliasedStatement(const PStatement& statement);

    /**
     * @brief evaluate the expression
     * @param fileName
     * @param expression
     * @param currentScope
     * @return the statement of the evaluation result
     */
    PEvalStatement evalExpression(const QString& fileName,
                               const QStringList& expression,
                               const PStatement& currentScope);
    PStatement findTypeDefinitionOf(const QString& fileName,
                                    const QString& aType,
                                    const PStatement& currentClass);
    PStatement findTypeDef(const PStatement& statement,
                          const QString& fileName);
    bool freeze();  // Freeze/Lock (stop reparse while searching)
    bool freeze(const QString& serialId);  // Freeze/Lock (stop reparse while searching)
    QStringList getClassesList();
    QSet<QString> getFileDirectIncludes(const QString& filename);
    QSet<QString> getFileIncludes(const QString& filename);
    QSet<QString> getFileUsings(const QString& filename);

    QString getHeaderFileName(const QString& relativeTo, const QString& line);// both

    void invalidateFile(const QString& fileName);
    bool isIncludeLine(const QString &line);
    bool isProjectHeaderFile(const QString& fileName);
    bool isSystemHeaderFile(const QString& fileName);
    void parseFile(const QString& fileName, bool inProject,
                   bool onlyIfNotParsed = false, bool updateView = true);
    void parseFileList(bool updateView = true);
    void parseHardDefines();
    bool parsing() const;
    void reset();
    void unFreeze(); // UnFree/UnLock (reparse while searching)
    QSet<QString> scannedFiles();

    QString prettyPrintStatement(const PStatement& statement, const QString& filename, int line = -1);

    bool enabled() const;
    void setEnabled(bool newEnabled);

    const QSet<QString> &filesToScan() const;
    void setFilesToScan(const QSet<QString> &newFilesToScan);

    void setOnGetFileStream(const GetFileStreamCallBack &newOnGetFileStream);

    int parserId() const;

    const QString &serialId() const;

    bool parseLocalHeaders() const;
    void setParseLocalHeaders(bool newParseLocalHeaders);

    bool parseGlobalHeaders() const;
    void setParseGlobalHeaders(bool newParseGlobalHeaders);

    const QSet<QString>& includePaths();
    const QSet<QString>& projectIncludePaths();

    const StatementModel &statementList() const;

signals:
    void onProgress(const QString& fileName, int total, int current);
    void onBusy();
    void onStartParsing();
    void onEndParsing(int total, int updateView);
private:
    PStatement addInheritedStatement(
            const PStatement& derived,
            const PStatement& inherit,
            StatementClassScope access);

    PStatement addChildStatement(
            // support for multiple parents (only typedef struct/union use multiple parents)
            const PStatement& parent,
            const QString& fileName,
            const QString& aType, // "Type" is already in use
            const QString& command,
            const QString& args,
            const QString& value,
            int line,
            StatementKind kind,
            const StatementScope& scope,
            const StatementClassScope& classScope,
            bool isDefinition,
            bool isStatic); // TODO: InheritanceList not supported
    PStatement addStatement(
            const PStatement& parent,
            const QString &fileName,
            const QString &aType, // "Type" is already in use
            const QString &command,
            const QString &args,
            const QString& value,
            int line,
            StatementKind kind,
            const StatementScope& scope,
            const StatementClassScope& classScope,
            bool isDefinition,
            bool isStatic);
    void setInheritance(int index, const PStatement& classStatement, bool isStruct);
    bool isCurrentScope(const QString& command);
    void addSoloScopeLevel(PStatement& statement, int line, bool shouldResetBlock = true); // adds new solo level
    void removeScopeLevel(int line); // removes level
    int skipBraces(int startAt);
    int skipBracket(int startAt);

    void internalClear();

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
    void fillListOfFunctions(const QString& fileName, int line,
                             const PStatement& statement,
                             const PStatement& scopeStatement, QStringList& list);
    QList<PStatement> getListOfFunctions(const QString& fileName, int line,
                                         const PStatement& statement,
                                         const PStatement& scopeStatement);
    PStatement findMemberOfStatement(
            const QString& phrase,
            const PStatement& scopeStatement);
    QList<PStatement> findMembersOfStatement(const QString& phrase,
                                             const PStatement& scopeStatement);
    PStatement findStatementInScope(
            const QString& name,
            const QString& noNameArgs,
            StatementKind kind,
            const PStatement& scope);
    PStatement findStatementInScope(
            const QString& name,
            const PStatement& scope) {
        if (!scope)
            return findMemberOfStatement(name,scope);
        if (scope->kind == StatementKind::skNamespace) {
            return findStatementInNamespace(name, scope->fullName);
        } else {
            return findMemberOfStatement(name,scope);
        }
    }
    PStatement findStatementInNamespace(
            const QString& name,
            const QString& namespaceName);

    //{Find statement starting from startScope}
    PStatement findStatementStartingFrom(const QString& fileName,
                                         const QString& phrase,
                                         const PStatement& startScope);


    /**
     * @brief evaluate the expression (starting from pos) in the scope
     * @param fileName
     * @param phraseExpression
     * @param pos
     * @param scope
     * @param previousResult the result of evalution for expression from 0 to pos-1
     * @param freeScoped if the expression left is
     * @return
     */
    PEvalStatement doEvalExpression(const QString& fileName,
                               const QStringList& phraseExpression,
                               int &pos,
                               const PStatement& scope,
                               const PEvalStatement& previousResult,
                               bool freeScoped);

    PEvalStatement doEvalPointerArithmetic(
            const QString& fileName,
            const QStringList& phraseExpression,
            int &pos,
            const PStatement& scope,
            const PEvalStatement& previousResult,
            bool freeScoped);
    PEvalStatement doEvalPointerToMembers(
            const QString& fileName,
            const QStringList& phraseExpression,
            int &pos,
            const PStatement& scope,
            const PEvalStatement& previousResult,
            bool freeScoped);
    PEvalStatement doEvalCCast(
            const QString& fileName,
            const QStringList& phraseExpression,
            int &pos,
            const PStatement& scope,
            const PEvalStatement& previousResult,
            bool freeScoped);
    PEvalStatement doEvalMemberAccess(
            const QString& fileName,
            const QStringList& phraseExpression,
            int &pos,
            const PStatement& scope,
            const PEvalStatement& previousResult,
            bool freeScoped);
    PEvalStatement doEvalScopeResolution(
            const QString& fileName,
            const QStringList& phraseExpression,
            int &pos,
            const PStatement& scope,
            const PEvalStatement& previousResult,
            bool freeScoped);
    PEvalStatement doEvalTerm(
            const QString& fileName,
            const QStringList& phraseExpression,
            int &pos,
            const PStatement& scope,
            const PEvalStatement& previousResult,
            bool freeScoped);

    PEvalStatement doCreateEvalNamespace(const PStatement& namespaceStatement);

    PEvalStatement doCreateEvalType(const QString& fileName,const PStatement& typeStatement);
    PEvalStatement doCreateEvalType(const QString& primitiveType);

    PEvalStatement doCreateEvalVariable(const QString& fileName, PStatement varStatement);
    PEvalStatement doCreateEvalFunction(const QString& fileName, PStatement funcStatement);
    PEvalStatement doCreateEvalLiteral(const QString& type);
    void  doSkipInExpression(const QStringList& expression, int&pos, const QString& startSymbol, const QString& endSymbol);

    bool isIdentifier(const QString& token) const {
        return (!token.isEmpty() && isLetterChar(token.front())
                && !token.contains('\"'));
    }

    bool isIntegerLiteral(const QString& token) const {
        if (token.isEmpty())
            return false;
        QChar ch = token.front();
        return (ch>='0' && ch<='9' && !token.contains(".") && !token.contains("e"));
    }
    bool isFloatLiteral(const QString& token) const {
        if (token.isEmpty())
            return false;
        QChar ch = token.front();
        return (ch>='0' && ch<='9' && (token.contains(".") || token.contains("e")));
    }
    bool isStringLiteral(const QString& token) const {
        if (token.isEmpty())
            return false;
        return (!token.startsWith('\'') && token.contains('"'));
    }

    bool isCharLiteral(const QString& token) const{
        if (token.isEmpty())
            return false;
        return (token.startsWith('\''));
    }
    PStatement doParseEvalTypeInfo(
            const QString& fileName,
            const PStatement& scope,
            const QString& type,
            QString& baseType,
            int& pointerLevel);

    int getBracketEnd(const QString& s, int startAt);
    StatementClassScope getClassScope(int index);
    int getCurrentBlockBeginSkip();
    int getCurrentBlockEndSkip();
    int getCurrentInlineNamespaceEndSkip();
    PStatement getCurrentScope(); // gets last item from last level
    QString getFirstTemplateParam(const PStatement& statement, const QString& filename,
                                  const QString& phrase, const PStatement& currentScope);
    int getFirstTemplateParamEnd(const QString& s, int startAt);

    void getFullNamespace(
            const QString& phrase,
            QString& sNamespace,
            QString& member);
    QString getFullStatementName(
            const QString& command,
            const PStatement& parent);
    PStatement getIncompleteClass(
            const QString& command,
            const PStatement& parentScope);
    QString getScopePrefix(const PStatement& statement);
    StatementScope  getScope();
    QString getStatementKey(const QString& sName,
                            const QString& sType,
                            const QString& sNoNameArgs);
    PStatement getTypeDef(const PStatement& statement,
                          const QString& fileName, const QString& aType);
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
            const PStatement& derived,
            bool isStruct,
            const PStatement& base,
            StatementClassScope access);
    PStatement doFindStatementInScope(const QString& name,
                                      const QString& noNameArgs,
                                      StatementKind kind,
                                      const PStatement& scope);
    void internalInvalidateFile(const QString& fileName);
    void internalInvalidateFiles(const QSet<QString>& files);
    QSet<QString> calculateFilesToBeReparsed(const QString& fileName);
    int calcKeyLenForStruct(const QString& word);
//    {
//    function GetClass(const Phrase: AnsiString): AnsiString;
//    function GetMember(const Phrase: AnsiString): AnsiString;
//    function GetOperator(const Phrase: AnsiString): AnsiString;
//    function GetRemainder(const Phrase: AnsiString): AnsiString;
//    }
    void scanMethodArgs(
            const PStatement& functionStatement,
            const QString& argStr);
    QString splitPhrase(const QString& phrase, QString& sClazz, QString &sMember,
                QString& sOperator);

    QString removeArgNames(const QString& args);

    bool isSpaceChar(const QChar& ch) const {
        return ch==' ' || ch =='\t';
    }

    bool isWordChar(const QChar& ch) const {
        return ch.isLetter()
                || ch == '_'
                || ch == '*'
                || ch == '&';
    }

    bool isLetterChar(const QChar& ch) const {
        return ch.isLetter()
                || ch == '_';
    }

    bool isDigitChar(const QChar& ch) const {
        return (ch>='0' && ch<='9');
    }

    /*'(', ';', ':', '{', '}', '#' */
    bool isSeperator(const QChar& ch) const {
        switch(ch.unicode()){
        case '(':
        case ';':
        case ':':
        case '{':
        case '}':
        case '#':
            return true;
        default:
            return false;
        }
    }

    /*';', '{', '}'*/
    bool isblockChar(const QChar& ch) const {
        switch(ch.unicode()){
        case ';':
        case '{':
        case '}':
            return true;
        default:
            return false;
        }
    }

    /* '#', ',', ';', ':', '{', '}', '!', '/', '+', '-', '<', '>' */
    bool isInvalidVarPrefixChar(const QChar& ch) const {
        switch (ch.unicode()) {
        case '#':
        case ',':
        case ';':
        case ':':
        case '{':
        case '}':
        case '!':
        case '/':
        case '+':
        case '-':
        case '<':
        case '>':
            return true;
        default:
            return false;
        }
    }

    /*'{', '}' */
    bool isBraceChar(const QChar& ch) const {
        return ch == '{' || ch =='}';
    }

    bool isLineChar(const QChar& ch) const {
        return ch=='\n' || ch=='\r';
    }

    bool isNotFuncArgs(const QString& args);

    /**
     * @brief Test if a statement is a class/struct/union/namespace/function
     * @param kind
     * @return
     */
    bool isNamedScope(StatementKind kind) const;

    /**
     * @brief Test if a statement is a class/struct/union/enum/enum class/typedef
     * @param kind
     * @return
     */
    bool isTypeStatement(StatementKind kind) const;

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
    //It's used in preprocessor, so we can't use fIncludeList instead

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
    QSet<QString> mInlineNamespaces;
    //fRemovedStatements: THashedStringList; //THashedStringList<String,PRemovedStatements>

    QMutex mMutex;
    GetFileStreamCallBack mOnGetFileStream;
    QMap<QString,SkipType> mCppKeywords;
    QSet<QString> mCppTypeKeywords;
};
using PCppParser = std::shared_ptr<CppParser>;

class CppFileParserThread : public QThread {
    Q_OBJECT
public:
    explicit CppFileParserThread(
            PCppParser parser,
            QString fileName,
            bool inProject,
            bool onlyIfNotParsed = false,
            bool updateView = true,
            QObject *parent = nullptr);

private:
    PCppParser mParser;
    QString mFileName;
    bool mInProject;
    bool mOnlyIfNotParsed;
    bool mUpdateView;

    // QThread interface
protected:
    void run() override;
};
using PCppParserThread = std::shared_ptr<CppFileParserThread>;

class CppFileListParserThread: public QThread {
    Q_OBJECT
public:
    explicit CppFileListParserThread(
            PCppParser parser,
            bool updateView = true,
            QObject *parent = nullptr);
private:
    PCppParser mParser;
    bool mUpdateView;
    // QThread interface
protected:
    void run() override;
};

void parseFile(
    PCppParser parser,
    const QString& fileName,
    bool inProject,
    bool onlyIfNotParsed = false,
    bool updateView = true);

void parseFileList(
        PCppParser parser,
        bool updateView = true);


#endif // CPPPARSER_H
