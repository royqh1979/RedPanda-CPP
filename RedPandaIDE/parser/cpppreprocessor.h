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
    CppPreprocessor(const CppPreprocessor&)=delete;
    CppPreprocessor& operator=(const CppPreprocessor&)=delete;

    void clear();

    void clearTempResults();
    void getDefineParts(const QString& input, QString &name, QString &args, QString &value);
    void addHardDefineByLine(const QString& line);
    void setScanOptions(bool parseSystem, bool parseLocal);
    void preprocess(const QString& fileName);

    void dumpDefinesTo(const QString& fileName) const;
    void dumpIncludesListTo(const QString& fileName) const;
    void addIncludePath(const QString& fileName);
    void addProjectIncludePath(const QString& fileName);
    void clearIncludePaths();
    void clearProjectIncludePaths();
    void removeScannedFile(const QString& filename);

    const QStringList& result() const{
        return mResult;
    };

    QHash<QString, PFileIncludes> &includesList();

    const QHash<QString, PFileIncludes> &includesList() const;

    QSet<QString> &scannedFiles();

    const QSet<QString> &includePaths();

    const QSet<QString> &projectIncludePaths();

    const DefineMap &hardDefines() const;

    const QList<QString> &includePathList() const;

    const QList<QString> &projectIncludePathList() const;
    void setOnGetFileStream(const GetFileStreamCallBack &newOnGetFileStream);

    static QList<PDefineArgToken> tokenizeValue(const QString& value);

private:
    void preprocessBuffer();
    void skipToEndOfPreprocessor();
    void skipToPreprocessor();
    QString getNextPreprocessor();
    void handleBranch(const QString& line);
    void handleDefine(const QString& line);
    void handleInclude(const QString& line, bool fromNext=false);
    void handlePreprocessor(const QString& value);
    void handleUndefine(const QString& line);
    QString expandMacros(const QString& line, int depth);
    QString expandMacros();
    void expandMacro(const QString& line, QString& newLine, QString& word, int& i, int depth);
    void expandMacro(QString& newLine, QString& word, int& i, int depth);
    QString removeGCCAttributes(const QString& line);
    void removeGCCAttribute(const QString&line, QString& newLine, int &i, const QString& word);
    PDefine getDefine(const QString& name) const{
        return mDefines.value(name,PDefine());
    }
    // current file stuff
    PParsedFile getInclude(int index) const {
        return mIncludes[index];
    }
    void openInclude(const QString& fileName);
    void closeInclude();

    // branch stuff
    bool getCurrentBranch(){
        if (!mBranchResults.isEmpty())
            return mBranchResults.last();
        else
            return true;
    }
    void setCurrentBranch(bool value){
        mBranchResults.append(value);
    }
    void removeCurrentBranch(){
        if (mBranchResults.size()>0)
            mBranchResults.pop_back();
    };
    // include stuff
    PFileIncludes getFileIncludesEntry(const QString& fileName){
        return mIncludesList.value(fileName,PFileIncludes());
    }
    void addDefinesInFile(const QString& fileName);
    void addDefineByParts(const QString& name, const QString& args,
                          const QString& value, bool hardCoded);
    void addDefineByLine(const QString& line, bool hardCoded);
    PDefine getHardDefine(const QString& name){
        return mHardDefines.value(name,PDefine());
    };
    void invalidDefinesInFile(const QString& fileName);

    void parseArgs(PDefine define);

    QStringList removeComments(const QStringList& text);
    /*
     * '_','a'..'z','A'..'Z','0'..'9'
     */
static  bool isWordChar(const QChar& ch);
    /*
     * 'A'..'Z', '0'..'9', 'a'..'z', '_', '*', '&', '~'
     */
static  bool isIdentChar(const QChar& ch);
    /*
     * '\r','\n'
     */
static  bool isLineChar(const QChar& ch);
    /*
     *  '\t' ' '
     */
static  bool isSpaceChar(const QChar& ch);
    /*
     * '+', '-', '*', '/', '!', '=', '<', '>', '&', '|', '^'
     */
//static  bool isOperatorChar(const QChar& ch);

    /*
     * 'A'..'Z', 'a'..'z', '_'
     */
static  bool isMacroIdentChar(const QChar& ch);

    /*
     * '0'..'9'
     */
static  bool isDigit(const QChar& ch);

    /*
     * '0'..'9','x',X','a'..'f','A'..'F','u','U','l','L'
     */
static  bool isNumberChar(const QChar& ch);

    QString lineBreak();

    bool evaluateIf(const QString& line);
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

    //temporary data when preprocessing single file
    int mIndex; // points to current file buffer.
    QString mFileName;
    QStringList mBuffer;
    QStringList mResult;
    PFileIncludes mCurrentIncludes;
    int mPreProcIndex;    
    QList<PParsedFile> mIncludes; // stack of files we've stepped into. last one is current file, first one is source file
    QList<bool> mBranchResults;// stack of branch results (boolean). last one is current branch, first one is outermost branch
    DefineMap mDefines; // working set, editable
    QSet<QString> mProcessed; // dictionary to save filename already processed


    //Result across processings.
    //used by parser even preprocess finished
    QHash<QString,PFileIncludes> mIncludesList;
    QHash<QString, PDefineMap> mFileDefines; //dictionary to save defines for each headerfile;
    QSet<QString> mScannedFiles;

    //option data for the parser
    //{ List of current project's include path }
    DefineMap mHardDefines; // set by "cpp -dM -E -xc NUL"
    QSet<QString> mProjectIncludePaths;
    //we also need include paths in order (for #include_next)
    QList<QString> mIncludePathList;
    QList<QString> mProjectIncludePathList;
    //{ List of current compiler set's include path}
    QSet<QString> mIncludePaths;

    bool mParseSystem;
    bool mParseLocal;

    GetFileStreamCallBack mOnGetFileStream;
};

using PCppPreprocessor = std::shared_ptr<CppPreprocessor>;

#endif // CPPPREPROCESSOR_H
