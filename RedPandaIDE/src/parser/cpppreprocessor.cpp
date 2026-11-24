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
    qint64 fileStampsNs = 0;
    qint64 includeRelationsNs = 0;
    qint64 readFileNs = 0;
    qint64 handleDefineNs = 0;
    qint64 handleIncludeNs = 0;
    qint64 replaceCommentsNs = 0;
    qint64 expandLineNs = 0;
    qint64 evaluateIfNs = 0;
    qint64 expandConditionNs = 0;
    qint64 evaluateConditionNs = 0;
    qint64 openIncludeCalls = 0;
    qint64 closeIncludeCalls = 0;
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
                   << "fileStamps" << fileStampsNs / 1000000.0
                   << "includeRelations" << includeRelationsNs / 1000000.0
                   << "readFile" << readFileNs / 1000000.0
                   << "handleDefine" << handleDefineNs / 1000000.0
                   << "handleInclude" << handleIncludeNs / 1000000.0
                   << "replaceComments" << replaceCommentsNs / 1000000.0
                   << "expandLine" << expandLineNs / 1000000.0
                   << "evaluateIf" << evaluateIfNs / 1000000.0
                   << "expandCondition" << expandConditionNs / 1000000.0
                   << "evaluateCondition" << evaluateConditionNs / 1000000.0;
        qWarning() << "PREPROCESSOR_PROFILE_CALLS"
                   << "openInclude" << openIncludeCalls
                   << "closeInclude" << closeIncludeCalls
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

class PpConditionEvaluator {
public:
    explicit PpConditionEvaluator(const QVector<PpToken> &tokens) : mTokens(tokens) {}

    bool evaluate(qlonglong &result)
    {
        return parseExpression(result, true) && mPos == mTokens.count();
    }

private:
    bool parseNumber(const QString &text, qlonglong &result)
    {
        QString value = text;
        bool ok = false;
        if (value.endsWith(QLatin1String("ULL"), Qt::CaseInsensitive)) {
            value.chop(3);
            result = value.toULongLong(&ok, 0);
        } else if (value.endsWith(QLatin1String("UL"), Qt::CaseInsensitive)) {
            value.chop(2);
            result = value.toULong(&ok, 0);
        } else if (value.endsWith(QLatin1String("LL"), Qt::CaseInsensitive)) {
            value.chop(2);
            result = value.toLongLong(&ok, 0);
        } else if (value.endsWith(QLatin1String("L"), Qt::CaseInsensitive)) {
            value.chop(1);
            result = value.toLong(&ok, 0);
        } else if (value.endsWith(QLatin1String("U"), Qt::CaseInsensitive)) {
            value.chop(1);
            result = value.toUInt(&ok, 0);
        } else {
            result = value.toInt(&ok, 0);
        }
        return ok;
    }

    bool parseCharacter(const QString &text, qlonglong &result)
    {
        int pos = 1;
        if (pos >= text.length())
            return false;
        if (text[pos] == '\\') {
            ++pos;
            if (pos >= text.length())
                return false;
            switch (text[pos].unicode()) {
            case 'n': result = '\n'; break;
            case 't': result = '\t'; break;
            case 'r': result = '\r'; break;
            case '0': result = 0; break;
            case 'a': result = '\a'; break;
            case 'b': result = '\b'; break;
            case 'f': result = '\f'; break;
            case 'v': result = '\v'; break;
            default: result = text[pos].unicode(); break;
            }
            ++pos;
        } else {
            result = text[pos].unicode();
            ++pos;
        }
        while (pos < text.length() && text[pos] != '\'') {
            result = (result << 8) | text[pos].unicode();
            ++pos;
        }
        return pos + 1 == text.length() && text[pos] == '\'';
    }

    bool parsePrimary(qlonglong &result, bool evaluate)
    {
        if (consume("(")) {
            return parseExpression(result, evaluate) && consume(")");
        }
        if (atEnd())
            return false;
        const PpToken &token = mTokens[mPos++];
        if (token.kind == PpTokenKind::Number)
            return parseNumber(token.text, result);
        if (token.kind == PpTokenKind::CharLiteral)
            return parseCharacter(token.text, result);
        return false;
    }

    bool parseUnary(qlonglong &result, bool evaluate)
    {
        if (consume("+"))
            return parsePrimary(result, evaluate);
        if (consume("-")) {
            if (!parsePrimary(result, evaluate))
                return false;
            if (evaluate)
                result = -result;
            return true;
        }
        if (consume("~")) {
            if (!parsePrimary(result, evaluate))
                return false;
            if (evaluate)
                result = ~result;
            return true;
        }
        if (consume("!")) {
            if (!parsePrimary(result, evaluate))
                return false;
            if (evaluate)
                result = !result;
            return true;
        }
        return parsePrimary(result, evaluate);
    }

    bool parseMul(qlonglong &result, bool evaluate)
    {
        if (!parseUnary(result, evaluate))
            return false;
        while (true) {
            enum class Operation { Multiply, Divide, Modulo };
            Operation operation;
            if (consume("*"))
                operation = Operation::Multiply;
            else if (consume("/"))
                operation = Operation::Divide;
            else if (consume("%"))
                operation = Operation::Modulo;
            else
                return true;
            qlonglong right = 0;
            if (!parseUnary(right, evaluate))
                return false;
            if (evaluate) {
                switch (operation) {
                case Operation::Multiply:
                    result *= right;
                    break;
                case Operation::Divide:
                    result = right != 0 ? result / right : 0;
                    break;
                case Operation::Modulo:
                    result = right != 0 ? result % right : 0;
                    break;
                }
            }
        }
    }

