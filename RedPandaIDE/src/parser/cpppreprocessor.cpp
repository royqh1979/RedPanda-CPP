/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#include "cpppreprocessor.h"
#include "ppexpander.h"

#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QMultiHash>
#include <qt_utils/utils.h>

namespace {
struct PreprocessorProfile {
    bool enabled = qEnvironmentVariableIsSet("REDPANDA_PARSER_PROFILE");
    qint64 openIncludeNs = 0;
    qint64 closeIncludeNs = 0;
    qint64 insideBoundaryNs = 0;
    qint64 fileStampsNs = 0;
    qint64 includeRelationsNs = 0;
    qint64 readFileNs = 0;
    qint64 handleDefineNs = 0;
    qint64 handleIncludeNs = 0;
    qint64 replaceCommentsNs = 0;
    qint64 expandLineNs = 0;
    qint64 evaluateIfNs = 0;
    qint64 openIncludeCalls = 0;
    qint64 closeIncludeCalls = 0;
    qint64 insideBoundaryCalls = 0;
    qint64 fileStampsCalls = 0;
    qint64 includeRelationsCalls = 0;
    qint64 readFileCalls = 0;
    qint64 handleDefineCalls = 0;
    qint64 handleIncludeCalls = 0;
    qint64 replaceCommentsCalls = 0;
    qint64 expandLineCalls = 0;
    qint64 evaluateIfCalls = 0;
    ~PreprocessorProfile()
    {
        if (!enabled)
            return;
        qWarning() << "PREPROCESSOR_PROFILE"
                   << "openInclude" << openIncludeNs / 1000000.0
                   << "closeInclude" << closeIncludeNs / 1000000.0
                   << "insideBoundary" << insideBoundaryNs / 1000000.0
                   << "fileStamps" << fileStampsNs / 1000000.0
                   << "includeRelations" << includeRelationsNs / 1000000.0
                   << "readFile" << readFileNs / 1000000.0
                   << "handleDefine" << handleDefineNs / 1000000.0
                   << "handleInclude" << handleIncludeNs / 1000000.0
                   << "replaceComments" << replaceCommentsNs / 1000000.0
                   << "expandLine" << expandLineNs / 1000000.0
                   << "evaluateIf" << evaluateIfNs / 1000000.0;
        qWarning() << "PREPROCESSOR_PROFILE_CALLS"
                   << "openInclude" << openIncludeCalls
                   << "closeInclude" << closeIncludeCalls
                   << "insideBoundary" << insideBoundaryCalls
                   << "fileStamps" << fileStampsCalls
                   << "includeRelations" << includeRelationsCalls
                   << "readFile" << readFileCalls
                   << "handleDefine" << handleDefineCalls
                   << "handleInclude" << handleIncludeCalls
                   << "replaceComments" << replaceCommentsCalls
                   << "expandLine" << expandLineCalls
                   << "evaluateIf" << evaluateIfCalls;
    }
};

PreprocessorProfile preprocessorProfile;

struct PreprocessorProfileScope {
    PreprocessorProfileScope(qint64 &target, qint64 &calls)
        : target(target), calls(calls)
    {
        if (preprocessorProfile.enabled) {
            ++calls;
            timer.start();
        }
    }
    ~PreprocessorProfileScope()
    {
        if (preprocessorProfile.enabled)
            target += timer.nsecsElapsed();
    }
    qint64 &target;
    qint64 &calls;
    QElapsedTimer timer;
};
}

CppPreprocessor::CppPreprocessor()
{
    mPreprocessorHandlers.insert("if",[this](const QString& tokens){ handleIf(tokens);});
    mPreprocessorHandlers.insert("ifdef",[this](const QString& tokens){ handleIfdef(tokens);});
    mPreprocessorHandlers.insert("ifndef",[this](const QString& tokens){ handleIfndef(tokens);});
    mPreprocessorHandlers.insert("elif",[this](const QString& tokens){ handleElif(tokens);});
    mPreprocessorHandlers.insert("elifdef",[this](const QString& tokens){ handleElifdef(tokens);});
    mPreprocessorHandlers.insert("elifndef",[this](const QString& tokens){ handleElifndef(tokens);});
    mPreprocessorHandlers.insert("else",[this](const QString& tokens){ handleElse(tokens);});
    mPreprocessorHandlers.insert("endif",[this](const QString& tokens){ handleEndif(tokens);});
    mPreprocessorHandlers.insert("define",[this](const QString& tokens){ handleDefine(tokens);});
    mPreprocessorHandlers.insert("undef",[this](const QString& tokens){ handleUndefine(tokens);});
    mPreprocessorHandlers.insert("include",[this](const QString& tokens){ handleInclude(tokens);});
    mPreprocessorHandlers.insert("include_next",[this](const QString& tokens){ handleIncludeNext(tokens);});
    mPreprocessorHandlers.insert("pragma",[this](const QString& tokens){ handlePragma(tokens);});
    mParseLocal = true;
    mParseSystem = true;
}

void CppPreprocessor::clear()
{
    clearTempResults();
    mRegistry.clear();
    mHardDefines.clear();
    mSession.defines.clear();
    mProjectIncludePaths.clear();
    mIncludePathList.clear();
    mProjectIncludePathList.clear();
    mIncludePaths.clear();
    mCppVersion = CppVersion::Cpp17;
}

void CppPreprocessor::clearTempResults()
{
    mSession.clear();
}

QString CppPreprocessor::boundaryAncestor(const QString &fileName, int occurrence) const
{
    const QStringList ancestors = mSession.boundaryAncestors.value(fileName);
    if (occurrence < 0 || occurrence >= ancestors.count())
        return QString();
    return ancestors[occurrence];
}

bool CppPreprocessor::isInsideBoundary(const QString &fileName) const
{
    PreprocessorProfileScope profileScope(preprocessorProfile.insideBoundaryNs,
                                          preprocessorProfile.insideBoundaryCalls);
    if (mRegistry.projectFiles.contains(fileName))
        return false;
    return isSystemHeaderFile(fileName, mIncludePaths)
            || isSystemHeaderFile(fileName, mProjectIncludePaths);
}

QHash<QString,QString> CppPreprocessor::userMacroSnapshot() const
{
    QHash<QString,QString> result;
    for (auto it = mSession.defines.constBegin(); it != mSession.defines.constEnd(); ++it) {
        if (!it.value()->hardCoded)
            result.insert(it.key(), signatureOfDefine(it.value()));
    }
    return result;
}

bool CppPreprocessor::hasValidFileStamps(const PParsedFileInfo &fileInfo) const
{
    PreprocessorProfileScope profileScope(preprocessorProfile.fileStampsNs,
                                          preprocessorProfile.fileStampsCalls);
    QSet<QString> files = fileInfo->includes();
    files.insert(fileInfo->fileName());
    for (const QString &fileName : files) {
        if (!mRegistry.fileMetadata.contains(fileName))
            continue;
        const Registry::FileMetadata &stamp = mRegistry.fileMetadata[fileName];
        if (stamp.size < 0)
            continue;
        QFileInfo diskInfo(fileName);
        if (!diskInfo.exists() || diskInfo.lastModified() != stamp.lastModified
                || diskInfo.size() != stamp.size)
            return false;
    }
    return true;
}

bool CppPreprocessor::isIncludeProtected(const QString &fileName) const
{
    return mRegistry.fileMetadata.value(fileName).includeProtected;
}

