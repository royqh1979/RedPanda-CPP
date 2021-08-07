#include "cpppreprocessor.h"

CppPreprocessor::CppPreprocessor(QObject *parent) : QObject(parent)
{

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

void CppPreprocessor::addDefinesInFile(const QString &fileName)
{
    if (mProcessed.contains(fileName))
        return;
    mProcessed.insert(fileName);
//    if FastIndexOf(fScannedFiles, FileName) = -1 then
//      Exit;
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