    bool parseAdd(qlonglong &result, bool evaluate)
    {
        if (!parseMul(result, evaluate))
            return false;
        while (true) {
            bool subtract = false;
            if (!consume("+")) {
                if (!consume("-"))
                    return true;
                subtract = true;
            }
            qlonglong right = 0;
            if (!parseMul(right, evaluate))
                return false;
            if (evaluate)
                result = subtract ? result - right : result + right;
        }
    }

    bool parseShift(qlonglong &result, bool evaluate)
    {
        if (!parseAdd(result, evaluate))
            return false;
        while (true) {
            bool rightShift = false;
            if (!consume("<<")) {
                if (!consume(">>"))
                    return true;
                rightShift = true;
            }
            qlonglong right = 0;
            if (!parseAdd(right, evaluate))
                return false;
            if (evaluate)
                result = rightShift ? result >> right : result << right;
        }
    }

    bool parseRelation(qlonglong &result, bool evaluate)
    {
        if (!parseShift(result, evaluate))
            return false;
        while (true) {
            enum class Operation { LessEqual, GreaterEqual, Less, Greater };
            Operation operation;
            if (consume("<="))
                operation = Operation::LessEqual;
            else if (consume(">="))
                operation = Operation::GreaterEqual;
            else if (consume("<"))
                operation = Operation::Less;
            else if (consume(">"))
                operation = Operation::Greater;
            else
                return true;
            qlonglong right = 0;
            if (!parseShift(right, evaluate))
                return false;
            if (evaluate) {
                switch (operation) {
                case Operation::LessEqual:
                    result = result <= right;
                    break;
                case Operation::GreaterEqual:
                    result = result >= right;
                    break;
                case Operation::Less:
                    result = result < right;
                    break;
                case Operation::Greater:
                    result = result > right;
                    break;
                }
            }
        }
    }

    bool parseEquality(qlonglong &result, bool evaluate)
    {
        if (!parseRelation(result, evaluate))
            return false;
        while (true) {
            bool unequal = false;
            if (!consume("==")) {
                if (!consume("!="))
                    return true;
                unequal = true;
            }
            qlonglong right = 0;
            if (!parseRelation(right, evaluate))
                return false;
            if (evaluate)
                result = unequal ? result != right : result == right;
        }
    }

    bool parseBitAnd(qlonglong &result, bool evaluate)
    {
        if (!parseEquality(result, evaluate))
            return false;
        while (consume("&")) {
            qlonglong right = 0;
            if (!parseEquality(right, evaluate))
                return false;
            if (evaluate)
                result &= right;
        }
        return true;
    }

    bool parseBitXor(qlonglong &result, bool evaluate)
    {
        if (!parseBitAnd(result, evaluate))
            return false;
        while (consume("^")) {
            qlonglong right = 0;
            if (!parseBitAnd(right, evaluate))
                return false;
            if (evaluate)
                result ^= right;
        }
        return true;
    }

    bool parseBitOr(qlonglong &result, bool evaluate)
    {
        if (!parseBitXor(result, evaluate))
            return false;
        while (consume("|")) {
            qlonglong right = 0;
            if (!parseBitXor(right, evaluate))
                return false;
            if (evaluate)
                result |= right;
        }
        return true;
    }

    bool parseLogicAnd(qlonglong &result, bool evaluate)
    {
        if (!parseBitOr(result, evaluate))
            return false;
        while (consume("&&")) {
            qlonglong right = 0;
            if (!parseBitOr(right, evaluate && result))
                return false;
            if (evaluate)
                result = result && right;
        }
        return true;
    }

    bool parseLogicOr(qlonglong &result, bool evaluate)
    {
        if (!parseLogicAnd(result, evaluate))
            return false;
        while (consume("||")) {
            qlonglong right = 0;
            if (!parseLogicAnd(right, evaluate && !result))
                return false;
            if (evaluate)
                result = result || right;
        }
        return true;
    }

    bool parseExpression(qlonglong &result, bool evaluate)
    {
        if (!parseLogicOr(result, evaluate))
            return false;
        if (!consume("?"))
            return true;
        bool condition = result != 0;
        qlonglong whenTrue = 0;
        if (!parseExpression(whenTrue, evaluate && condition) || !consume(":"))
            return false;
        qlonglong whenFalse = 0;
        if (!parseExpression(whenFalse, evaluate && !condition))
            return false;
        if (evaluate)
            result = condition ? whenTrue : whenFalse;
        return true;
    }

    bool consume(const char *text)
    {
        if (atEnd() || mTokens[mPos].text != QLatin1String(text))
            return false;
        ++mPos;
        return true;
    }

    bool atEnd() const { return mPos >= mTokens.count(); }

    const QVector<PpToken> &mTokens;
    int mPos = 0;
};
}