PParsedFileInfo CppPreprocessor::replayRecordForFile(const QString &fileName) const
{
    PParsedFileInfo fileInfo = findFileInfo(fileName);
    if (fileInfo)
        return fileInfo;
    if (!mSession.includeStack.isEmpty()) {
        const QString &ancestor = mSession.includeStack.back()->boundaryAncestor;
        if (!ancestor.isEmpty())
            return findFileInfo(ancestor);
    }
    return PParsedFileInfo();
}

QSet<QString> CppPreprocessor::calculateAffectedFiles(const QString &fileName) const
{
    QSet<QString> affected{fileName};
    for (const QString &scannedFile : mRegistry.scannedFiles) {
        PParsedFileInfo fileInfo = mRegistry.replayRecords.value(scannedFile);
        if (fileInfo && fileInfo->including(fileName))
            affected.insert(scannedFile);
    }
    return affected;
}

void CppPreprocessor::invalidateFileAndDependents(const QString &fileName)
{
    QSet<QString> affected = calculateAffectedFiles(fileName);
    for (const PParsedFile &parsedFile : mSession.includeStack) {
        if (affected.contains(parsedFile->fileName)) {
            mRegistry.pendingInvalidations.insert(parsedFile->fileName);
            affected.remove(parsedFile->fileName);
        }
    }
    for (const QString &affectedFile : affected) {
        if (mFileInvalidationHandler)
            mFileInvalidationHandler(affectedFile);
        else
            removeScannedFile(affectedFile);
    }
}

#ifdef REDPANDA_PARSER_TEST
CppPreprocessor::EventCounts CppPreprocessor::eventCounts(const QString &fileName) const
{
    EventCounts result;
    result.scan = mScanCounts.value(fileName);
    result.replay = mReplayCounts.value(fileName);
    result.skip = mSkipCounts.value(fileName);
    result.invalidate = mInvalidateCounts.value(fileName);
    return result;
}

void CppPreprocessor::clearEventCounts()
{
    mScanCounts.clear();
    mReplayCounts.clear();
    mSkipCounts.clear();
    mInvalidateCounts.clear();
}
#endif

void CppPreprocessor::addDefineByParts(const QString &name, const QString &args, const QString &value, bool hardCoded)
{
    // Check for duplicates
    PDefine define = std::make_shared<Define>();
    define->name = name;
    define->args = args;
    define->value = value;
    define->filename = mSession.fileName;
    define->hardCoded = hardCoded;
    if (!args.isEmpty()) {
        // precompile the replacement list for the pp-token engine
        define->paramNames = splitMacroParams(
                define->args.mid(1, define->args.length()-2));
        define->replacementList = compileReplacementList(
                define->value, define->paramNames);
    }
    if (hardCoded) {
        mHardDefines.insert(name,define);
        mSession.defines.insert(name,define);
        if (name == "__cplusplus") {
            // strip integer suffixes (U, L, UL, LL, ULL) before converting
            QString v = value.trimmed();
            while (v.endsWith('U') || v.endsWith('u')
                   || v.endsWith('L') || v.endsWith('l'))
                v.chop(1);
            bool ok;
            long version = v.toLong(&ok);
            mCppVersion = ok ? static_cast<int>(version) : 0;
        }
    } else {
        PParsedFileInfo fileInfo = replayRecordForFile(mSession.fileName);
        if (fileInfo)
            fileInfo->defines()->insert(define->name,define);
        mSession.defines.insert(name,define);
    }
}

void CppPreprocessor::getDefineParts(const QString &input, QString &name, QString &args, QString &value)
{
    QString s = input.trimmed();
    name = "";
    args = "";
    value = "";

    // Rules:
    // When the character before the first opening brace is nonblank, a function is defined.
    // After that point, switch from name to args
    // The value starts after the first blank character outside of the outermost () pair

    int i = 0;
    int level = 0;
    bool isFunction = false;
    int argStart = 0;
    while (i < s.length()) {
        // When we find the first opening brace, check if this is a function define
        if (s[i] == '(') {
            level++;
            if ((level == 1) && (!isFunction)) { // found a function define!
                name = s.mid(0,i);
                argStart = i;
                isFunction = true;
            }
        } else if (s[i]==')') {
            level--;
        } else if (isFunction && (level == 0)) {
            break;
        } else if (isSpaceChar(s[i]) && (level == 0)) {
            break;
        }
        i++;
    }
    if (isFunction) {
        // Name has already been found
        args = s.mid(argStart,i-argStart);
        //todo: expand macro (if already have)
    } else {
        name = s.mid(0,i);
        args = "";
    }
    value = removeGCCAttributes(s.mid(i).trimmed());
    name.squeeze();
    value.squeeze();
    args.squeeze();
}

void CppPreprocessor::addHardDefineByLine(const QString &line)
{
    Q_ASSERT(line.startsWith('#'));
    addDefineByLine(line.mid(1),true);
}

void CppPreprocessor::addDefineByLine(const QString &line, bool hardCoded)
{
    // Remove define
    constexpr int DEFINE_LEN=6;
    QString s = line.mid(DEFINE_LEN).trimmed();

    QString name, args, value;
    // Get parts from generalized function
    getDefineParts(s, name, args, value);

    // Add to the list
    addDefineByParts(name, args, value, hardCoded);
}


void CppPreprocessor::preprocess(const QString &fileName)
{
    mStopForParserReset = false;
    clearTempResults();
    QSet<QString> pendingInvalidations = mRegistry.pendingInvalidations;
    mRegistry.pendingInvalidations.clear();
    for (const QString &pendingFile : pendingInvalidations)
        invalidateFileAndDependents(pendingFile);
    mSession.defines = mHardDefines;
    mSession.fileName = fileName;
    PParsedFileInfo oldRootInfo = findFileInfo(fileName);
    if (oldRootInfo && !hasValidFileStamps(oldRootInfo))
        invalidateFileAndDependents(fileName);
    else
        removeScannedFile(fileName);
    openInclude(fileName);
    preprocessBuffer();
}

void CppPreprocessor::invalidDefinesInFile(const QString &fileName)
{
    PParsedFileInfo fileInfo = findFileInfo(fileName);
    if (!fileInfo)
        return;
    PDefineMap defineMap = fileInfo->defines();
    if (defineMap) {
        for (const PDefine &define : *defineMap) {
            const PDefine &current = mSession.defines.value(define->name);
            if (current == define) {
                mSession.defines.remove(define->name);
                PDefine hardDefine = mHardDefines.value(define->name);
                if (hardDefine)
                    mSession.defines.insert(define->name, hardDefine);
            }
        }
        defineMap->clear();
    }
    PDefineMap undefineMap = fileInfo->undefines();
    if (undefineMap) {
        for (const PDefine &define : *undefineMap)
            mSession.defines.insert(define->name, define);
        undefineMap->clear();
    }
}

void CppPreprocessor::dumpDefinesTo(const QString &fileName) const
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        QTextStream stream(&file);
        for (const PDefine& define:mSession.defines) {
            stream<<QString("%1 %2 %3 %4\n")
                    .arg(define->name,define->args,define->value)
                    .arg(define->hardCoded)<<Qt::endl;
        }
    }
}

void CppPreprocessor::dumpIncludesListTo(const QString &fileName) const
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        QTextStream stream(&file);
        for (const PParsedFileInfo& fileInfo:mRegistry.replayRecords) {
            stream<<fileInfo->fileName()<<" : "<<Qt::endl;
            stream<<"\t**direct-includes:**"<<Qt::endl;
            foreach (const QString& s,fileInfo->directIncludes()) {
                stream<<"\t--"+s<<Qt::endl;
            }
            stream<<"\t**includes:**"<<Qt::endl;
            foreach (const QString& s,fileInfo->includes()) {
                stream<<"\t--"+s<<Qt::endl;
            }
        }
    }
}

