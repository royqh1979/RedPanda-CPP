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
#ifndef CPPPREPROCESSOR_H
#define CPPPREPROCESSOR_H

#include <QObject>
#include <QTextStream>
#include "parserutils.h"

enum class DefineArgTokenType{
    Symbol,
    Identifier,
    Space,
    Sharp,
    DSharp,
    Other
};
struct DefineArgToken {
    QString value;
    DefineArgTokenType type;
};
using PDefineArgToken = std::shared_ptr<DefineArgToken>;

struct ParsedFile {
    int index; // 0-based for programming convenience
    QString fileName; // Record filename, but not used now
    QStringList buffer; // do not concat them all
    int branches; //branch levels;
    QMap<int, bool> branchVisibility;
    PParsedFileInfo fileInfo;
    bool boundaryEntry = false;
    QString boundaryAncestor;
    QHash<QString,QString> userMacros;
    QStringList boundaryPrefix;
    int guardBranchDepth = -1;
    QString includeGuardToken;
    bool includeGuardDefined = false;
    bool guardClosed = false;
    bool contentAfterGuard = false;
};

using PParsedFile = std::shared_ptr<ParsedFile>;

class CppPreprocessor
{
    enum class ContentType {
        AnsiCComment,
        CppComment,
        String,
        Character,
        EscapeSequence,
        RawStringPrefix,
        RawString,
        Other
    };
public:

    using FileInvalidationHandler = std::function<void(const QString&)>;

    explicit CppPreprocessor();
    CppPreprocessor(const CppPreprocessor&)=delete;
    CppPreprocessor& operator=(const CppPreprocessor&)=delete;

    void clear();

    void clearTempResults();
    void getDefineParts(const QString& input, QString &name, QString &args, QString &value);
    void addHardDefineByLine(const QString& line);
    void setScanOptions(bool parseSystem, bool parseLocal) {
        mParseSystem = parseSystem;
        mParseLocal=parseLocal;
    }
    void preprocess(const QString& fileName);
    void stopForParserReset() {mStopForParserReset = true;}

    // Safety net for unconditional include cycles (a user error): real
    // compilers recurse until a nesting limit and then error out; we
    // skip the include at this depth instead. Depth-bounded intentional
    // patterns (Boost-style iteration) terminate by branch evaluation
    // and never reach it.
    static constexpr int DEFAULT_MAX_INCLUDE_NESTING_DEPTH = 200;
    static constexpr qint64 INCLUDE_SCAN_WARNING_THRESHOLD = 1000;
    static constexpr qint64 INCLUDE_SCAN_STOP_THRESHOLD = 10000;
    int maxIncludeNestingDepth() const { return mMaxIncludeNestingDepth; }
    void setMaxIncludeNestingDepth(int depth) { mMaxIncludeNestingDepth = depth; }

    void dumpDefinesTo(const QString& fileName) const;
    void dumpIncludesListTo(const QString& fileName) const;
    void addIncludePath(const QString& fileName);
    void addProjectIncludePath(const QString& fileName);
    void clearIncludePaths() {
        mIncludePaths.clear();
        mIncludePathList.clear();
    }
    void clearProjectIncludePaths() {
        mProjectIncludePaths.clear();
        mProjectIncludePathList.clear();
    }
    void removeReplayRecord(const QString& filename);
    QSet<QString> calculateAffectedFiles(const QString &fileName) const;
    void invalidateFile(const QString &fileName) { invalidateFileAndDependents(fileName); }

    PDefine getDefine(const QString& name) const{
        return mSession.defines.value(name,PDefine());
    }

    int cppVersion() const { return mCppVersion; }

    QString expandMacros(QString text) const;

    const QStringList& result() const{
        return mSession.result;
    };

    PParsedFileInfo findFileInfo(const QString& fileName) const {
        return mRegistry.fileInfos.value(fileName);
    }

    PReplayRecord findReplayRecord(const QString &fileName) const {
        return mRegistry.replayRecords.value(fileName);
    }

    void removeFileInfo(const QString& fileName) {
        mRegistry.fileInfos.remove(fileName);
    }

    bool hasReplayRecord(const QString& fileName) const {
        return mRegistry.replayRecords.contains(fileName);
    }

    bool includesFile(const QString &contextFileName, const QString &fileName) const {
        PParsedFileInfo contextInfo = findFileInfo(contextFileName);
        return contextInfo && contextInfo->includes().contains(fileName);
    }

