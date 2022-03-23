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

#define MAX_DEFINE_EXPAND_DEPTH 20
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
    PFileIncludes fileIncludes; // includes of this file
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

    explicit CppPreprocessor();
    void clear();
    void clearResult();
    void getDefineParts(const QString& input, QString &name, QString &args, QString &value);
    void addHardDefineByLine(const QString& line);
    void reset(); //reset but don't clear generated defines
    void setScanOptions(bool parseSystem, bool parseLocal);
    void preprocess(const QString& fileName, QStringList buffer = QStringList());

    void dumpDefinesTo(const QString& fileName) const;
    void dumpIncludesListTo(const QString& fileName) const;
    void addIncludePath(const QString& fileName);
    void addProjectIncludePath(const QString& fileName);
    void clearIncludePaths();
    void clearProjectIncludePaths();

    const QStringList &result() const {
        return mResult;
    }

    QHash<QString, PFileIncludes> &includesList() {
        return mIncludesList;
    }

    QSet<QString> &scannedFiles() {
        return mScannedFiles;
    }

    const QSet<QString> &includePaths() {
        return mIncludePaths;
    }

    const QSet<QString> &projectIncludePaths() {
        return mProjectIncludePaths;
    }

    const DefineMap &hardDefines() const {
        return mHardDefines;
    }

    const QList<QString> &includePathList() const {
        return mIncludePathList;
    }

    const QList<QString> &projectIncludePathList() const {
        return mProjectIncludePathList;
    }