void CppPreprocessor::addIncludePath(const QString &fileName)
{
    if (!mIncludePaths.contains(fileName)) {
        mIncludePaths.insert(fileName);
        mIncludePathList.append(fileName);
    }
}

void CppPreprocessor::addProjectIncludePath(const QString &fileName)
{
    if (!mProjectIncludePaths.contains(fileName)) {
        mProjectIncludePaths.insert(fileName);
        mProjectIncludePathList.append(fileName);
    }
}

void CppPreprocessor::removeScannedFile(const QString &filename)
{
#ifdef REDPANDA_PARSER_TEST
    if (mRegistry.scannedFiles.contains(filename) || mRegistry.replayRecords.contains(filename))
        mInvalidateCounts[filename]++;
#endif
    invalidDefinesInFile(filename);
    mRegistry.scannedFiles.remove(filename);
    mRegistry.replayRecords.remove(filename);
    mRegistry.fileMetadata.remove(filename);
}

QString CppPreprocessor::expandMacros(QString text) const
{
    QStringList lines{text};
    PpTokenizer tokenizer(lines);
    PpExpander expander([this](const QString& name) {
        return getDefine(name);
    });
    QString out;
    expander.expandLine(tokenizer, out);
    return out;
}

QString CppPreprocessor::getNextPreprocessor()
{
    skipToPreprocessor(); // skip until # at start of line
    if (mSession.index >= mSession.buffer.count())
        return ""; // we've gone past the final #preprocessor line. Yay

    // Calculate index to insert defines in in result file
    mSession.preProcIndex = (mSession.result.count() - 1) + 1; // offset by one for #include rootfile

    // Assemble whole line, convert newlines to space
    QString result = mSession.buffer[mSession.index];
    mSession.result.append("");// defines resolve into empty files, except #define and #include
    return result;
}

void CppPreprocessor::handleDefine(const QString &tokens)
{
    PreprocessorProfileScope profileScope(preprocessorProfile.handleDefineNs,
                                          preprocessorProfile.handleDefineCalls);
    if (getCurrentBranch() == BranchResult::isTrue) {
        QString name,args,value;
        getDefineParts(tokens, name, args, value);
        if (name == mSession.fileIncludeOnceToken)
            mSession.fileIncludeOnceToken = "";
        // Add to the list
        addDefineByParts(name, args, value, false);
        mSession.result[mSession.preProcIndex] = "#define " + tokens; // add define to result file so the parser can handle it
    }
}

void CppPreprocessor::handleInclude(const QString &tokens, bool fromNext)
{
    PreprocessorProfileScope profileScope(preprocessorProfile.handleIncludeNs,
                                          preprocessorProfile.handleIncludeCalls);
    if (getCurrentBranch()!=BranchResult::isTrue) // we're skipping due to a branch failure
        return;

    PParsedFile file = mSession.includeStack.back();
    QString fileName;
    // Get full header file name
    QString currentDir = excludeTrailingPathDelimiter(extractFileDir(file->fileName));
    QStringList includes;
    QStringList projectIncludes;
    if (fromNext && mIncludePaths.contains(currentDir)) {
        int i = mIncludePathList.indexOf(currentDir);
        includes = mIncludePathList.mid(i+1);
        // foreach(const QString& s, mIncludePathList) {
        //     if (found) {
        //         includes.append(s);
        //         continue;
        //     } else if (s == currentDir)
        //         found = true;
        // }
        projectIncludes = mProjectIncludePathList;
    } else if (fromNext && mProjectIncludePaths.contains(currentDir)) {
        includes = mIncludePathList;
        int i = mProjectIncludePathList.indexOf(currentDir);
        projectIncludes = mProjectIncludePathList.mid(i+1);
    } else {
        includes = mIncludePathList;
        projectIncludes = mProjectIncludePathList;
    }

    QString s=tokens;
    if (!s.startsWith('<') && !s.startsWith('\"'))
        s = expandMacros(s);

    fileName = getHeaderFilename(
                file->fileName,
                s,
                includes,
                projectIncludes);

    if (fileName.isEmpty())
        return;

    PParsedFileInfo oldCurrentIncludes = mSession.currentFileInfo;
    openInclude(fileName);
}

void CppPreprocessor::handlePreprocessor(const QString& command, const QString& tokens)
{
    if (!mSession.includeStack.isEmpty()
            && mSession.includeStack.back()->guardClosed)
        mSession.includeStack.back()->contentAfterGuard = true;
    std::function<void(const QString& tokens)> handler = mPreprocessorHandlers.value(command);
    if (handler) {
        handler(tokens);
        if (!command.startsWith("include") && !command.startsWith("pragma")
                && !command.startsWith("undef"))
            mSession.fileJustOpenned = false;
    }
}

void CppPreprocessor::handleUndefine(const QString& tokens)
{
    if (getCurrentBranch() != BranchResult::isTrue)
        return;
    // Remove undef
    QString name = tokens.trimmed();

//    //may be defined many times
//    while (true) {
    PDefine define = getDefine(name);
    if (define) {
        //remove the define from defines set
        mSession.defines.remove(name);
        PParsedFileInfo fileInfo = replayRecordForFile(mSession.fileName);
        if (define->filename == mSession.fileName) {
            if (fileInfo && fileInfo->defines())
                fileInfo->defines()->remove(name);
        } else if (fileInfo) {
            PDefineMap undefineMap = fileInfo->undefines();
            if (!undefineMap->contains(name))
                undefineMap->insert(name, define);
        }
    }
    mSession.result[mSession.preProcIndex] = "#undef " + name; // add define to result file so the parser can handle it

}

void CppPreprocessor::handleIf(const QString &tokens)
{
    if (getCurrentBranch()!=BranchResult::isTrue) {// we are already inside an if that is NOT being taken
        setCurrentBranch(BranchResult::parentIsFalse);// so don't take this one either
    } else {
        bool testResult = evaluateIf(tokens);
        setCurrentBranch(testResult?(BranchResult::isTrue):(BranchResult::isFalse));
    }
}

QString CppPreprocessor::signatureOfDefine(const PDefine &define)
{
    if (!define)
        return QStringLiteral("\x02");
    return define->args + QStringLiteral("\x01") + define->value;
}

void CppPreprocessor::handleIfdef(const QString &tokens)
{
        if (getCurrentBranch()!=BranchResult::isTrue) {
            setCurrentBranch(BranchResult::parentIsFalse);
        } else {
            QString name = tokens.trimmed();
            setCurrentBranch( getDefine(name)!=nullptr?(BranchResult::isTrue):(BranchResult::isFalse) );
        }
}

void CppPreprocessor::handleIfndef(const QString &tokens)
{
    if (getCurrentBranch()!=BranchResult::isTrue) {
        setCurrentBranch(BranchResult::parentIsFalse);
    } else {
        QString name = tokens.trimmed();

        if (mSession.fileJustOpenned) {
            mSession.fileIncludeOnceToken = name;
            mSession.includeStack.back()->guardCandidate = true;
        }
        setCurrentBranch( getDefine(name)==nullptr?(BranchResult::isTrue):(BranchResult::isFalse) );
    }
}

void CppPreprocessor::handleElif(const QString &tokens)
{
    BranchResult oldResult = getCurrentBranch(); // take either if or else
    removeCurrentBranch();
    BranchResult elseResult = calcElseBranchResult(oldResult);
    if (elseResult == BranchResult::isTrue) { // don't take this one, if  previous has been taken
        bool testResult = evaluateIf(tokens);
        setCurrentBranch(testResult?(BranchResult::isTrue):(BranchResult::isFalse));
    } else {
        setCurrentBranch(elseResult);
    }
}

