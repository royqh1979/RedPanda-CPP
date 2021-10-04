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
    void addDefineByParts(const QString& name, const QString& args,
                          const QString& value, bool hardCoded);
    void getDefineParts(const QString& input, QString &name, QString &args, QString &value);
    void addDefineByLine(const QString& line, bool hardCoded);
    PDefine getDefine(const QString& name);
    PDefine getHardDefine(const QString& name);
    void reset(); //reset but don't clear generated defines
    void resetDefines();
    void setScanOptions(bool parseSystem, bool parseLocal);
    void preprocess(const QString& fileName, QStringList buffer = QStringList());
    void invalidDefinesInFile(const QString& fileName);

    void dumpDefinesTo(const QString& fileName) const;
    void dumpIncludesListTo(const QString& fileName) const;
    void addIncludePath(const QString& fileName);
    void addProjectIncludePath(const QString& fileName);
    void clearIncludePaths();
    void clearProjectIncludePaths();
    QStringList result() const;

    QHash<QString, PFileIncludes> &includesList();

    QSet<QString> &scannedFiles();

    const QSet<QString> &includePaths();

    const QSet<QString> &projectIncludePaths();

    const DefineMap &hardDefines() const;

signals:

private:
    void preprocessBuffer();
    void skipToEndOfPreprocessor();
    void skipToPreprocessor();
    QString getNextPreprocessor();
    void simplify(QString& output);
    void handleBranch(const QString& line);
    void handleDefine(const QString& line);
    void handleInclude(const QString& line, bool fromNext=false);
    void handlePreprocessor(const QString& value);
    void handleUndefine(const QString& line);
    QString expandMacros(const QString& line, int depth);
    void expandMacro(const QString& line, QString& newLine, QString& word, int& i, int depth);
    QString removeGCCAttributes(const QString& line);
    void removeGCCAttribute(const QString&line, QString& newLine, int &i, const QString& word);

    // current file stuff
    PParsedFile getInclude(int index);
    void openInclude(const QString& fileName, QStringList bufferedText=QStringList());
    void closeInclude();

    // branch stuff
    bool getCurrentBranch();
    void setCurrentBranch(bool value);
    void removeCurrentBranch();
    // include stuff
    PFileIncludes getFileIncludesEntry(const QString& FileName);
    void addDefinesInFile(const QString& fileName);

    void parseArgs(PDefine define);
    QList<PDefineArgToken> tokenizeValue(const QString& value);

    QStringList removeComments(const QStringList& text);
    /*
     * '_','a'..'z','A'..'Z','0'..'9'
     */
    bool isWordChar(const QChar& ch);
    /*
     * 'A'..'Z', '0'..'9', 'a'..'z', '_', '*', '&', '~'
     */
    bool isIdentChar(const QChar& ch);
    /*
     * '\r','\n'
     */
    bool isLineChar(const QChar& ch);
    /*
     *  '\t' ' '
     */
    bool isSpaceChar(const QChar& ch);
    /*
     * '+', '-', '*', '/', '!', '=', '<', '>', '&', '|', '^'
     */
    bool isOperatorChar(const QChar& ch);

    /*
     * 'A'..'Z', 'a'..'z', '_'
     */
    bool isMacroIdentChar(const QChar& ch);

    /*
     * '0'..'9'
     */
    bool isDigit(const QChar& ch);

    /*
     * '0'..'9','x',X','a'..'f','A'..'F','u','U','l','L'
     */
    bool isNumberChar(const QChar& ch);

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
    int mIndex; // points to current file buffer. do not free
    QString mFileName; // idem
    QStringList mBuffer; // idem
    QStringList mResult;
    PFileIncludes mCurrentIncludes;
    int mPreProcIndex;
    QHash<QString,PFileIncludes> mIncludesList;
    DefineMap mHardDefines; // set by "cpp -dM -E -xc NUL"
    DefineMap mDefines; // working set, editable
    QHash<QString, PDefineMap> mFileDefines; //dictionary to save defines for each headerfile;
    QList<PParsedFile> mIncludes; // stack of files we've stepped into. last one is current file, first one is source file
    QList<bool> mBranchResults;// stack of branch results (boolean). last one is current branch, first one is outermost branch
    //{ List of current compiler set's include path}
    QSet<QString> mIncludePaths;
    //{ List of current project's include path }
    QSet<QString> mProjectIncludePaths;
    //we also need include paths in order (for #include_next)
    QList<QString> mIncludePathList;
    QList<QString> mProjectIncludeList;

    bool mParseSystem;
    bool mParseLocal;
    QSet<QString> mScannedFiles;
    QSet<QString> mProcessed; // dictionary to save filename already processed
};

#endif // CPPPREPROCESSOR_H