CppPreprocessor::CppPreprocessor()
{
    mPreprocessorHandlers.insert("if",[this](Session &session, const QString& tokens){ handleIf(session, tokens);});
    mPreprocessorHandlers.insert("ifdef",[this](Session &session, const QString& tokens){ handleIfdef(session, tokens);});
    mPreprocessorHandlers.insert("ifndef",[this](Session &session, const QString& tokens){ handleIfndef(session, tokens);});
    mPreprocessorHandlers.insert("elif",[this](Session &session, const QString& tokens){ handleElif(session, tokens);});
    mPreprocessorHandlers.insert("elifdef",[this](Session &session, const QString& tokens){ handleElifdef(session, tokens);});
    mPreprocessorHandlers.insert("elifndef",[this](Session &session, const QString& tokens){ handleElifndef(session, tokens);});
    mPreprocessorHandlers.insert("else",[this](Session &session, const QString& tokens){ handleElse(session, tokens);});
    mPreprocessorHandlers.insert("endif",[this](Session &session, const QString& tokens){ handleEndif(session, tokens);});
    mPreprocessorHandlers.insert("define",[this](Session &session, const QString& tokens){ handleDefine(session, tokens);});
    mPreprocessorHandlers.insert("undef",[this](Session &session, const QString& tokens){ handleUndefine(session, tokens);});
    mPreprocessorHandlers.insert("include",[this](Session &session, const QString& tokens){ handleInclude(session, tokens);});
    mPreprocessorHandlers.insert("include_next",[this](Session &session, const QString& tokens){ handleIncludeNext(session, tokens);});
    mPreprocessorHandlers.insert("pragma",[this](Session &session, const QString& tokens){ handlePragma(session, tokens);});
    mParseLocal = true;
    mParseSystem = true;
}

void CppPreprocessor::resetForReuse()
{
    mRegistry.clearAnalysisResults();
    mProjectFiles.clear();
    mHardDefines.clear();
    mProjectIncludePaths.clear();
    mIncludePathList.clear();
    mProjectIncludePathList.clear();
    mIncludePaths.clear();
    mCppVersion = CppVersion::Cpp17;
    mCompilerType = CppCompilerType::GCC;
}

void CppPreprocessor::clearAnalysisResults()
{
    mRegistry.clearAnalysisResults();
}

QString CppPreprocessor::boundaryAncestor(const Session &session, const QString &fileName, int occurrence) const
{
    const QStringList ancestors = session.boundaryAncestors.value(fileName);
    if (occurrence < 0 || occurrence >= ancestors.count())
        return QString();
    return ancestors[occurrence];
}

QHash<QString,QString> CppPreprocessor::userMacroSnapshot(const Session &session) const
{
    QHash<QString,QString> result;
    for (auto it = session.defines.constBegin(); it != session.defines.constEnd(); ++it) {
        if (!it.value()->hardCoded)
            result.insert(it.key(), signatureOfDefine(it.value()));
    }
    return result;
}

bool CppPreprocessor::hasChangedFiles(const QString &fileName, const QSet<QString> &includes) const
{
    PreprocessorProfileScope profileScope(preprocessorProfile.fileStampsNs,
                                          preprocessorProfile.fileStampsCalls);
    QSet<QString> files = includes;
    files.insert(fileName);
    for (const QString &fileName : files) {
        PParsedFileInfo fileInfo = findFileInfo(fileName);
        if (!fileInfo || fileInfo->fileSize() < 0)
            continue;
        QFileInfo diskInfo(fileName);
        if (!diskInfo.exists() || diskInfo.lastModified() != fileInfo->lastModified()
                || diskInfo.size() != fileInfo->fileSize())
            return true;
    }
    return false;
}

bool CppPreprocessor::isIncludeProtected(const QString &fileName) const
{
    PParsedFileInfo fileInfo = findFileInfo(fileName);
    return fileInfo && fileInfo->includeProtected();
}

PReplayRecord CppPreprocessor::replayRecordForFile(const Session &session, const QString &fileName) const
{
    PReplayRecord record = findReplayRecord(fileName);
    if (record)
        return record;
    if (!session.includeStack.isEmpty()) {
        const QString &ancestor = session.includeStack.back()->boundaryAncestor;
        if (!ancestor.isEmpty())
            return findReplayRecord(ancestor);
    }
    return PReplayRecord();
}

QSet<QString> CppPreprocessor::calculateAffectedFiles(const QString &fileName) const
{
    QSet<QString> affected{fileName};
    for (auto it = mRegistry.replayRecords.constBegin();
            it != mRegistry.replayRecords.constEnd(); ++it) {
        PParsedFileInfo rootInfo = findFileInfo(it.key());
        if (rootInfo && rootInfo->including(fileName))
            affected.insert(it.key());
    }
    return affected;
}

void CppPreprocessor::invalidateFileAndDependents(const QString &fileName)
{
    QSet<QString> affected = calculateAffectedFiles(fileName);
    for (const QString &affectedFile : affected) {
        if (mFileInvalidationHandler)
            mFileInvalidationHandler(affectedFile);
        else
            removeReplayRecord(affectedFile);
    }
}