void CppPreprocessor::handleElifdef(const QString &tokens)
{
    if (supportCPP23()) {
        BranchResult oldResult = getCurrentBranch(); // take either if or else
        removeCurrentBranch();
        BranchResult elseResult = calcElseBranchResult(oldResult);
        if (elseResult == BranchResult::isTrue) { // don't take this one, if  previous has been taken
            QString name = tokens.trimmed();

            setCurrentBranch( getDefine(name)!=nullptr?(BranchResult::isTrue):(BranchResult::isFalse) );
        } else {
            setCurrentBranch(elseResult);
        }
    }
}

void CppPreprocessor::handleElifndef(const QString &tokens)
{
    if (supportCPP23()) {
        BranchResult oldResult = getCurrentBranch(); // take either if or else
        removeCurrentBranch();
        BranchResult elseResult = calcElseBranchResult(oldResult);
        if (elseResult == BranchResult::isTrue) { // don't take this one, if  previous has been taken
            QString name = tokens.trimmed();

            setCurrentBranch( getDefine(name)==nullptr?(BranchResult::isTrue):(BranchResult::isFalse) );
        } else {
            setCurrentBranch(elseResult);
        }
    }
}

void CppPreprocessor::handleElse(const QString &tokens)
{
    BranchResult oldResult = getCurrentBranch(); // take either if or else
    removeCurrentBranch();
    setCurrentBranch(calcElseBranchResult(oldResult));
}

void CppPreprocessor::handleEndif(const QString &tokens)
{
    PParsedFile current = mSession.includeStack.back();
    removeCurrentBranch();
    if (current->guardCandidate && mSession.branchResults.isEmpty()
            && mSession.fileIncludeOnceToken.isEmpty())
        current->guardClosed = true;
}

void CppPreprocessor::handleInclude(const QString &tokens)
{
    handleInclude(tokens,false);
}

void CppPreprocessor::handleIncludeNext(const QString &tokens)
{
    handleInclude(tokens,true);
}

void CppPreprocessor::handlePragma(const QString &tokens)
{
    if (tokens.trimmed()=="once") {
        mRegistry.fileMetadata[mSession.fileName].includeProtected = true;
    }
}

QString CppPreprocessor::removeGCCAttributes(const QString &line)
{
    QString newLine = "";
    QString word = "";
    int lenLine = line.length();
    int i=0;
    while(i< lenLine) {
        if (isWordChar(line[i])) {
            word += line[i];
        } else {
            if (!word.isEmpty()) {
                removeGCCAttribute(line,newLine,i,word);
            }
            word = "";
            if (i<lenLine) {
                newLine = newLine+line[i];
            }
        }
        i++;
    }
    if (!word.isEmpty())
        removeGCCAttribute(line,newLine,i,word);
    return newLine;
}

void CppPreprocessor::removeGCCAttribute(const QString &line, QString &newLine, int &i, const QString &word)
{
    int lenLine = line.length();
    int level = 0;
    if (word=="__attribute__") {
        while ( (i<lenLine) && isSpaceChar(line[i]))
            i++;
        if ((i<lenLine) && (line[i]=='(')) {
            level=0;
            while (i<lenLine) {
                switch(line[i].unicode()) {
                case '(': level++;
                    break;
                case ')': level--;
                    break;
                }
                i++;
                if (level==0)
                    break;
            }
        }
    } else {
        newLine += word;
    }
}

void CppPreprocessor::openInclude(QString fileName)
{
    PreprocessorProfileScope profileScope(preprocessorProfile.openIncludeNs,
                                          preprocessorProfile.openIncludeCalls);
    if (mStopForParserReset)
        return;
    if (mSession.includeStack.count() >= mMaxIncludeNestingDepth) {
#ifdef REDPANDA_PARSER_TEST
        mSkipCounts[fileName]++;
#endif
        return;
    }

    PParsedFileInfo fileInfo = findFileInfo(fileName);
    if (fileInfo)
        fileName = fileInfo->fileName();
    else
        fileInfo = std::make_shared<ParsedFileInfo>(fileName);

    bool inStack = false;
    for (const PParsedFile &parsedFile : mSession.includeStack) {
        if (parsedFile->fileName == fileName) {
            inStack = true;
            break;
        }
    }

    bool boundaryEntry = false;
    QString boundaryAncestor;
    if (mSession.includeStack.isEmpty()) {
        boundaryEntry = isInsideBoundary(fileName);
        boundaryAncestor = boundaryEntry ? fileName : QString();
    } else {
        const PParsedFile &parent = mSession.includeStack.back();
        bool parentInside = isInsideBoundary(parent->fileName);
        bool childInside = isInsideBoundary(fileName);
        boundaryEntry = !parentInside && childInside;
        boundaryAncestor = boundaryEntry ? fileName : parent->boundaryAncestor;
        if (parentInside && !childInside)
            qWarning() << "Boundary-inside file includes a boundary-outside file:"
                       << parent->fileName << fileName;
    }

    if (!inStack && mRegistry.scannedFiles.contains(fileName)
            && !hasValidFileStamps(fileInfo)) {
        invalidateFileAndDependents(fileName);
        fileInfo = findFileInfo(fileName);
        if (!fileInfo)
            fileInfo = std::make_shared<ParsedFileInfo>(fileName);
    }

    bool includeProtected = isIncludeProtected(fileName);
    if (mSession.includeStack.count() > 0) {
        PreprocessorProfileScope relationsScope(preprocessorProfile.includeRelationsNs,
                                                preprocessorProfile.includeRelationsCalls);
        for (PParsedFile &parsedFile : mSession.includeStack) {
            parsedFile->fileInfo->addInclude(fileName);
            for (const QString &includedFileName : fileInfo->includes())
                parsedFile->fileInfo->addInclude(includedFileName);
        }
        PParsedFile innerMostFile = mSession.includeStack.back();
        innerMostFile->fileInfo->addDirectInclude(fileName);
        innerMostFile->index = mSession.index;
        innerMostFile->branches = mSession.branchResults.count();
    }

    if (includeProtected && mSession.processed.contains(fileName)) {
#ifdef REDPANDA_PARSER_TEST
        mSkipCounts[fileName]++;
#endif
        return;
    }

    bool repeatedUnprotected = mSession.processed.contains(fileName)
            && !includeProtected;
    bool canReplay = boundaryEntry && !inStack && !repeatedUnprotected
            && mRegistry.scannedFiles.contains(fileName);
    if (canReplay) {
        bool valid = mSession.allBoundaryEntriesReplayed
                && fileInfo->hasReplayCondition()
                && fileInfo->userMacros() == userMacroSnapshot()
                && fileInfo->boundaryPrefix() == mSession.boundaryPrefix;
        if (!valid) {
            invalidateFileAndDependents(fileName);
            fileInfo = std::make_shared<ParsedFileInfo>(fileName);
        } else {
#ifdef REDPANDA_PARSER_TEST
            mReplayCounts[fileName]++;
#endif
            addDefinesInFile(fileName);
            mSession.boundaryPrefix.append(fileName);
            return;
        }
    }

    PParsedFile parsedFile = std::make_shared<ParsedFile>();
    parsedFile->index = 0;
    parsedFile->fileName = fileName;
    parsedFile->branches = 0;
    parsedFile->fileInfo = fileInfo;
    parsedFile->boundaryEntry = boundaryEntry;
    parsedFile->boundaryAncestor = boundaryAncestor;
    if (boundaryEntry)
        parsedFile->userMacros = userMacroSnapshot();
    parsedFile->boundaryPrefix = mSession.boundaryPrefix;

    mSession.currentFileInfo = fileInfo;
    bool registryRecord = boundaryEntry || boundaryAncestor.isEmpty();
    if (registryRecord) {
        mRegistry.replayRecords.insert(fileName, mSession.currentFileInfo);
        mRegistry.scannedFiles.insert(fileName);
    }
    bool isSystemFile = isSystemHeaderFile(fileName, mIncludePaths)
            || isSystemHeaderFile(fileName, mProjectIncludePaths);
    bool scanContent = (mParseSystem && isSystemFile) || (mParseLocal && !isSystemFile);
#ifdef REDPANDA_PARSER_TEST
    if (scanContent)
        mScanCounts[fileName]++;
#endif
    mSession.processed.insert(fileName);
    mSession.boundaryAncestors[fileName].append(boundaryAncestor);

    if (mSession.fileCache.contains(fileName)) {
        parsedFile->buffer = mSession.fileCache.value(fileName);
    } else if (scanContent) {
        QStringList bufferedText;
        if (mOnGetFileStream && mOnGetFileStream(fileName, bufferedText))
            parsedFile->buffer = bufferedText;
        else {
            PreprocessorProfileScope readScope(preprocessorProfile.readFileNs,
                                               preprocessorProfile.readFileCalls);
            parsedFile->buffer = readFileToLines(fileName);
            QFileInfo diskInfo(fileName);
            mRegistry.fileMetadata.insert(fileName,
                    Registry::FileMetadata{diskInfo.lastModified(), diskInfo.size()});
        }
        combineLinesEndingWithBackslash(parsedFile->buffer);
        replaceCommentsBySpaceChar(parsedFile->buffer);
    }
    mSession.includeStack.append(parsedFile);

    mSession.index = parsedFile->index;
    mSession.fileName = parsedFile->fileName;
    mSession.buffer = parsedFile->buffer;

    QString includeLine = "#include " + fileName + ":-1";
    if (mSession.includeStack.count() > 1) {
        mSession.result[mSession.preProcIndex] = includeLine;
        mSession.index = -1;
    } else {
        mSession.result.append(includeLine);
    }
    if (boundaryEntry) {
        mSession.allBoundaryEntriesReplayed = false;
        mSession.boundaryPrefix.append(fileName);
    }
    mSession.fileJustOpenned = true;
    mSession.fileIncludeOnceToken = "";
}