private:
    void preprocessBuffer();
    void skipToEndOfPreprocessor();
    void skipToPreprocessor();
    QString getNextPreprocessor();
    void simplify(QString& output) {
        // Remove #
        output = output.mid(1).trimmed();
    }
    void handleBranch(const QString& line);
    void handleDefine(const QString& line) {
        if (getCurrentBranch()) {
            addDefineByLine(line, false);
            mResult[mPreProcIndex] = '#' + line; // add define to result file so the parser can handle it
        }
    }
    void handleInclude(const QString& line, bool fromNext=false);
    void handlePreprocessor(const QString& value);
    void handleUndefine(const QString& line);
    QString expandMacros(const QString& line, int depth);
    void expandMacro(const QString& line, QString& newLine, QString& word, int& i, int depth);
    QString removeGCCAttributes(const QString& line);
    void removeGCCAttribute(const QString&line, QString& newLine, int &i, const QString& word);
    PDefine getDefine(const QString& name) {
        return mDefines.value(name,PDefine());
    }
    // current file stuff
    PParsedFile getInclude(int index) {
        return mIncludes[index];
    }
    void openInclude(const QString& fileName, QStringList bufferedText=QStringList());
    void closeInclude();

    // branch stuff
    bool getCurrentBranch() const{
        if (!mBranchResults.isEmpty())
            return mBranchResults.last();
        else
            return true;
    }
    void setCurrentBranch(bool value) {
        mBranchResults.append(value);
    }
    void removeCurrentBranch() {
        if (mBranchResults.size()>0)
            mBranchResults.pop_back();
    }
    // include stuff
    PFileIncludes getFileIncludesEntry(const QString& fileName) {
        return mIncludesList.value(fileName,PFileIncludes());
    }
    void addDefinesInFile(const QString& fileName);
    void resetDefines() {
        mDefines = mHardDefines;
    }
    void addDefineByParts(const QString& name, const QString& args,
                          const QString& value, bool hardCoded);
    void addDefineByLine(const QString& line, bool hardCoded);
    PDefine getHardDefine(const QString& name) {
        return mHardDefines.value(name,PDefine());
    }
    void invalidDefinesInFile(const QString& fileName);

    void parseArgs(PDefine define);
    QList<PDefineArgToken> tokenizeValue(const QString& value);

    QStringList removeComments(const QStringList& text);
    /*
     * '_','a'..'z','A'..'Z','0'..'9'
     */
    bool isWordChar(const QChar& ch) const{
        return (ch=='_' || (ch>='0' && ch<='9')
                || ch.isLetter()
                );
    }
    /*
     * 'A'..'Z', '0'..'9', 'a'..'z', '_', '*', '&', '~'
     */
    bool isIdentChar(const QChar& ch) const {
        return (ch=='_' || ch == '*' || ch == '&' || ch == '~' ||
                ch.isLetter()
                || (ch>='0' && ch<='9'));
    }
    /*
     * '\r','\n'
     */
    bool isLineChar(const QChar& ch) const {
        return ch=='\r' || ch == '\n';
    }
    /*
     *  '\t' ' '
     */
    bool isSpaceChar(const QChar& ch) const {
        return ch == ' ' || ch == '\t';
    }
    /*
     * '+', '-', '*', '/', '!', '=', '<', '>', '&', '|', '^'
     */
    bool isOperatorChar(const QChar& ch) const {
        switch(ch.unicode()) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '!':
        case '=':
        case '<':
        case '>':
        case '&':
        case '|':
        case '^':
            return true;
        default:
            return false;
        }
    }

    /*
     * 'A'..'Z', 'a'..'z', '_'
     */
    bool isMacroIdentChar(const QChar& ch) const {
        return ch.isLetter()
                || ch == '_';
    }

    /*
     * '0'..'9'
     */
    bool isDigit(const QChar& ch) const {
        return (ch>='0' && ch<='9');
    }

    /*
     * '0'..'9','x',X','a'..'f','A'..'F','u','U','l','L'
     */
    bool isNumberChar(const QChar& ch) const {
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

    QString lineBreak() const{
        return "\n";
    }

    bool evaluateIf(const QString& line) {
        QString newLine = expandDefines(line); // replace FOO by numerical value of FOO
        return  evaluateExpression(newLine);
    }
    QString expandDefines(QString line);
    bool skipBraces(const QString&line, int& index, int step = 1);
    QString expandFunction(PDefine define,QString args);
    bool skipSpaces(const QString &expr, int& pos);
    bool evalNumber(const QString &expr, int& result, int& pos);
    bool evalTerm(const QString &expr, int& result, int& pos);
    bool evalUnaryExpr(const QString &expr, int& result, int& pos);
    bool evalMulExpr(const QString &expr, int& result, int& pos);
    bool evalAddExpr(const QString &expr, int& result, int& pos);
    bool evalShiftExpr(const QString &expr, int& result, int& pos);
    bool evalRelationExpr(const QString &expr, int& result, int& pos);
    bool evalEqualExpr(const QString &expr, int& result, int& pos);
    bool evalBitAndExpr(const QString &expr, int& result, int& pos);
    bool evalBitXorExpr(const QString &expr, int& result, int& pos);
    bool evalBitOrExpr(const QString &expr, int& result, int& pos);
    bool evalLogicAndExpr(const QString &expr, int& result, int& pos);
    bool evalLogicOrExpr(const QString &expr, int& result, int& pos);
    bool evalExpr(const QString &expr, int& result, int& pos);

    int evaluateExpression(QString line);
private:
    int mIndex; // points to current file buffer. do not free
    QString mFileName; // idem
    QStringList mBuffer; // idem
    QStringList mResult;
    PFileIncludes mCurrentIncludes;
    int mPreProcIndex;
    QList<PParsedFile> mIncludes; // stack of files we've stepped into. last one is current file, first one is source file
    QList<bool> mBranchResults;// stack of branch results (boolean). last one is current branch, first one is outermost branch
    DefineMap mDefines; // working set, editable
    QSet<QString> mProcessed; // dictionary to save filename already processed

    //used by parser even preprocess finished
    DefineMap mHardDefines; // set by "cpp -dM -E -xc NUL"
    QHash<QString,PFileIncludes> mIncludesList;
    QHash<QString, PDefineMap> mFileDefines; //dictionary to save defines for each headerfile;
    //{ List of current project's include path }
    QSet<QString> mProjectIncludePaths;
    //we also need include paths in order (for #include_next)
    QList<QString> mIncludePathList;
    QList<QString> mProjectIncludePathList;
    //{ List of current compiler set's include path}
    QSet<QString> mIncludePaths;

    bool mParseSystem;
    bool mParseLocal;
    QSet<QString> mScannedFiles;
};

#endif // CPPPREPROCESSOR_H
