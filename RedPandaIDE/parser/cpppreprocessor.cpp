#include "cpppreprocessor.h"
#include "../utils.h"

CppPreprocessor::CppPreprocessor(QObject *parent) : QObject(parent)
{

}

QString CppPreprocessor::getNextPreprocessor()
{

    skipToPreprocessor(); // skip until # at start of line
    int preProcFrom = mIndex;
    if (preProcFrom >= mBuffer.count())
        return ""; // we've gone past the final #preprocessor line. Yay
    skipToEndOfPreprocessor();
    int preProcTo = mIndex;

    // Calculate index to insert defines in in result file
    mPreProcIndex = (mResult.count() - 1) + 1; // offset by one for #include rootfile

    // Assemble whole line, including newlines
    QString result;
    for (int i=preProcFrom;i<=preProcTo;i++) {
        result+=mBuffer[i]+'\n';
        mResult.append("");// defines resolve into empty files, except #define and #include
    }
    // Step over
    mIndex++;
    return result;
}

void CppPreprocessor::handleBranch(const QString &line)
{
    if (line.startsWith("ifdef")) {
//        // if a branch that is not at our level is false, current branch is false too;
//        for (int i=0;i<=mBranchResults.count()-2;i++) {
//            if (!mBranchResults[i]) {
//                setCurrentBranch(false);
//                return;
//            }
//        }
        if (!getCurrentBranch()) {
            setCurrentBranch(false);
        } else {
            constexpr int IFDEF_LEN = 5; //length of ifdef;
            QString name = line.mid(IFDEF_LEN).trimmed();
            int dummy;
            setCurrentBranch( getDefine(name,dummy)!=nullptr );

        }
    } else if (line.startsWith("ifndef")) {
//        // if a branch that is not at our level is false, current branch is false too;
//        for (int i=0;i<=mBranchResults.count()-2;i++) {
//            if (!mBranchResults[i]) {
//                setCurrentBranch(false);
//                return;
//            }
//        }
        if (!getCurrentBranch()) {
            setCurrentBranch(false);
        } else {
            constexpr int IFNDEF_LEN = 6; //length of ifndef;
            QString name = line.mid(IFNDEF_LEN).trimmed();
            int dummy;
            setCurrentBranch( getDefine(name,dummy)==nullptr );
        }
    } else if (line.startsWith("if")) {
        //        // if a branch that is not at our level is false, current branch is false too;
        //        for (int i=0;i<=mBranchResults.count()-2;i++) {
        //            if (!mBranchResults[i]) {
        //                setCurrentBranch(false);
        //                return;
        //            }
        //        }
        if (!getCurrentBranch()) {// we are already inside an if that is NOT being taken
            setCurrentBranch(false);// so don't take this one either
        } else {
            constexpr int IF_LEN = 2; //length of if;
            QString ifLine = line.mid(IF_LEN).trimmed();

            bool testResult = evaludateIf(ifLine);
            setCurrentBranch(testResult);
        }
    } else if (line.startsWith("else")) {
        bool oldResult = getCurrentBranch(); // take either if or else
        removeCurrentBranch();
        setCurrentBranch(!oldResult);
    } else if (line.startsWith("elif")) {
        bool oldResult = getCurrentBranch(); // take either if or else
        removeCurrentBranch();
        if (oldResult) { // don't take this one, if  previous has been taken
            setCurrentBranch(false);
        } else {
            constexpr int ELIF_LEN = 4; //length of if;
            QString ifLine = line.mid(ELIF_LEN).trimmed();
            bool testResult = evaludateIf(ifLine);
            setCurrentBranch(testResult);
        }
    } else if (line.startsWith("endif")) {
        removeCurrentBranch();
    }
}

QString CppPreprocessor::expandMacros(const QString &line, int depth)
{
    //prevent infinit recursion
    if (depth > 20)
        return line;
    QString word;
    QString newLine;
    int lenLine = line.length();
    int i=0;
    while (i< lenLine) {
        QChar ch=line[i];
        if (isIdentChar(ch)) {
            word += ch;
        } else {
            if (!word.isEmpty()) {
                expandMacro(line,newLine,word,i,depth);
            }
            word = "";
            if (i< lenLine) {
                newLine += ch;
            }
        }
        i++;
    }
    if (!word.isEmpty()) {
        expandMacro(line,newLine,word,i,depth);
    }
    return newLine;
}

