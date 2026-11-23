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

    enum class BranchResult {
        isTrue,
        isFalse_but_trued,
        isFalse,
        parentIsFalse
    };

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
    };

    using FileInvalidationHandler = std::function<void(const QString&)>;
    using StopRequestedHandler = std::function<bool()>;

    explicit CppPreprocessor();
    CppPreprocessor(const CppPreprocessor&)=delete;
    CppPreprocessor& operator=(const CppPreprocessor&)=delete;

    void resetForReuse();

    void clearAnalysisResults();
    void getDefineParts(const QString& input, QString &name, QString &args, QString &value);
    void addHardDefineByLine(const QString& line);
    void setScanOptions(bool parseSystem, bool parseLocal) {
        mParseSystem = parseSystem;
        mParseLocal=parseLocal;
    }
    void preprocess(Session &session, const QString& fileName);
    void setStopRequestedHandler(const StopRequestedHandler &handler) {
        mStopRequestedHandler = handler;
    }

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

    void dumpDefinesTo(const Session &session, const QString& fileName) const;
    void dumpIncludesListTo(const QString& fileName) const;
    void addIncludePath(const QString& fileName);
    void addProjectIncludePath(const QString& fileName);
    void removeReplayRecord(const QString& filename);
    void removeReplayRecord(Session &session, const QString& filename);
    QSet<QString> calculateAffectedFiles(const QString &fileName) const;
    void invalidateFileAndDependents(const QString &fileName);
    void invalidateFileAndDependents(Session &session, const QString &fileName);

    static PDefine getDefine(const Session &session, const QString& name) {
        return session.defines.value(name,PDefine());
    }

    int cppVersion() const { return mCppVersion; }
    CppCompilerType compilerType() const { return mCompilerType; }

    QString expandMacros(QString text) const;
    QString expandMacros(const Session &session, QString text) const;

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
    void addProjectFile(const QString &fileName) { mProjectFiles.insert(fileName); }
    void removeProjectFile(const QString &fileName) { mProjectFiles.remove(fileName); }
    QString boundaryAncestor(const Session &session, const QString &fileName, int occurrence) const;

#ifdef REDPANDA_PARSER_TEST
    struct EventCounts {
        int scan = 0;
        int replay = 0;
        int skip = 0;
        int invalidate = 0;
    };
    EventCounts eventCounts(const QString &fileName) const;
    void clearEventCounts();
#endif

    static QList<PDefineArgToken> tokenizeValue(const QString& value);
    static void combineLinesEndingWithBackslash(QStringList& text);
    static void replaceCommentsBySpaceChar(QStringList& text);
    bool evaluateIf(const QString& line) const;