void CppPreprocessor::closeInclude()
{
    PreprocessorProfileScope profileScope(preprocessorProfile.closeIncludeNs,
                                          preprocessorProfile.closeIncludeCalls);
    if (mSession.includeStack.isEmpty())
        return;
    PParsedFile lastFile = mSession.includeStack.back();
    if (lastFile->boundaryEntry)
        lastFile->fileInfo->setReplayCondition(
                lastFile->userMacros, lastFile->boundaryPrefix);
    if (lastFile->guardCandidate) {
        bool fullGuard = lastFile->guardClosed && !lastFile->contentAfterGuard;
        mRegistry.fileMetadata[lastFile->fileName].includeProtected =
                isIncludeProtected(lastFile->fileName) || fullGuard;
    }
    if (!isIncludeProtected(lastFile->fileName))
        mSession.fileCache.insert(lastFile->fileName, lastFile->buffer);
    mSession.includeStack.pop_back();

    if (mSession.includeStack.isEmpty())
        return;
    PParsedFile parsedFile = mSession.includeStack.back();

    // Continue where we left off
    mSession.index = parsedFile->index+1;
    mSession.fileName = parsedFile->fileName;
    // Point to previous buffer and start past the include we walked into
    mSession.buffer = parsedFile->buffer;
    while (mSession.branchResults.count()>parsedFile->branches) {
        mSession.branchResults.pop_back();
    }


    // Start augmenting previous include list again
    mSession.currentFileInfo = parsedFile->fileInfo;

    // Update result file (we've left the previous file)
    mSession.result.append(
                QString("#include %1:%2").arg(parsedFile->fileName)
                .arg(parsedFile->index));
}

CppPreprocessor::BranchResult CppPreprocessor::calcElseBranchResult(BranchResult oldResult)
{
    switch(oldResult) {
    case BranchResult::isTrue: return BranchResult::isFalse_but_trued;
    case BranchResult::isFalse: return BranchResult::isTrue;
    case BranchResult::isFalse_but_trued: return BranchResult::isFalse_but_trued;
    case BranchResult::parentIsFalse: return BranchResult::parentIsFalse;
    }
    Q_ASSERT( false ); //We should fail here.
    return BranchResult::isFalse;
}

void CppPreprocessor::addDefinesInFile(const QString &fileName)
{
    if (mSession.processed.contains(fileName))
        return;
    mSession.processed.insert(fileName);

    PParsedFileInfo fileInfo = findFileInfo(fileName);
    PDefineMap defineList = fileInfo ? fileInfo->defines() : PDefineMap();
    if (defineList) {
        foreach (const PDefine& define, defineList->values()) {
            mSession.defines.insert(define->name,define);
        }
    }

    if (fileInfo) {
        foreach (const QString& file, fileInfo->includes()) {
            addDefinesInFile(file);
        }
    }
}

void CppPreprocessor::combineLinesEndingWithBackslash(QStringList &text)
{
    if (text.isEmpty())
        return;
    int i = text.length()-1;
    while (i>0) {
        const QString& prevLineText = text[i-1];
        int lastI=prevLineText.length()-1;
        while (lastI>=0 && isSpaceChar(prevLineText[lastI]))
            lastI--;
        if (lastI>=0 && prevLineText[lastI]=='\\') {
            text[i-1] = prevLineText.left(lastI)+text[i];
            text[i]="";
        }
        i--;
    }
}

QList<PDefineArgToken> CppPreprocessor::tokenizeValue(const QString &value)
{
    int i=0;
    PDefineArgToken  token = std::make_shared<DefineArgToken>();
    token->type = DefineArgTokenType::Other;
    QList<PDefineArgToken> tokens;
    bool skipSpaces=false;
    while (i<value.length()) {
        QChar ch = value[i];
        if (isSpaceChar(ch)) {
            if (token->type==DefineArgTokenType::Other) {
                token->value = " ";
                token->type = DefineArgTokenType::Space;
            } else if (token->type!=DefineArgTokenType::Space) {
                tokens.append(token);
                token = std::make_shared<DefineArgToken>();
                token->value = " ";
                token->type = DefineArgTokenType::Space;
            }
            i++;
        } else if (ch=='#') {
            bool isDSharp = (i+1<value.length()) && (value[i+1]=='#');
            if (token->type==DefineArgTokenType::Space) {
                if (!isDSharp) {
                    // keep whitespace before '#' (stringize)
                    tokens.append(token);
                }
                // whitespace before '##' is dropped (pasting ignores it)
                token = std::make_shared<DefineArgToken>();
            } else if (token->type!=DefineArgTokenType::Other) {
                tokens.append(token);
                token = std::make_shared<DefineArgToken>();
            }
            if (isDSharp) {
                i+=2;
                token->value = "##";
                token->type = DefineArgTokenType::DSharp;
            } else {
                i++;
                token->value = "#";
                token->type = DefineArgTokenType::Sharp;
            }
            skipSpaces=true;
            tokens.append(token);
            token = std::make_shared<DefineArgToken>();
            token->value = "";
            token->type = DefineArgTokenType::Other;
        } else if (isWordChar(ch)) {
            if (token->type==DefineArgTokenType::Other) {
                token->value = ch ;
                token->type = DefineArgTokenType::Identifier;
            } else if (token->type==DefineArgTokenType::Identifier) {
                token->value+=ch;
            } else if (skipSpaces && token->type==DefineArgTokenType::Space) {
                //dont use space;
                token->value = ch ;
                token->type = DefineArgTokenType::Identifier;
            } else {
                tokens.append(token);
                token = std::make_shared<DefineArgToken>();
                token->value = ch ;
                token->type = DefineArgTokenType::Identifier;
            }
            skipSpaces=false;
            i++;
        } else {
            if (skipSpaces && token->type==DefineArgTokenType::Space) {
                //dont use space;
            } else if (token->type!=DefineArgTokenType::Other) {
                tokens.append(token);
                token = std::make_shared<DefineArgToken>();
            }
            skipSpaces=false;
            token->value = ch ;
            token->type = DefineArgTokenType::Symbol;
            i++;
        }
    }
    if(token->type!=DefineArgTokenType::Other)
        tokens.append(token);
    return tokens;
}