    const QSet<QString>& includePaths() const {
        return mIncludePaths;
    }

    QSet<QString> replayRootFiles() const {
        const QStringList files = mRegistry.replayRecords.keys();
        return QSet<QString>(files.begin(), files.end());
    }

    const QSet<QString> &projectIncludePaths() const {
        return mProjectIncludePaths;
    }

    const DefineMap &hardDefines() const { return mHardDefines; }

    const QList<QString> &includePathList() const { return mIncludePathList; }

    const QList<QString> &projectIncludePathList() const { return mProjectIncludePathList; }
    void setOnGetFileStream(const GetFileStreamFunc &newOnGetFileStream) { mOnGetFileStream = newOnGetFileStream; }
    void setFileInvalidationHandler(const FileInvalidationHandler &handler) { mFileInvalidationHandler = handler; }
    void addProjectFile(const QString &fileName) { mRegistry.projectFiles.insert(fileName); }
    void removeProjectFile(const QString &fileName) { mRegistry.projectFiles.remove(fileName); }
    void clearProjectFiles() { mRegistry.projectFiles.clear(); }
    QString boundaryAncestor(const QString &fileName, int occurrence) const;
    const QHash<QString,QStringList> &boundaryAncestors() const { return mSession.boundaryAncestors; }

#ifdef REDPANDA_PARSER_TEST
    struct EventCounts {
        int scan = 0;
        int replay = 0;
        int skip = 0;
        int invalidate = 0;
    };
    EventCounts eventCounts(const QString &fileName) const;
    void clearEventCounts();
    QStringList currentBoundaryPrefix() const { return mSession.boundaryPrefix; }
#endif

    static QList<PDefineArgToken> tokenizeValue(const QString& value);
    static void combineLinesEndingWithBackslash(QStringList& text);
    static void replaceCommentsBySpaceChar(QStringList& text);
    bool evaluateIf(const QString& line) const;

private:

    enum class BranchResult {
        isTrue,   /* This branch is true */
        isFalse_but_trued, /* This branch is false, but a previous branch is true */
        isFalse, /* This branch and all previous branches is false */
        parentIsFalse
    };

    bool supportCPP23() const;

    QString expandFunctionLikeMacro(PDefine define,const QString &args, const QSet<QString> &macrosToBeIgnored, bool &ok) const;
    void preprocessBuffer();
    void skipToPreprocessor();
    QString getNextPreprocessor();
    // Expand one logical line of the buffer with the pp-token engine;
    // returns the number of physical lines consumed.
    int expandLineWithEngine(const QStringList& buffer, int startLine, QString& out);


    void handleDefine(const QString& tokens);
    void handleUndefine(const QString& tokens);
    void handleIf(const QString& tokens);
    void handleIfdef(const QString& tokens);
    void handleIfndef(const QString& tokens);
    void handleElif(const QString& tokens);
    void handleElifdef(const QString& tokens);
    void handleElifndef(const QString& tokens);
    void handleElse(const QString& tokens);
    void handleEndif(const QString& tokens);
    void handleInclude(const QString&tokens);
    void handleIncludeNext(const QString& tokens);
    void handlePragma(const QString& tokens);

    void handleInclude(const QString& line, bool fromNext);
    void handlePreprocessor(const QString& command, const QString& tokens);
    QString removeGCCAttributes(const QString& line);
    void removeGCCAttribute(const QString&line, QString& newLine, int &i, const QString& word);

    // current file stuff
    PParsedFile getInclude(int index) const {
        return mSession.includeStack[index];
    }
    void openInclude(QString fileName);
    void closeInclude();