private:

    bool supportCPP23() const;
    bool stopRequested() const {
        return mStopPreprocessing || (mStopRequestedHandler && mStopRequestedHandler());
    }

    QString expandFunctionLikeMacro(PDefine define,const QString &args, const QSet<QString> &macrosToBeIgnored, bool &ok) const;
    bool evaluateIf(const Session &session, const QString& line) const;
    void preprocessBuffer(Session &session);
    void skipToPreprocessor(Session &session);
    QString getNextPreprocessor(Session &session);
    // Expand one logical line of the buffer with the pp-token engine;
    // returns the number of physical lines consumed.
    int expandLineWithEngine(Session &session, const QStringList& buffer, int startLine, QString& out);


    void handleDefine(Session &session, const QString& tokens);
    void handleUndefine(Session &session, const QString& tokens);
    void handleIf(Session &session, const QString& tokens);
    void handleIfdef(Session &session, const QString& tokens);
    void handleIfndef(Session &session, const QString& tokens);
    void handleElif(Session &session, const QString& tokens);
    void handleElifdef(Session &session, const QString& tokens);
    void handleElifndef(Session &session, const QString& tokens);
    void handleElse(Session &session, const QString& tokens);
    void handleEndif(Session &session, const QString& tokens);
    void handleInclude(Session &session, const QString&tokens);
    void handleIncludeNext(Session &session, const QString& tokens);
    void handlePragma(Session &session, const QString& tokens);

    void handleInclude(Session &session, const QString& line, bool fromNext);
    void handlePreprocessor(Session &session, const QString& command, const QString& tokens);
    QString removeGCCAttributes(const QString& line);
    void removeGCCAttribute(const QString&line, QString& newLine, int &i, const QString& word);

    // current file stuff
    PParsedFile getInclude(const Session &session, int index) const {
        return session.includeStack[index];
    }
    void openInclude(Session &session, QString fileName);
    void closeInclude(Session &session);

    // branch stuff
    BranchResult getCurrentBranch(const Session &session){
        if (!session.branchResults.isEmpty())
            return session.branchResults.last();
        else
            return BranchResult::isTrue;
    }
    BranchResult calcElseBranchResult(BranchResult oldResult);
    bool sameResultWithCurrentBranch(const Session &session, BranchResult value) {
        return (getCurrentBranch(session)==BranchResult::isTrue && value == BranchResult::isTrue)
                || (getCurrentBranch(session)!=BranchResult::isTrue && value != BranchResult::isTrue);
    }
    void setCurrentBranch(Session &session, BranchResult value){
        if (!sameResultWithCurrentBranch(session, value)) {
            session.includeStack.back()->branchVisibility.insert(
                    session.index+1,value==BranchResult::isTrue);
        }
        session.branchResults.append(value);
    }
    void removeCurrentBranch(Session &session){
        BranchResult value = getCurrentBranch(session);
        if (session.branchResults.size()>0) {
            session.branchResults.pop_back();
        }
        if (!sameResultWithCurrentBranch(session, value)) {
            session.includeStack.back()->branchVisibility.insert(
                    session.index+1,getCurrentBranch(session)==BranchResult::isTrue);
        }
    }
    void addDefinesInFile(Session &session, const QString& fileName);
    void addDefineByParts(Session *session, const QString &fileName, const QString& name, const QString& args,
                          const QString& value, bool hardCoded);
    void addDefineByLine(const QString& line, bool hardCoded);
    PDefine getHardDefine(const QString& name){
        return mHardDefines.value(name,PDefine());
    };
    void invalidDefinesInFile(Session &session, const QString& fileName);
    static QString signatureOfDefine(const PDefine& define);
    QHash<QString,QString> userMacroSnapshot(const Session &session) const;
    bool hasChangedFiles(const QString &fileName, const QSet<QString> &includes) const;
    bool isIncludeProtected(const QString &fileName) const;
    PReplayRecord replayRecordForFile(const Session &session, const QString &fileName) const;


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

    QVector<PpToken> expandConditionTokens(const Session &session, const QString &line) const;
private:
    struct Registry {
        QHash<QString, PParsedFileInfo> fileInfos;
        QHash<QString, PReplayRecord> replayRecords;
        QSet<QString> pendingInvalidations;

        void clearAnalysisResults() {
            fileInfos.clear();
            replayRecords.clear();
            pendingInvalidations.clear();
        }
    };

    Registry mRegistry;

    //option data for the parser
    //{ List of current project's include path }
    DefineMap mHardDefines; // set by "cpp -dM -E -xc NUL"
    QSet<QString> mProjectFiles;
    QSet<QString> mProjectIncludePaths;
    //we also need include paths in order (for #include_next)
    QList<QString> mIncludePathList;
    QList<QString> mProjectIncludePathList;
    //{ List of current compiler set's include path}
    QSet<QString> mIncludePaths;

    using PreprocessorHandler = std::function<void(Session&, const QString&)>;
    QMap<QString, PreprocessorHandler> mPreprocessorHandlers;

    bool mParseSystem;
    bool mParseLocal;
    int mCppVersion = CppVersion::Cpp17;
    CppCompilerType mCompilerType = CppCompilerType::GCC;

    bool mStopPreprocessing = false;
    int mMaxIncludeNestingDepth = DEFAULT_MAX_INCLUDE_NESTING_DEPTH;

    GetFileStreamFunc mOnGetFileStream;
    FileInvalidationHandler mFileInvalidationHandler;
    StopRequestedHandler mStopRequestedHandler;

#ifdef REDPANDA_PARSER_TEST
    QHash<QString, int> mScanCounts;
    QHash<QString, int> mReplayCounts;
    QHash<QString, int> mSkipCounts;
    QHash<QString, int> mInvalidateCounts;
#endif
};

using PCppPreprocessor = std::shared_ptr<CppPreprocessor>;

#endif // CPPPREPROCESSOR_H