void CppPreprocessor::replaceCommentsBySpaceChar(QStringList &text)
{
    PreprocessorProfileScope profileScope(preprocessorProfile.replaceCommentsNs,
                                          preprocessorProfile.replaceCommentsCalls);
    ContentType currentType = ContentType::Other;
    QString delimiter;
    for (int lineIdx = 0; lineIdx < text.length(); lineIdx++) {
        const QString& line = text[lineIdx];
        int pos = 0;
        int lineLen=line.length();
        int currentLineIdx = lineIdx;
        QString s;
        s.reserve(line.length());
        // String & Char Literal can't to next line
        if (currentType == ContentType::Character
                || currentType ==  ContentType::String
                || currentType ==  ContentType::EscapeSequence)
            currentType = ContentType::Other;
        // Really treat Ansi C Style Comment as a space (and merge lines) only when it's used to define macros.
        while (pos<lineLen) {
            QChar ch =line[pos];
            if (currentType == ContentType::AnsiCComment) {
                if (ch=='*' && (pos+1<lineLen) && line[pos+1]=='/') {
                    pos+=2;
                    currentType = ContentType::Other;
                } else {
                    pos+=1;
                }
                continue;
            }
            switch (ch.unicode()) {
            case '"':
                switch (currentType) {
                case ContentType::String:
                    currentType=ContentType::Other;
                    break;
                case ContentType::RawString:
                    if (QStringView(line.constData(), pos).endsWith(')'+delimiter))
                        currentType = ContentType::Other;
                    break;
                case ContentType::Other:
                    currentType=ContentType::String;
                    break;
                case ContentType::RawStringPrefix:
                    delimiter+=ch;
                    break;
                default:
                    break;
                }
                s+=ch;
                break;
            case '\'':
                switch (currentType) {
                case ContentType::Character:
                    currentType=ContentType::Other;
                    break;
                case ContentType::Other:
                    currentType=ContentType::Character;
                    break;
                case ContentType::RawStringPrefix:
                    delimiter+=ch;
                    break;
                default:
                    break;
                }
                s+=ch;
                break;
            case 'R':
                if (currentType == ContentType::Other && pos+1<lineLen && line[pos+1]=='"') {
                    s+=ch;
                    pos++;
                    ch = line[pos];
                    currentType=ContentType::RawStringPrefix;
                    delimiter = "";
                }
                s+=ch;
                break;
            case '(':
                switch(currentType) {
                case ContentType::RawStringPrefix:
                    currentType = ContentType::RawString;
                    break;
                default:
                    break;
                }
                s+=ch;
                break;
            case '/':
                if (currentType == ContentType::Other) {
                    if (pos+1<lineLen && line[pos+1]=='/') {
                        // line comment
                        pos = lineLen+1; // skip chars left in the current line
                        break;
                    } else if (pos+1<lineLen && line[pos+1]=='*') {
                        /* ansi c comment */
                        s+=' '; // replace comments with a space
                        pos++;
                        currentType = ContentType::AnsiCComment;
                        break;
                    }
                }
                s+=ch;
                break;
            case '\\':
                switch (currentType) {
                case ContentType::String:
                case ContentType::Character:
                    pos++;
                    s+=ch;
                    if (pos<lineLen) {
                        ch = line[pos];
                        s+=ch;
                    }
                    break;
                default:
                    s+=ch;
                }
                break;
            default:
                if (currentType == ContentType::RawStringPrefix)
                    delimiter += ch;
                s+=ch;
            }
            pos++;
        }
        text[currentLineIdx] = s;
        if (currentLineIdx!=lineIdx)
            text[lineIdx].clear();
    }
}

bool CppPreprocessor::supportCPP23() const
{
    return mCppVersion >= CppVersion::Cpp23;
}

// Whitespace before '#' is allowed: a line whose first non-blank
// character is '#' is a preprocessor directive line.
static bool isDirectiveLine(const QString& line)
{
    for (const QChar& ch : line) {
        if (ch == ' ' || ch == '\t')
            continue;
        return ch == '#';
    }
    return false;
}

void CppPreprocessor::preprocessBuffer()
{
    while (mSession.includeStack.count() > 0) {
        QString s;
        do {
            if (mStopForParserReset) {
                mSession.buffer.clear();
                return;
            }
            s = getNextPreprocessor();
            if (isDirectiveLine(s)) {
                int pos = 0;
                while (pos < s.length() && (s[pos] == ' ' || s[pos] == '\t'))
                    ++pos;
                ++pos;
                while (pos < s.length() && (s[pos] == ' ' || s[pos] == '\t'))
                    ++pos;
                int commandStart = pos;
                while (pos < s.length()
                       && ((s[pos] >= 'a' && s[pos] <= 'z') || s[pos] == '_'))
                    ++pos;
                QString command = s.mid(commandStart, pos - commandStart);
                while (pos < s.length() && (s[pos] == ' ' || s[pos] == '\t'))
                    ++pos;
                QString tokens = s.mid(pos);
                if (!command.isEmpty()) {
                    handlePreprocessor(command, tokens);
                }
            }
            // Step over
            mSession.index++;
        } while (!s.isEmpty());
        closeInclude();
    }
}

void CppPreprocessor::skipToPreprocessor()
{
    int bufferCount = mSession.buffer.count();
// Increment until a line begins with a #
    while ((mSession.index < bufferCount) && !isDirectiveLine(mSession.buffer[mSession.index])) {
        if (mSession.includeStack.back()->guardClosed
                && !mSession.buffer[mSession.index].trimmed().isEmpty())
            mSession.includeStack.back()->contentAfterGuard = true;
        if (getCurrentBranch()==BranchResult::isTrue) { // if not skipping, expand current macros
            QString expanded;
            int consumed = expandLineWithEngine(mSession.buffer, mSession.index, expanded);
            mSession.result.append(expanded);
            // lines consumed by multi-line macro invocations become
            // empty lines to keep the result aligned with the source
            for (int i = 1; i < consumed; i++) {
                mSession.result.append("");
            }
            mSession.index += consumed;
        } else { // If skipping due to a failed branch, clear line
            mSession.result.append("");
            mSession.index++;
        }
    }
}

int CppPreprocessor::expandLineWithEngine(const QStringList &buffer, int startLine, QString &out)
{
    PreprocessorProfileScope profileScope(preprocessorProfile.expandLineNs,
                                          preprocessorProfile.expandLineCalls);
    PpTokenizer tokenizer(buffer, startLine);
    PpExpander expander([this](const QString& name) {
        return getDefine(name);
    });
    return expander.expandLine(tokenizer, out);
}