    // branch stuff
    BranchResult getCurrentBranch(){
        if (!mSession.branchResults.isEmpty())
            return mSession.branchResults.last();
        else
            return BranchResult::isTrue;
    }
    BranchResult calcElseBranchResult(BranchResult oldResult);
    bool sameResultWithCurrentBranch(BranchResult value) {
        return (getCurrentBranch()==BranchResult::isTrue && value == BranchResult::isTrue)
                || (getCurrentBranch()!=BranchResult::isTrue && value != BranchResult::isTrue);
    }
    void setCurrentBranch(BranchResult value){
        if (!sameResultWithCurrentBranch(value)) {
            mSession.includeStack.back()->branchVisibility.insert(
                    mSession.index+1,value==BranchResult::isTrue);
        }
        mSession.branchResults.append(value);
    }
    void removeCurrentBranch(){
        BranchResult value = getCurrentBranch();
        if (mSession.branchResults.size()>0) {
            mSession.branchResults.pop_back();
        }
        if (!sameResultWithCurrentBranch(value)) {
            mSession.includeStack.back()->branchVisibility.insert(
                    mSession.index+1,getCurrentBranch()==BranchResult::isTrue);
        }
    }
    void addDefinesInFile(const QString& fileName);
    void addDefineByParts(const QString& name, const QString& args,
                          const QString& value, bool hardCoded);
    void addDefineByLine(const QString& line, bool hardCoded);
    PDefine getHardDefine(const QString& name){
        return mHardDefines.value(name,PDefine());
    };
    void invalidDefinesInFile(const QString& fileName);
    static QString signatureOfDefine(const PDefine& define);
    QHash<QString,QString> userMacroSnapshot() const;
    bool hasChangedFiles(const QString &fileName, const QSet<QString> &includes) const;
    bool isIncludeProtected(const QString &fileName) const;
    PReplayRecord replayRecordForFile(const QString &fileName) const;
    void invalidateFileAndDependents(const QString &fileName);


    /*
     * '_','a'..'z','A'..'Z','0'..'9'
     */
    static  bool isWordChar(const QChar& ch) {
        return (ch=='_'
                || ch.isLetter()
                || (ch>='0' && ch<='9'));
    }

    /*
     *  '\t' ' '
     */
    static  bool isSpaceChar(const QChar& ch) { return ch == ' ' || ch == '\t'; }

    /*
     * '0'..'9'
     */
    static  bool isDigit(const QChar& ch) { return (ch>='0' && ch<='9'); }

    QString lineBreak() { return "\n"; }

    QVector<PpToken> expandConditionTokens(const QString &line) const;
private:
    struct Session {
        int index = 0;
        QString fileName;
        QStringList buffer;
        QStringList result;
        PParsedFileInfo currentFileInfo;
        int preProcIndex = 0;
        QList<PParsedFile> includeStack;
        QList<BranchResult> branchResults;
        DefineMap defines;
        QSet<QString> processed;
        qint64 scannedIncludeCount = 0;
        bool fileJustOpenned = false;
        QHash<QString, QStringList> fileCache;
        QStringList boundaryPrefix;
        bool allBoundaryEntriesReplayed = true;
        QHash<QString, QStringList> boundaryAncestors;

        void clear() {
            fileName.clear();
            buffer.clear();
            result.clear();
            currentFileInfo.reset();
            fileJustOpenned = false;
            fileCache.clear();
            includeStack.clear();
            branchResults.clear();
            processed.clear();
            scannedIncludeCount = 0;
            boundaryPrefix.clear();
            allBoundaryEntriesReplayed = true;
            boundaryAncestors.clear();
        }
    };

    struct Registry {
        QHash<QString, PParsedFileInfo> fileInfos;
        QHash<QString, PReplayRecord> replayRecords;
        QSet<QString> projectFiles;
        QSet<QString> pendingInvalidations;

        void clear() {
            fileInfos.clear();
            replayRecords.clear();
            projectFiles.clear();
            pendingInvalidations.clear();
        }
    };

    Session mSession;
    Registry mRegistry;

    //option data for the parser
    //{ List of current project's include path }
    DefineMap mHardDefines; // set by "cpp -dM -E -xc NUL"
    QSet<QString> mProjectIncludePaths;
    //we also need include paths in order (for #include_next)
    QList<QString> mIncludePathList;
    QList<QString> mProjectIncludePathList;
    //{ List of current compiler set's include path}
    QSet<QString> mIncludePaths;

    QMap<QString, std::function<void(const QString&)>> mPreprocessorHandlers;

    bool mParseSystem;
    bool mParseLocal;
    int mCppVersion = CppVersion::Cpp17;

    bool mStopForParserReset;
    int mMaxIncludeNestingDepth = DEFAULT_MAX_INCLUDE_NESTING_DEPTH;

    GetFileStreamFunc mOnGetFileStream;
    FileInvalidationHandler mFileInvalidationHandler;

#ifdef REDPANDA_PARSER_TEST
    QHash<QString, int> mScanCounts;
    QHash<QString, int> mReplayCounts;
    QHash<QString, int> mSkipCounts;
    QHash<QString, int> mInvalidateCounts;
#endif
};

using PCppPreprocessor = std::shared_ptr<CppPreprocessor>;

#endif // CPPPREPROCESSOR_H