void CppPreprocessor::invalidateFileAndDependents(Session &session, const QString &fileName)
{
    QSet<QString> affected = calculateAffectedFiles(fileName);
    for (const PParsedFile &parsedFile : session.includeStack) {
        if (affected.contains(parsedFile->fileName)) {
            mRegistry.pendingInvalidations.insert(parsedFile->fileName);
            affected.remove(parsedFile->fileName);
        }
    }
    for (const QString &affectedFile : affected) {
        if (mFileInvalidationHandler)
            mFileInvalidationHandler(affectedFile);
        else
            removeReplayRecord(affectedFile);
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

void CppPreprocessor::addDefineByParts(Session *session, const QString &fileName, const QString &name, const QString &args, const QString &value, bool hardCoded)
{
    // Check for duplicates
    PDefine define = std::make_shared<Define>();
    define->name = name;
    define->args = args;
    define->value = value;
    define->filename = fileName;
    define->hardCoded = hardCoded;
    if (args.isEmpty()) {
        define->valueTokens = tokenizeMacroValue(value);
    } else {
        // precompile the replacement list for the pp-token engine
        define->paramNames = splitMacroParams(
                define->args.mid(1, define->args.length()-2));
        define->replacementList = compileReplacementList(
                define->value, define->paramNames);
    }
    if (hardCoded) {
        mHardDefines.insert(name,define);
        if (session)
            session->defines.insert(name,define);
        if (name == "__cplusplus") {
            // strip integer suffixes (U, L, UL, LL, ULL) before converting
            QString v = value.trimmed();
            while (v.endsWith('U') || v.endsWith('u')
                   || v.endsWith('L') || v.endsWith('l'))
                v.chop(1);
            bool ok;
            long version = v.toLong(&ok);
            mCppVersion = ok ? static_cast<int>(version) : 0;
        } else if (name == "__clang__") {
            mCompilerType = CppCompilerType::Clang;
        }
    } else {
        PReplayRecord replayRecord = replayRecordForFile(*session, fileName);
        if (replayRecord) {
            replayRecord->defines()->insert(define->name,define);
            replayRecord->undefines()->remove(name);
        }
        session->defines.insert(name,define);
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
    addDefineByParts(nullptr, QString(), name, args, value, hardCoded);
}


void CppPreprocessor::preprocess(Session &session, const QString &fileName)
{
    session = Session{};
    mStopPreprocessing = false;
    if (stopRequested())
        return;
    QSet<QString> pendingInvalidations = mRegistry.pendingInvalidations;
    mRegistry.pendingInvalidations.clear();
    for (const QString &pendingFile : pendingInvalidations)
        invalidateFileAndDependents(session, pendingFile);
    session.defines = mHardDefines;
    session.fileName = fileName;
    PParsedFileInfo oldRootInfo = findFileInfo(fileName);
    if (oldRootInfo && hasChangedFiles(oldRootInfo->fileName(), oldRootInfo->includes()))
        invalidateFileAndDependents(session, fileName);
    else
        removeReplayRecord(fileName);
    openInclude(session, fileName);
    preprocessBuffer(session);
    if (preprocessorProfile.enabled)
        qWarning() << "PREPROCESSOR_PROTECTION scannedIncludes" << session.scannedIncludeCount;
}

void CppPreprocessor::dumpDefinesTo(const Session &session, const QString &fileName) const
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        QTextStream stream(&file);
        for (const PDefine& define:session.defines) {
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
        for (auto it = mRegistry.replayRecords.constBegin();
                it != mRegistry.replayRecords.constEnd(); ++it) {
            PParsedFileInfo fileInfo = findFileInfo(it.key());
            if (!fileInfo)
                continue;
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

void CppPreprocessor::removeReplayRecord(const QString &filename)
{
#ifdef REDPANDA_PARSER_TEST
    if (mRegistry.replayRecords.contains(filename))
        mInvalidateCounts[filename]++;
#endif
    mRegistry.replayRecords.remove(filename);
}

QString CppPreprocessor::expandMacros(QString text) const
{
    Session session;
    session.defines = mHardDefines;
    return expandMacros(session, text);
}

QString CppPreprocessor::expandMacros(const Session &session, QString text) const
{
    QStringList lines{text};
    PpTokenizer tokenizer(lines);
    PpExpander expander([this, &session](const QString& name) {
        return getDefine(session, name);
    });
    expander.setCompilerType(mCompilerType);
    QString out;
    expander.expandLine(tokenizer, out);
    return out;
}

QString CppPreprocessor::getNextPreprocessor(Session &session)
{
    skipToPreprocessor(session); // skip until # at start of line
    if (session.index >= session.buffer.count())
        return ""; // we've gone past the final #preprocessor line. Yay

    // Calculate index to insert defines in in result file
    session.preProcIndex = (session.result.count() - 1) + 1; // offset by one for #include rootfile

    // Assemble whole line, convert newlines to space
    QString result = session.buffer[session.index];
    session.result.append("");// defines resolve into empty files, except #define and #include
    return result;
}

void CppPreprocessor::handleDefine(Session &session, const QString &tokens)
{
    PreprocessorProfileScope profileScope(preprocessorProfile.handleDefineNs,
                                          preprocessorProfile.handleDefineCalls);
    if (getCurrentBranch(session) == BranchResult::isTrue) {
        QString name,args,value;
        getDefineParts(tokens, name, args, value);
        PParsedFile current = session.includeStack.back();
        if (name == current->includeGuardToken)
            current->includeGuardDefined = true;
        // Add to the list
        addDefineByParts(&session, session.fileName, name, args, value, false);
        session.result[session.preProcIndex] = "#define " + tokens; // add define to result file so the parser can handle it
    }
}

void CppPreprocessor::handleInclude(Session &session, const QString &tokens, bool fromNext)
{
    PreprocessorProfileScope profileScope(preprocessorProfile.handleIncludeNs,
                                          preprocessorProfile.handleIncludeCalls);
    if (getCurrentBranch(session)!=BranchResult::isTrue) // we're skipping due to a branch failure
        return;

    PParsedFile file = session.includeStack.back();
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
        s = expandMacros(session, s);

    fileName = getHeaderFilename(
                file->fileName,
                s,
                includes,
                projectIncludes);

    if (fileName.isEmpty())
        return;

    PParsedFileInfo oldCurrentIncludes = session.currentFileInfo;
    openInclude(session, fileName);
}

void CppPreprocessor::handlePreprocessor(Session &session, const QString& command, const QString& tokens)
{
    if (!session.includeStack.isEmpty()
            && session.includeStack.back()->guardClosed)
        session.includeStack.back()->contentAfterGuard = true;
    PreprocessorHandler handler = mPreprocessorHandlers.value(command);
    if (handler) {
        handler(session, tokens);
        if (!command.startsWith("include") && !command.startsWith("pragma")
                && !command.startsWith("undef"))
            session.fileJustOpenned = false;
    }
}

void CppPreprocessor::handleUndefine(Session &session, const QString& tokens)
{
    if (getCurrentBranch(session) != BranchResult::isTrue)
        return;
    // Remove undef
    QString name = tokens.trimmed();

//    //may be defined many times
//    while (true) {
    PDefine define = getDefine(session, name);
    if (define) {
        //remove the define from defines set
        session.defines.remove(name);
        PReplayRecord replayRecord = replayRecordForFile(session, session.fileName);
        if (replayRecord) {
            replayRecord->defines()->remove(name);
            if (define->filename != session.fileName) {
                PDefineMap undefineMap = replayRecord->undefines();
                if (!undefineMap->contains(name))
                    undefineMap->insert(name, define);
            }
        }
    }
    session.result[session.preProcIndex] = "#undef " + name; // add define to result file so the parser can handle it

}

void CppPreprocessor::handleIf(Session &session, const QString &tokens)
{
    if (getCurrentBranch(session)!=BranchResult::isTrue) {// we are already inside an if that is NOT being taken
        setCurrentBranch(session, BranchResult::parentIsFalse);// so don't take this one either
    } else {
        bool testResult = evaluateIf(session, tokens);
        setCurrentBranch(session, testResult?(BranchResult::isTrue):(BranchResult::isFalse));
    }
}

QString CppPreprocessor::signatureOfDefine(const PDefine &define)
{
    if (!define)
        return QStringLiteral("\x02");
    return define->args + QStringLiteral("\x01") + define->value;
}

void CppPreprocessor::handleIfdef(Session &session, const QString &tokens)
{
        if (getCurrentBranch(session)!=BranchResult::isTrue) {
            setCurrentBranch(session, BranchResult::parentIsFalse);
        } else {
            QString name = tokens.trimmed();
            bool defined = getDefine(session, name)
                    || PpExpander::supportsConditionBuiltin(mCompilerType, name);
            setCurrentBranch(session,  defined?(BranchResult::isTrue):(BranchResult::isFalse) );
        }
}

void CppPreprocessor::handleIfndef(Session &session, const QString &tokens)
{
    if (getCurrentBranch(session)!=BranchResult::isTrue) {
        setCurrentBranch(session, BranchResult::parentIsFalse);
    } else {
        QString name = tokens.trimmed();

        if (session.fileJustOpenned) {
            session.includeStack.back()->guardBranchDepth = session.branchResults.count();
            session.includeStack.back()->includeGuardToken = name;
        }
        bool defined = getDefine(session, name)
                || PpExpander::supportsConditionBuiltin(mCompilerType, name);
        setCurrentBranch(session,  !defined?(BranchResult::isTrue):(BranchResult::isFalse) );
    }
}

void CppPreprocessor::handleElif(Session &session, const QString &tokens)
{
    BranchResult oldResult = getCurrentBranch(session); // take either if or else
    removeCurrentBranch(session);
    BranchResult elseResult = calcElseBranchResult(oldResult);
    if (elseResult == BranchResult::isTrue) { // don't take this one, if  previous has been taken
        bool testResult = evaluateIf(session, tokens);
        setCurrentBranch(session, testResult?(BranchResult::isTrue):(BranchResult::isFalse));
    } else {
        setCurrentBranch(session, elseResult);
    }
}

void CppPreprocessor::handleElifdef(Session &session, const QString &tokens)
{
    if (supportCPP23()) {
        BranchResult oldResult = getCurrentBranch(session); // take either if or else
        removeCurrentBranch(session);
        BranchResult elseResult = calcElseBranchResult(oldResult);
        if (elseResult == BranchResult::isTrue) { // don't take this one, if  previous has been taken
            QString name = tokens.trimmed();

            setCurrentBranch(session,  getDefine(session, name)!=nullptr?(BranchResult::isTrue):(BranchResult::isFalse) );
        } else {
            setCurrentBranch(session, elseResult);
        }
    }
}

void CppPreprocessor::handleElifndef(Session &session, const QString &tokens)
{
    if (supportCPP23()) {
        BranchResult oldResult = getCurrentBranch(session); // take either if or else
        removeCurrentBranch(session);
        BranchResult elseResult = calcElseBranchResult(oldResult);
        if (elseResult == BranchResult::isTrue) { // don't take this one, if  previous has been taken
            QString name = tokens.trimmed();

            setCurrentBranch(session,  getDefine(session, name)==nullptr?(BranchResult::isTrue):(BranchResult::isFalse) );
        } else {
            setCurrentBranch(session, elseResult);
        }
    }
}

void CppPreprocessor::handleElse(Session &session, const QString &tokens)
{
    BranchResult oldResult = getCurrentBranch(session); // take either if or else
    removeCurrentBranch(session);
    setCurrentBranch(session, calcElseBranchResult(oldResult));
}

void CppPreprocessor::handleEndif(Session &session, const QString &tokens)
{
    PParsedFile current = session.includeStack.back();
    removeCurrentBranch(session);
    if (current->guardBranchDepth >= 0
            && session.branchResults.count() == current->guardBranchDepth
            && current->includeGuardDefined)
        current->guardClosed = true;
}

void CppPreprocessor::handleInclude(Session &session, const QString &tokens)
{
    handleInclude(session, tokens,false);
}

void CppPreprocessor::handleIncludeNext(Session &session, const QString &tokens)
{
    handleInclude(session, tokens,true);
}

void CppPreprocessor::handlePragma(Session &session, const QString &tokens)
{
    if (tokens.trimmed()=="once" && session.currentFileInfo)
        session.currentFileInfo->setIncludeProtected(true);
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

void CppPreprocessor::openInclude(Session &session, QString fileName)
{
    PreprocessorProfileScope profileScope(preprocessorProfile.openIncludeNs,
                                          preprocessorProfile.openIncludeCalls);
    if (stopRequested())
        return;
    if (session.includeStack.count() >= mMaxIncludeNestingDepth) {
#ifdef REDPANDA_PARSER_TEST
        mSkipCounts[fileName]++;
#endif
        return;
    }

    PParsedFileInfo fileInfo = findFileInfo(fileName);
    if (fileInfo) {
        fileName = fileInfo->fileName();
    } else {
        fileInfo = std::make_shared<ParsedFileInfo>(fileName);
        mRegistry.fileInfos.insert(fileName, fileInfo);
    }

    PParsedFile stackEntry;
    for (const PParsedFile &parsedFile : session.includeStack) {
        if (parsedFile->fileName == fileName) {
            stackEntry = parsedFile;
            break;
        }
    }
    bool inStack = stackEntry != nullptr;

    bool isSystemFile = isSystemHeaderFile(fileName, mIncludePaths)
            || isSystemHeaderFile(fileName, mProjectIncludePaths);
    bool childInside = !mProjectFiles.contains(fileName) && isSystemFile;
    bool boundaryEntry = false;
    QString boundaryAncestor;
    if (session.includeStack.isEmpty()) {
        boundaryEntry = childInside;
        boundaryAncestor = boundaryEntry ? fileName : QString();
    } else {
        const PParsedFile &parent = session.includeStack.back();
        bool parentInside = parent->boundaryEntry || !parent->boundaryAncestor.isEmpty();
        boundaryEntry = !parentInside && childInside;
        boundaryAncestor = boundaryEntry ? fileName : parent->boundaryAncestor;
        if (parentInside && !childInside)
            qWarning() << "Boundary-inside file includes a boundary-outside file:"
                       << parent->fileName << fileName;
    }

    if (!inStack && hasReplayRecord(fileName)
            && hasChangedFiles(fileInfo->fileName(), fileInfo->includes())) {
        invalidateFileAndDependents(session, fileName);
        fileInfo = findFileInfo(fileName);
        if (!fileInfo) {
            fileInfo = std::make_shared<ParsedFileInfo>(fileName);
            mRegistry.fileInfos.insert(fileName, fileInfo);
        }
    }

    bool includeProtected = isIncludeProtected(fileName);
    if (session.includeStack.count() > 0) {
        PreprocessorProfileScope relationsScope(preprocessorProfile.includeRelationsNs,
                                                preprocessorProfile.includeRelationsCalls);
        for (PParsedFile &parsedFile : session.includeStack) {
            parsedFile->fileInfo->addInclude(fileName);
            for (const QString &includedFileName : fileInfo->includes())
                parsedFile->fileInfo->addInclude(includedFileName);
        }
        PParsedFile innerMostFile = session.includeStack.back();
        innerMostFile->fileInfo->addDirectInclude(fileName);
        innerMostFile->index = session.index;
        innerMostFile->branches = session.branchResults.count();
    }

    bool insideActiveGuard = stackEntry
            && !stackEntry->includeGuardToken.isEmpty()
            && session.branchResults.count() > stackEntry->guardBranchDepth
            && getDefine(session, stackEntry->includeGuardToken);
    // Recursive inclusion inside an active, defined guard is a no-op.
    // Unguarded cycles still re-enter and may produce different content.
    if ((includeProtected && session.processed.contains(fileName))
            || insideActiveGuard) {
#ifdef REDPANDA_PARSER_TEST
        mSkipCounts[fileName]++;
#endif
        return;
    }

    bool repeatedUnprotected = session.processed.contains(fileName)
            && !includeProtected;
    bool canReplay = boundaryEntry && !inStack && !repeatedUnprotected
            && hasReplayRecord(fileName);
    PReplayRecord replayRecord = findReplayRecord(fileName);
    if (canReplay) {
        bool valid = session.allBoundaryEntriesReplayed
                && replayRecord && replayRecord->hasReplayCondition()
                && replayRecord->userMacros() == userMacroSnapshot(session)
                && replayRecord->boundaryPrefix() == session.boundaryPrefix;
        if (!valid) {
            invalidateFileAndDependents(session, fileName);
            fileInfo = findFileInfo(fileName);
            if (!fileInfo) {
                fileInfo = std::make_shared<ParsedFileInfo>(fileName);
                mRegistry.fileInfos.insert(fileName, fileInfo);
            }
        } else {
#ifdef REDPANDA_PARSER_TEST
            mReplayCounts[fileName]++;
#endif
            addDefinesInFile(session, fileName);
            session.boundaryPrefix.append(fileName);
            return;
        }
    }

    if (!inStack) {
        QDateTime lastModified = fileInfo->lastModified();
        qint64 fileSize = fileInfo->fileSize();
        fileInfo = std::make_shared<ParsedFileInfo>(fileName);
        fileInfo->setFileMetadata(lastModified, fileSize);
        fileInfo->setIncludeProtected(includeProtected);
        mRegistry.fileInfos.insert(fileName, fileInfo);
    }

    PParsedFile parsedFile = std::make_shared<ParsedFile>();
    parsedFile->index = 0;
    parsedFile->fileName = fileName;
    parsedFile->branches = 0;
    parsedFile->fileInfo = fileInfo;
    parsedFile->boundaryEntry = boundaryEntry;
    parsedFile->boundaryAncestor = boundaryAncestor;
    if (boundaryEntry)
        parsedFile->userMacros = userMacroSnapshot(session);
    parsedFile->boundaryPrefix = session.boundaryPrefix;

    session.currentFileInfo = fileInfo;
    bool replayRoot = boundaryEntry || boundaryAncestor.isEmpty();
    if (replayRoot)
        mRegistry.replayRecords.insert(fileName, std::make_shared<ReplayRecord>());
    bool scanContent = (mParseSystem && isSystemFile) || (mParseLocal && !isSystemFile);
    if (scanContent) {
        if (session.scannedIncludeCount >= INCLUDE_SCAN_STOP_THRESHOLD) {
            qWarning() << "Too many files scanned during preprocessing; stopping at"
                       << session.scannedIncludeCount << fileName;
            mStopPreprocessing = true;
            session.result.clear();
            Q_ASSERT_X(false, "openInclude", "Too many files scanned during preprocessing");
            return;
        }
        ++session.scannedIncludeCount;
        if (session.scannedIncludeCount == INCLUDE_SCAN_WARNING_THRESHOLD) {
            qWarning() << "Preprocessing has scanned" << session.scannedIncludeCount
                       << "files; the latest file is" << fileName;
        }
#ifdef REDPANDA_PARSER_TEST
        mScanCounts[fileName]++;
#endif
    }
    session.processed.insert(fileName);
    session.boundaryAncestors[fileName].append(boundaryAncestor);

    if (session.fileCache.contains(fileName)) {
        parsedFile->buffer = session.fileCache.value(fileName);
    } else if (scanContent) {
        QStringList bufferedText;
        if (mOnGetFileStream && mOnGetFileStream(fileName, bufferedText))
            parsedFile->buffer = bufferedText;
        else {
            PreprocessorProfileScope readScope(preprocessorProfile.readFileNs,
                                               preprocessorProfile.readFileCalls);
            parsedFile->buffer = readFileToLines(fileName);
            QFileInfo diskInfo(fileName);
            fileInfo->setFileMetadata(diskInfo.lastModified(), diskInfo.size());
        }
        combineLinesEndingWithBackslash(parsedFile->buffer);
        replaceCommentsBySpaceChar(parsedFile->buffer);
    }
    session.includeStack.append(parsedFile);

    session.index = parsedFile->index;
    session.fileName = parsedFile->fileName;
    session.buffer = parsedFile->buffer;

    QString includeLine = "#include " + fileName + ":-1";
    if (session.includeStack.count() > 1) {
        session.result[session.preProcIndex] = includeLine;
        session.index = -1;
    } else {
        session.result.append(includeLine);
    }
    if (boundaryEntry) {
        session.allBoundaryEntriesReplayed = false;
        session.boundaryPrefix.append(fileName);
    }
    session.fileJustOpenned = true;
}


void CppPreprocessor::closeInclude(Session &session)
{
    PreprocessorProfileScope profileScope(preprocessorProfile.closeIncludeNs,
                                          preprocessorProfile.closeIncludeCalls);
    if (session.includeStack.isEmpty())
        return;
    PParsedFile lastFile = session.includeStack.back();
    if (lastFile->boundaryEntry) {
        PReplayRecord replayRecord = findReplayRecord(lastFile->fileName);
        Q_ASSERT(replayRecord);
        if (replayRecord)
            replayRecord->setReplayCondition(
                    lastFile->userMacros, lastFile->boundaryPrefix);
    }
    if (lastFile->guardBranchDepth >= 0) {
        bool fullGuard = lastFile->guardClosed && !lastFile->contentAfterGuard;
        lastFile->fileInfo->setIncludeProtected(
                lastFile->fileInfo->includeProtected() || fullGuard);
    }
    if (!isIncludeProtected(lastFile->fileName))
        session.fileCache.insert(lastFile->fileName, lastFile->buffer);
    lastFile->fileInfo->setBranchVisibility(lastFile->branchVisibility);
    session.includeStack.pop_back();

    if (session.includeStack.isEmpty())
        return;
    PParsedFile parsedFile = session.includeStack.back();

    // Continue where we left off
    session.index = parsedFile->index+1;
    session.fileName = parsedFile->fileName;
    // Point to previous buffer and start past the include we walked into
    session.buffer = parsedFile->buffer;
    while (session.branchResults.count()>parsedFile->branches) {
        session.branchResults.pop_back();
    }


    // Start augmenting previous include list again
    session.currentFileInfo = parsedFile->fileInfo;

    // Update result file (we've left the previous file)
    session.result.append(
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

void CppPreprocessor::addDefinesInFile(Session &session, const QString &fileName)
{
    if (session.processed.contains(fileName))
        return;
    session.processed.insert(fileName);

    PReplayRecord replayRecord = findReplayRecord(fileName);
    PDefineMap defineList = replayRecord ? replayRecord->defines() : PDefineMap();
    if (defineList) {
        foreach (const PDefine& define, defineList->values()) {
            session.defines.insert(define->name,define);
        }
    }
    PDefineMap undefineList = replayRecord ? replayRecord->undefines() : PDefineMap();
    if (undefineList) {
        foreach (const PDefine& define, undefineList->values()) {
            session.defines.remove(define->name);
        }
    }

    PParsedFileInfo fileInfo = findFileInfo(fileName);
    if (fileInfo) {
        foreach (const QString& file, fileInfo->includes()) {
            addDefinesInFile(session, file);
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

void CppPreprocessor::preprocessBuffer(Session &session)
{
    while (session.includeStack.count() > 0) {
        QString s;
        do {
            if (stopRequested()) {
                session.buffer.clear();
                return;
            }
            s = getNextPreprocessor(session);
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
                    handlePreprocessor(session, command, tokens);
                }
            }
            // Step over
            session.index++;
        } while (!s.isEmpty());
        closeInclude(session);
    }
}

void CppPreprocessor::skipToPreprocessor(Session &session)
{
    int bufferCount = session.buffer.count();
// Increment until a line begins with a #
    while ((session.index < bufferCount) && !isDirectiveLine(session.buffer[session.index])) {
        if (session.includeStack.back()->guardClosed
                && !session.buffer[session.index].trimmed().isEmpty())
            session.includeStack.back()->contentAfterGuard = true;
        if (getCurrentBranch(session)==BranchResult::isTrue) { // if not skipping, expand current macros
            QString expanded;
            int consumed = expandLineWithEngine(session, session.buffer, session.index, expanded);
            session.result.append(expanded);
            // lines consumed by multi-line macro invocations become
            // empty lines to keep the result aligned with the source
            for (int i = 1; i < consumed; i++) {
                session.result.append("");
            }
            session.index += consumed;
        } else { // If skipping due to a failed branch, clear line
            session.result.append("");
            session.index++;
        }
    }
}

int CppPreprocessor::expandLineWithEngine(Session &session, const QStringList &buffer, int startLine, QString &out)
{
    PreprocessorProfileScope profileScope(preprocessorProfile.expandLineNs,
                                          preprocessorProfile.expandLineCalls);
    PpTokenizer tokenizer(buffer, startLine);
    PpExpander expander([this, &session](const QString& name) {
        return getDefine(session, name);
    });
    expander.setCompilerType(mCompilerType);
    return expander.expandLine(tokenizer, out);
}

bool CppPreprocessor::evaluateIf(const QString &line) const
{
    Session session;
    session.defines = mHardDefines;
    return evaluateIf(session, line);
}

bool CppPreprocessor::evaluateIf(const Session &session, const QString &line) const
{
    PreprocessorProfileScope profileScope(preprocessorProfile.evaluateIfNs,
                                          preprocessorProfile.evaluateIfCalls);
    QElapsedTimer conditionTimer;
    if (preprocessorProfile.enabled)
        conditionTimer.start();
    QVector<PpToken> tokens = expandConditionTokens(session, line);
    qint64 expansionNs = 0;
    if (preprocessorProfile.enabled) {
        expansionNs = conditionTimer.nsecsElapsed();
        preprocessorProfile.expandConditionNs += expansionNs;
    }
    qlonglong result = 0;
    PpConditionEvaluator evaluator(tokens);
    bool valid = evaluator.evaluate(result);
    if (preprocessorProfile.enabled)
        preprocessorProfile.evaluateConditionNs += conditionTimer.nsecsElapsed() - expansionNs;
    return valid && result != 0;
}

QVector<PpToken> CppPreprocessor::expandConditionTokens(const Session &session, const QString &line) const
{
    // #if/#elif expressions are expanded by the same pp-token engine in
    // condition mode (defined/__has_include as operators, undefined
    // identifiers evaluate to 0, references are recorded for the
    // environment-dependency check).
    QStringList lines{line};
    PpTokenizer tokenizer(lines);
    PpExpander expander([this, &session](const QString& name) {
        return getDefine(session, name);
    });
    expander.setCompilerType(mCompilerType);
    expander.setHasIncludeNext([this, &session](const QString &args) {
        QString header = args;
        if (!header.startsWith('<') && !header.startsWith('\"')) {
            QStringList argLines{header};
            PpTokenizer argTokenizer(argLines);
            PpExpander argExpander([this, &session](const QString& name) {
                return getDefine(session, name);
            });
            argExpander.setCompilerType(mCompilerType);
            argExpander.expandLine(argTokenizer, header);
        }
        if (!session.currentFileInfo)
            return false;
        QString currentDir = excludeTrailingPathDelimiter(
                    extractFileDir(session.currentFileInfo->fileName()));
        QStringList includes = mIncludePathList;
        QStringList projectIncludes = mProjectIncludePathList;
        if (mIncludePaths.contains(currentDir)) {
            includes = mIncludePathList.mid(mIncludePathList.indexOf(currentDir) + 1);
        } else if (mProjectIncludePaths.contains(currentDir)) {
            projectIncludes = mProjectIncludePathList.mid(
                        mProjectIncludePathList.indexOf(currentDir) + 1);
        }
        return !getHeaderFilename(session.currentFileInfo->fileName(), header,
                                  includes, projectIncludes).isEmpty();
    });
    expander.setConditionMode(true,
        [this, &session](const QString& args) {
            QString header = args;
            if (!header.startsWith('<') && !header.startsWith('\"')) {
                QStringList argLines{header};
                PpTokenizer argTokenizer(argLines);
                PpExpander argExpander([this, &session](const QString& name) {
                    return getDefine(session, name);
                });
                argExpander.setCompilerType(mCompilerType);
                argExpander.expandLine(argTokenizer, header);
            }
            if (!session.currentFileInfo)
                return false;
            QString fileName = getHeaderFilename(
                    session.currentFileInfo->fileName(),
                    header,
                    mIncludePathList,
                    mProjectIncludePathList);
            return !fileName.isEmpty();
        },
        [this, &session](const QString& name) {

        });
    QVector<PpToken> inputTokens;
    while (true) {
        PpToken token = tokenizer.next();
        if (token.kind == PpTokenKind::EndOfInput || token.kind == PpTokenKind::EndOfLine)
            break;
        inputTokens.append(token);
    }
    return expander.expandTokens(inputTokens);
}