bool CppPreprocessor::isNumberChar(const QChar &ch)
{
    if (ch>='0' && ch<='9')
        return true;
    if (ch>='a' && ch<='f')
        return true;
    if (ch>='A' && ch<='F')
        return true;
    switch(ch.unicode()) {
    case 'x':
    case 'X':
    case 'u':
    case 'U':
    case 'l':
    case 'L':
        return true;
    default:
        return false;
    }
}

bool CppPreprocessor::evaluateIf(const QString &line) const
{
    PreprocessorProfileScope profileScope(preprocessorProfile.evaluateIfNs,
                                          preprocessorProfile.evaluateIfCalls);
    QString newLine = expandMacrosInConditioningExpression(line); // replace FOO by numerical value of FOO
    bool result = evaluateExpression(newLine);
    //qDebug()<<newLine<<line<<result;
    return  result;
}

QString CppPreprocessor::expandMacrosInConditioningExpression(QString line) const
{
    // #if/#elif expressions are expanded by the same pp-token engine in
    // condition mode (defined/__has_include as operators, undefined
    // identifiers evaluate to 0, references are recorded for the
    // environment-dependency check).
    QStringList lines{line};
    PpTokenizer tokenizer(lines);
    PpExpander expander([this](const QString& name) {
        return getDefine(name);
    });
    expander.setConditionMode(true,
        [this](const QString& args) {
            QString header = args;
            if (!header.startsWith('<') && !header.startsWith('\"')) {
                QStringList argLines{header};
                PpTokenizer argTokenizer(argLines);
                PpExpander argExpander([this](const QString& name) {
                    return getDefine(name);
                });
                argExpander.expandLine(argTokenizer, header);
            }
            if (!mSession.currentFileInfo)
                return false;
            QString fileName = getHeaderFilename(
                    mSession.currentFileInfo->fileName(),
                    header,
                    mIncludePathList,
                    mProjectIncludePathList);
            return !fileName.isEmpty();
        },
        [this](const QString& name) {

        });
    QString out;
    expander.expandLine(tokenizer, out);
    return out;
}

bool CppPreprocessor::skipSpaces(const QString &expr, int &pos) const
{
    while (pos<expr.length() && isSpaceChar(expr[pos]))
        pos++;
    return pos<expr.length();
}

bool CppPreprocessor::evalNumber(const QString &expr, NumberType &result, int &pos) const
{
    if (!skipSpaces(expr,pos))
        return false;
    if (!isDigit(expr[pos]))
        return false;
    QString s;
    while (pos<expr.length() && isNumberChar(expr[pos])) {
        s+=expr[pos];
        pos++;
    }
    bool ok;

    if (s.endsWith("ULL",Qt::CaseInsensitive)) {
        s.resize(s.length()-3);
        result = s.toULongLong(&ok, 0);
    } else if (s.endsWith("UL",Qt::CaseInsensitive)) {
        s.resize(s.length()-2);
        result = s.toULong(&ok, 0);
    } else if (s.endsWith("LL",Qt::CaseInsensitive)) {
        s.resize(s.length()-2);
        result = s.toLongLong(&ok, 0);
    } else if (s.endsWith("L",Qt::CaseInsensitive)) {
        s.resize(s.length()-1);
        result = s.toLong(&ok, 0);
    } else if (s.endsWith("U",Qt::CaseInsensitive)) {
        s.resize(s.length()-1);
        result = s.toUInt(&ok, 0);
    } else {
        result = s.toInt(&ok, 0);
    }
    return ok;
}

bool CppPreprocessor::evalTerm(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!skipSpaces(expr,pos))
        return false;
    if (expr[pos]=='\'') {
        // character constant, e.g. 'a', '\n', or a multi-char constant
        pos++;
        if (pos>=expr.length())
            return false;
        if (expr[pos]=='\\') {
            pos++;
            if (pos>=expr.length())
                return false;
            switch (expr[pos].unicode()) {
            case 'n': result = '\n'; break;
            case 't': result = '\t'; break;
            case 'r': result = '\r'; break;
            case '0': result = 0; break;
            case 'a': result = '\a'; break;
            case 'b': result = '\b'; break;
            case 'f': result = '\f'; break;
            case 'v': result = '\v'; break;
            default: result = expr[pos].unicode(); break; // '\\', '\'', '"', ...
            }
            pos++;
        } else {
            result = expr[pos].unicode();
            pos++;
        }
        // fold any remaining characters (multi-char constants, GCC-style)
        while (pos<expr.length() && expr[pos]!='\'') {
            if (evaluate)
                result = (result << 8) | expr[pos].unicode();
            pos++;
        }
        if (pos>=expr.length())
            return false; // unterminated
        pos++; // skip closing '
        return true;
    }
    if (expr[pos]=='(') {
        pos++;
        if (!evalExpr(expr,result,pos,evaluate))
            return false;
        if (!skipSpaces(expr,pos))
            return false;
        if (expr[pos]!=')')
            return false;
        pos++;
        return true;
    } else {
        return evalNumber(expr,result,pos);
    }
}

/*
 * unary_expr = term
     | '+' term
     | '-' term
     | '!' term
     | '~' term
 */
bool CppPreprocessor::evalUnaryExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!skipSpaces(expr,pos))
        return false;
    if (expr[pos]=='+') {
        pos++;
        if (!evalTerm(expr,result,pos,evaluate))
            return false;
    } else if (expr[pos]=='-') {
        pos++;
        if (!evalTerm(expr,result,pos,evaluate))
            return false;
        if (evaluate)
            result = -result;
    } else if (expr[pos]=='~') {
        pos++;
        if (!evalTerm(expr,result,pos,evaluate))
            return false;
        if (evaluate)
            result = ~result;
    } else if (expr[pos]=='!') {
        pos++;
        if (!evalTerm(expr,result,pos,evaluate))
            return false;
        if (evaluate)
            result = !result;
    } else {
        return evalTerm(expr,result,pos,evaluate);
    }
    return true;
}

/*
 * mul_expr = unary_expr
     | mul_expr '*' unary_expr
     | mul_expr '/' unary_expr
     | mul_expr '%' unary_expr
 */
bool CppPreprocessor::evalMulExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!evalUnaryExpr(expr,result,pos,evaluate))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        NumberType rightResult;
        if (expr[pos]=='*') {
            pos++;
            if (!evalUnaryExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate)
                result *= rightResult;
        } else if (expr[pos]=='/') {
            pos++;
            if (!evalUnaryExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate) {
                if (rightResult != 0)
                    result /= rightResult;
                else
                    result = 0;
            }
        } else if (expr[pos]=='%') {
            pos++;
            if (!evalUnaryExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate) {
                if (rightResult != 0)
                    result %= rightResult;
                else
                    result = 0;
            }
        } else {
            break;
        }
    }
    return true;
}

/*
 * add_expr = mul_expr
     | add_expr '+' mul_expr
     | add_expr '-' mul_expr
 */
bool CppPreprocessor::evalAddExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!evalMulExpr(expr,result,pos,evaluate))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        NumberType rightResult;
        if (expr[pos]=='+') {
            pos++;
            if (!evalMulExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate)
                result += rightResult;
        } else if (expr[pos]=='-') {
            pos++;
            if (!evalMulExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate)
                result -= rightResult;
        } else {
            break;
        }
    }
    return true;
}

/*
 * shift_expr = add_expr
     | shift_expr "<<" add_expr
     | shift_expr ">>" add_expr
 */
