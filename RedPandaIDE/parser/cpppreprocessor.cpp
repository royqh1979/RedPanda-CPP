#include "cpppreprocessor.h"

CppPreprocessor::CppPreprocessor(QObject *parent) : QObject(parent)
{

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