void CppPreprocessor::expandMacro(const QString &line, QString &newLine, QString &word, int &i, int depth)
{
    int lenLine = line.length();
    if (word == "__attribute__") {
        //skip gcc __attribute__
        while ((i<lenLine) && (line[i] == ' ' || line[i]=='\t'))
            i++;
        if ((i<lenLine) && (line[i]=="(")) {
            int level=0;
            while (i<lenLine) {
                switch(line[i].unicode()) {
                    case '(':
                        level++;
                    break;
                    case ')':
                        level--;
                    break;
                }
                i++;
                if (level==0)
                    break;
            }
        }
    } else {
        int index;
        PDefine define = getDefine(word,index);
        if (define && define->args=="" && (!define->isMultiLine)) {
            //newLine:=newLine+RemoveGCCAttributes(define^.Value);
            if (define->value != word )
              newLine += expandMacros(define->value,depth+1);
            else
              newLine += word;

        } else if (define && (!define->isMultiLine) && (define->args!="")) {
            while ((i<lenLine) && (line[i] == ' ' || line[i]=='\t'))
                i++;
            int argStart=-1;
            int argEnd=-1;
            if ((i<lenLine) && (line[i]=='(')) {
                argStart =i+1;
                int level=0;
                while (i<lenLine) {
                    switch(line[i].unicode()) {
                        case '(':
                            level++;
                        break;
                        case ')':
                            level--;
                        break;
                    }
                    i++;
                    if (level==0)
                        break;
                }
                if (level==0) {
                    argEnd = i-2;
                    QString arg = line.mid(argStart,argEnd-argStart+1).trimmed();
                    QString formattedValue = define->formatValue;
                    for (int i=0;i<define->argList.count();i++) {
                        formattedValue = formattedValue.arg(arg);
                    }
                    newLine += expandMacros(formattedValue,depth+1);
                }
            }
        } else {
            newLine += word;
        }
    }
}

PParsedFile CppPreprocessor::getInclude(int index)
{
    return mIncludes[index];
}

void CppPreprocessor::closeInclude()
{
    if (mIncludes.isEmpty())
        return;
    mIncludes.pop_back();

    if (mIncludes.isEmpty())
        return;
    PParsedFile parsedFile = mIncludes.back();

    // Continue where we left off
    mIndex = parsedFile->index;
    mFileName = parsedFile->fileName;
    // Point to previous buffer and start past the include we walked into
    mBuffer = parsedFile->buffer;
    while (mBranchResults.count()>parsedFile->branches) {
        mBranchResults.pop_back();
    }


    // Start augmenting previous include list again
    //fCurrentIncludes := GetFileIncludesEntry(fFileName);
    mCurrentIncludes = parsedFile->fileIncludes;

    // Update result file (we've left the previous file)
    mResult.append(
                QString("#include %1:%2").arg(parsedFile->fileName)
                .arg(parsedFile->index+1));
}

bool CppPreprocessor::getCurrentBranch()
{
    if (!mBranchResults.isEmpty())
        return mBranchResults.last();
    else
        return true;
}

QString CppPreprocessor::getResult()
{
    QString s;
    for (QString line:mResult) {
        s.append(line);
        s.append(lineBreak());
    }
    return s;
}

PFileIncludes CppPreprocessor::getFileIncludesEntry(const QString &fileName)
{
    return mIncludesList.value(fileName,PFileIncludes());
}

void CppPreprocessor::addDefinesInFile(const QString &fileName)
{
    if (mProcessed.contains(fileName))
        return;
    mProcessed.insert(fileName);

    //todo: why test this?
    if (!mScannedFiles.contains(fileName))
        return;

    PDefineMap defineList = mFileDefines.value(fileName, PDefineMap());

    if (defineList) {
        for (PDefine define: defineList->values()) {
            mDefines.insert(define->name,define);
        }
    }
    PFileIncludes fileIncludes = getFileIncludesEntry(fileName);
    if (fileIncludes) {
        for (QString s:fileIncludes->includeFiles.keys()) {
            addDefinesInFile(s);
        }
    }
}

bool CppPreprocessor::isIdentChar(const QChar &ch)
{
    if (ch=='_' || (ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || (ch>='0' && ch<='9')) {
        return true;
    }
    return false;
}

QString CppPreprocessor::lineBreak()
{
    return "\n";
}