bool CppPreprocessor::evalShiftExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!evalAddExpr(expr,result,pos,evaluate))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        NumberType rightResult;
        if (pos+1<expr.length() && expr[pos] == '<' && expr[pos+1]=='<') {
            pos += 2;
            if (!evalAddExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate)
                result = (result << rightResult);
        } else if (pos+1<expr.length() && expr[pos] == '>' && expr[pos+1]=='>') {
            pos += 2;
            if (!evalAddExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate)
                result = (result >> rightResult);
        } else {
            break;
        }
    }
    return true;
}

/*
 * relation_expr = shift_expr
     | relation_expr ">=" shift_expr
     | relation_expr ">" shift_expr
     | relation_expr "<=" shift_expr
     | relation_expr "<" shift_expr
 */
bool CppPreprocessor::evalRelationExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!evalShiftExpr(expr,result,pos,evaluate))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        NumberType rightResult;
        if (expr[pos]=='<') {
            if (pos+1<expr.length() && expr[pos+1]=='=') {
                pos+=2;
                if (!evalShiftExpr(expr,rightResult,pos,evaluate))
                    return false;
                if (evaluate)
                    result = (result <= rightResult);
            } else {
                pos++;
                if (!evalShiftExpr(expr,rightResult,pos,evaluate))
                    return false;
                if (evaluate)
                    result = (result < rightResult);
            }
        } else if (expr[pos]=='>') {
            if (pos+1<expr.length() && expr[pos+1]=='=') {
                pos+=2;
                if (!evalShiftExpr(expr,rightResult,pos,evaluate))
                    return false;
                if (evaluate)
                    result = (result >= rightResult);
            } else {
                pos++;
                if (!evalShiftExpr(expr,rightResult,pos,evaluate))
                    return false;
                if (evaluate)
                    result = (result > rightResult);
            }
        } else {
            break;
        }
    }
    return true;
}

/*
 * equal_expr = relation_expr
     | equal_expr "==" relation_expr
     | equal_expr "!=" relation_expr
 */
bool CppPreprocessor::evalEqualExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!evalRelationExpr(expr,result,pos,evaluate))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (pos+1<expr.length() && expr[pos]=='!' && expr[pos+1]=='=') {
            pos+=2;
            NumberType rightResult;
            if (!evalRelationExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate)
                result = (result != rightResult);
        } else if (pos+1<expr.length() && expr[pos]=='=' && expr[pos+1]=='=') {
            pos+=2;
            NumberType rightResult;
            if (!evalRelationExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate)
                result = (result == rightResult);
        } else {
            break;
        }
    }
    return true;
}

/*
 * bit_and_expr = equal_expr
     | bit_and_expr "&" equal_expr
 */
bool CppPreprocessor::evalBitAndExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!evalEqualExpr(expr,result,pos,evaluate))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (expr[pos]=='&'
                && (pos+1 == expr.length()
                || expr[pos+1]!='&')) {
            pos++;
            NumberType rightResult;
            if (!evalEqualExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate)
                result = result & rightResult;
        } else {
            break;
        }
    }
    return true;
}

/*
 * bit_xor_expr = bit_and_expr
     | bit_xor_expr "^" bit_and_expr
 */
bool CppPreprocessor::evalBitXorExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!evalBitAndExpr(expr,result,pos,evaluate))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (expr[pos]=='^') {
            pos++;
            NumberType rightResult;
            if (!evalBitAndExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate)
                result = result ^ rightResult;
        } else {
            break;
        }
    }
    return true;
}

/*
 * bit_or_expr = bit_xor_expr
     | bit_or_expr "|" bit_xor_expr
 */
bool CppPreprocessor::evalBitOrExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!evalBitXorExpr(expr,result,pos,evaluate))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (expr[pos] == '|'
                && (pos+1 == expr.length()
                || expr[pos+1]!='|')) {
            pos++;
            NumberType rightResult;
            if (!evalBitXorExpr(expr,rightResult,pos,evaluate))
                return false;
            if (evaluate)
                result = result | rightResult;
        } else {
            break;
        }
    }
    return true;
}

/*
 * logic_and_expr = bit_or_expr
    | logic_and_expr "&&" bit_or_expr
 */
bool CppPreprocessor::evalLogicAndExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!evalBitOrExpr(expr,result,pos,evaluate))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (pos+1<expr.length() && expr[pos]=='&' && expr[pos+1] =='&') {
            pos+=2;
            NumberType rightResult;
            if (!evalBitOrExpr(expr,rightResult,pos,evaluate && result))
                return false;
            if (evaluate)
                result = result && rightResult;
        } else {
            break;
        }
    }
    return true;
}

/*
 * logic_or_expr = logic_and_expr
    | logic_or_expr "||" logic_and_expr
 */
bool CppPreprocessor::evalLogicOrExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!evalLogicAndExpr(expr,result,pos,evaluate))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (pos+1<expr.length() && expr[pos]=='|' && expr[pos+1] =='|') {
            pos+=2;
            NumberType rightResult;
            if (!evalLogicAndExpr(expr,rightResult,pos,evaluate && !result))
                return false;
            if (evaluate)
                result = result || rightResult;
        } else {
            break;
        }
    }
    return true;
}

bool CppPreprocessor::evalConnditionalExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    if (!evalLogicOrExpr(expr,result,pos,evaluate))
        return false;
    if (!skipSpaces(expr,pos))
        return true;
    if (expr[pos] == '?') {
        pos++; // skip '?'
        int condition = result;
        NumberType result1,result2;
        if (!evalExpr(expr,result1,pos,evaluate && condition))
            return false;
        if (!skipSpaces(expr,pos))
            return false;
        if (expr[pos] != ':')
            return false;
        pos++; // skip ':'
        if (!evalExpr(expr,result2,pos,evaluate && !condition))
            return false;
        if (evaluate)
            result = condition ? result1 : result2;
    }
    return true;
}

bool CppPreprocessor::evalExpr(const QString &expr, NumberType &result, int &pos, bool evaluate) const
{
    return evalConnditionalExpr(expr,result,pos,evaluate);
}

/* BNF for C constant expression evaluation
 * term = number
     | '(' expression ')'
unary_expr = term
     | '+' term
     | '-' term
     | '!' term
     | '~' term
mul_expr = term
     | mul_expr '*' term
     | mul_expr '/' term
     | mul_expr '%' term
add_expr = mul_expr
     | add_expr '+' mul_expr
     | add_expr '-' mul_expr
shift_expr = add_expr
     | shift_expr "<<" add_expr
     | shift_expr ">>" add_expr
relation_expr = shift_expr
     | relation_expr ">=" shift_expr
     | relation_expr ">" shift_expr
     | relation_expr "<=" shift_expr
     | relation_expr "<" shift_expr
equal_expr = relation_expr
     | equal_expr "==" relation_expr
     | equal_expr "!=" relation_expr
bit_and_expr = equal_expr
     | bit_and_expr "&" equal_expr
bit_xor_expr = bit_and_expr
     | bit_xor_expr "^" bit_and_expr
bit_or_expr = bit_xor_expr
     | bit_or_expr "|" bit_xor_expr
logic_and_expr = bit_or_expr
    | logic_and_expr "&&" bit_or_expr
logic_or_expr = logic_and_expr
    | logic_or_expr "||" logic_and_expr
conditional_expr= logic_or_expr
    | logic_or_expr ? <expression> : conditional_expr

    */

bool CppPreprocessor::evaluateExpression(QString line) const
{
    int pos = 0;
    NumberType result;
    bool ok = evalExpr(line,result,pos,true);
    if (!ok)
        return false;
    //expr not finished
    if (skipSpaces(line,pos))
        return false;
    return result;
}


