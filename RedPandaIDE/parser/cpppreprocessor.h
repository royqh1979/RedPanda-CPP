#ifndef CPPPREPROCESSOR_H
#define CPPPREPROCESSOR_H

#include <QObject>
#include <QTextStream>
#include "utils.h"

struct ParsedFile {
    int index; // 0-based for programming convenience
    QString fileName; // Record filename, but not used now
    QStringList buffer; // do not concat them all
    int branches; //branch levels;
    PFileIncludes fileIncludes; // includes of this file
};
using PParsedFile = std::shared_ptr<ParsedFile>;

class CppPreprocessor : public QObject
{
    Q_OBJECT
public:

    explicit CppPreprocessor(QObject *parent = nullptr);
    void clear();
    void addDefineByParts(const QString& name, const QString& args,
                          const QString& value, bool hardCoded);
    void getDefineParts(const QString& Input, QString &name, QString &args, QString &value);
    void addDefineByLine(const QString& line, bool hardCoded);
    PDefine getDefine(const QString& name, int &index);
    PDefine getHardDefine(const QString& name, int &index);
    void reset(); //reset but don't clear generated defines
    void resetDefines();
    void setScanOptions(bool parseSystem, bool parseLocal);
    void setIncludePaths(QStringList &list);
    void setProjectIncludePaths(QStringList& list);
    void setScannedFileList(QStringList &list);
    void setIncludesList(QString& list);
    void preprocessStream(const QString& fileName, QTextStream stream = QTextStream());
    void preprocessFile(const QString& fileName);
    void invalidDefinesInFile(const QString& fileName);
signals:

private:
    void preprocessBuffer();
    void skipToEndOfPreprocessor();
    void skipToPreprocessor();
    QString getNextPreprocessor();
    void simplify(QString& output);
    void handleBranch(const QString& line);
    void handleDefine(const QString& line);
    void handleInclude(const QString& line);
    void handlePreprocessor(const QString& value);
    void handleUndefine(const QString& line);
    QString expandMacros(const QString& line, int depth);
    void expandMacro(const QString& line, QString& newLine, QString& word, int& i, int depth);
    QString removeGCCAttributes(const QString& line);
    QString removeSuffixes(const QString& input);
    // current file stuff
    PParsedFile getInclude(int index);
    void openInclude(const QString& fileName, QTextStream stream = QTextStream());
    void closeInclude();

    // branch stuff
    bool getCurrentBranch();
    void setCurrentBranch(bool value);
    void removeCurrentBranch();
    QString getResult();
    // include stuff
    PFileIncludes getFileIncludesEntry(const QString& FileName);
    void addDefinesInFile(const QString& fileName);

    bool isIdentChar(const QChar& ch);

    QString lineBreak();
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
    QStringList mIncludePaths; // path to include folders
    QStringList mProjectIncludePaths;
    bool mParseSystem;
    bool mParseLocal;
    QSet<QString> mScannedFiles;
    QSet<QString> mProcessed; // dictionary to save filename already processed
};

#endif // CPPPREPROCESSOR_H
