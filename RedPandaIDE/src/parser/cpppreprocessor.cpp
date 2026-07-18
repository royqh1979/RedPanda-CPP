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
#include "cpppreprocessor.h"

#include <QFile>
#include <QDebug>
#include <QMessageBox>
#include <QMultiHash>
#include <qt_utils/utils.h>

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
    mFileOnlyIncludeOnce = true;
}

void CppPreprocessor::clear()
{
    //don't use reset(), it will reset(add) defines.
    clearTempResults();

    //Result across processings.
    //used by parser even preprocess finished
    mFileInfos.clear();
    mFileDefines.clear(); //dictionary to save defines for each headerfile;
    mFileUndefines.clear(); //dictionary to save undefines for each headerfile;
    mScannedFiles.clear();

    //option data for the parser
    //{ List of current project's include path }
    mHardDefines.clear();
    mDefines.clear();
    //mHardDefines.clear(); // set by "cpp -dM -E -xc NUL"
    mProjectIncludePaths.clear();
    //we also need include paths in order (for #include_next)
    mIncludePathList.clear();
    mProjectIncludePathList.clear();
    //{ List of current compiler set's include path}
    mIncludePaths.clear();
    mSupportCPP23=false;
}

void CppPreprocessor::clearTempResults()
{    
    //temporary data when preprocessing single file
    mFileName="";
    mBuffer.clear();
    mResult.clear();
    mCurrentFileInfo=nullptr;
    mFileJustOpenned = false;
    mFileIncludeOnceToken = "";
    mFilesCouldRepeatInclude.clear();
    mFileCache.clear();
    mIncludeStack.clear(); // stack of files we've stepped into. last one is current file, first one is source file
    mBranchResults.clear();// stack of branch results (boolean). last one is current branch, first one is outermost branch
    //mDefines.clear(); // working set, editable
    mProcessed.clear(); // dictionary to save filename already processed
}

void CppPreprocessor::addDefineByParts(const QString &name, const QString &args, const QString &value, bool hardCoded)
{
    // Check for duplicates
    PDefine define = std::make_shared<Define>();
    define->name = name;
    define->args = args;
    define->value = value;
    define->filename = mFileName;
    //define->argList;
    define->formatValue = value;
    define->hardCoded = hardCoded;
    define->varArgIndex = -1;
    if (!args.isEmpty())
        parseArgs(define);
    if (hardCoded) {
        mHardDefines.insert(name,define);
        mDefines.insert(name,define);
        if (name == "__cplusplus") {
            bool ok;
            int version = value.toLong(&ok);
            mSupportCPP23 = (ok && version >=202302);
        }
    } else {
        PDefineMap defineMap = mFileDefines.value(mFileName,PDefineMap());
        if (!defineMap) {
            defineMap = std::make_shared<DefineMap>();
            mFileDefines.insert(mFileName,defineMap);
        }
        defineMap->insert(define->name,define);
        mDefines.insert(name,define);
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
    mFileName = fileName;
    openInclude(fileName);
    preprocessBuffer();
}

void CppPreprocessor::invalidDefinesInFile(const QString &fileName)
{
    //remove all defines defined in this file
    PDefineMap defineMap = mFileDefines.value(fileName,PDefineMap());
    if (defineMap) {
        foreach (const PDefine& define, *defineMap) {
            const PDefine& p = mDefines.value(define->name);
            if (p == define) {
                mDefines.remove(define->name);
                PDefine p2 = mHardDefines.value(define->name);
                if (p2)
                    mDefines.insert(define->name, p2);
            }
        }
        mFileDefines.remove(fileName);
    }
    //restore all defines undefined in this file
    PDefineMap undefineMap = mFileUndefines.value(fileName,PDefineMap());
    if (undefineMap) {
        foreach (const PDefine& define, *undefineMap) {
            mDefines.insert(define->name, define);
        }
        mFileUndefines.remove(fileName);
    }
}

void CppPreprocessor::dumpDefinesTo(const QString &fileName) const
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        QTextStream stream(&file);
        for (const PDefine& define:mDefines) {
            stream<<QString("%1 %2 %3 %4 %5\n")
                    .arg(define->name,define->args,define->value)
                    .arg(define->hardCoded).arg(define->formatValue)<<Qt::endl;
        }
    }
}

void CppPreprocessor::dumpIncludesListTo(const QString &fileName) const
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        QTextStream stream(&file);
        for (const PParsedFileInfo& fileInfo:mFileInfos) {
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
    invalidDefinesInFile(filename);
    mScannedFiles.remove(filename);
    mFileInfos.remove(filename);
    mFileDefines.remove(filename);
    mFileUndefines.remove(filename);
}

QString CppPreprocessor::expandMacros(QString text) const
{
    QSet<QString> dummySet;
    return const_cast<CppPreprocessor*>(this)->expandMacros(text, false, dummySet);
}

QString CppPreprocessor::getNextPreprocessor()
{
    skipToPreprocessor(); // skip until # at start of line
    if (mIndex >= mBuffer.count())
        return ""; // we've gone past the final #preprocessor line. Yay

    // Calculate index to insert defines in in result file
    mPreProcIndex = (mResult.count() - 1) + 1; // offset by one for #include rootfile

    // Assemble whole line, convert newlines to space
    QString result = mBuffer[mIndex];
    mResult.append("");// defines resolve into empty files, except #define and #include
    return result;
}

QString CppPreprocessor::expandMacros(QString text, bool handleBuffer)
{
    QSet<QString> dummySet;
    return expandMacros(text, handleBuffer, dummySet);
}

void CppPreprocessor::handleDefine(const QString &tokens)
{
    if (getCurrentBranch() == BranchResult::isTrue) {
        QString name,args,value;
        getDefineParts(tokens, name, args, value);
        if (name == mFileIncludeOnceToken) {
            mFileIncludeOnceToken = "";
            mFilesCouldRepeatInclude.remove(mIncludeStack.back()->fileName);
            //qDebug()<<"- "<<mIncludeStack.back()->fileName;
        }
        // Add to the list
        addDefineByParts(name, args, value, false);
        mResult[mPreProcIndex] = "#define " + tokens; // add define to result file so the parser can handle it
    }
}

void CppPreprocessor::handleInclude(const QString &tokens, bool fromNext)
{
    if (getCurrentBranch()!=BranchResult::isTrue) // we're skipping due to a branch failure
        return;

    PParsedFile file = mIncludeStack.back();
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

    PParsedFileInfo oldCurrentIncludes = mCurrentFileInfo;
    openInclude(fileName);
}

void CppPreprocessor::handlePreprocessor(const QString& command, const QString& tokens)
{
    std::function<void(const QString& tokens)> handler = mPreprocessorHandlers.value(command);
    if (handler) {
        handler(tokens);
        if (!command.startsWith("include") && !command.startsWith("pragma")
                && !command.startsWith("undef"))
            mFileJustOpenned = false;
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
        mDefines.remove(name);
        if (define->filename == mFileName) {
            //remove the define form the file where it defines
            PDefineMap defineMap = mFileDefines.value(mFileName);
            if (defineMap) {
                defineMap->remove(name);
            }
        } else {
            // add it to undefine map
            PDefineMap undefineMap = mFileUndefines.value(mFileName);
            if (!undefineMap) {
                undefineMap = std::make_shared<DefineMap>();
                mFileUndefines.insert(mFileName,undefineMap);
            }
            if (!undefineMap->contains(name))
                undefineMap->insert(name, define);
        }
    }
    mResult[mPreProcIndex] = "#undef " + name; // add define to result file so the parser can handle it

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
        if (mFileJustOpenned) {
            mFileIncludeOnceToken = name;
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
    removeCurrentBranch();
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
    if (tokens.trimmed()=="once")
        mFilesCouldRepeatInclude.remove(mIncludeStack.back()->fileName);
}

QString CppPreprocessor::expandMacros(QString text, bool handleBuffer, const QSet<QString>& macrosToBeIgnored)
{
    QString newLine;
    ContentType currentType = ContentType::Other;
    int lenLine = text.length();
    int prevI = 0;
    int i=0;
    int wordStart = 0;
    QString word;
    QString delimiter;
    QMultiHash<int,QString> tempIngoreMacros; // endLine, name
    bool lastWordNotProcessed = false;;
    while (i<lenLine) {
        QChar ch;
        if (!lastWordNotProcessed) {
            for(int t=prevI;t<i;t++)
                tempIngoreMacros.remove(t);
            prevI = i;
            ch=text[i];
        } else {
            ch=' ';
        }
        if (currentType == ContentType::Other && isWordChar(ch)) {
            if (word.isEmpty())
                wordStart = i;
            word += ch;
            if (i==lenLine-1) {
                lastWordNotProcessed=true;
                continue;
            }
        } else {
            if (lastWordNotProcessed) {
                lastWordNotProcessed = false;
                i++;
            }
            if (!word.isEmpty()) {
                QSet<QString> macrosUsed;
                QSet<QString> ignores=macrosToBeIgnored;
                foreach(const QString& name, tempIngoreMacros)
                    ignores.insert(name);
                QString newWord = expandMacro(text,word,i, handleBuffer,ignores,macrosUsed);
                if (!macrosUsed.isEmpty()) {
                    //adjust ignore macro list
                    QMultiHash<int,QString> tempMacros2 = tempIngoreMacros;
                    tempIngoreMacros.clear();
                    int diff = newWord.length()-word.length();
                    foreach(int idx, tempMacros2.uniqueKeys()) {
                        QList<QString> names = tempMacros2.values(idx);
                        foreach(const QString& name, names)
                            tempIngoreMacros.insert(idx+diff,name);
                    }
                    //rescan (see ISO/IEC 9899:1999 6.10.3.4 Rescanning and futher replacement)
                    foreach(const QString& name, macrosUsed) {
                        tempIngoreMacros.insert(wordStart+newWord.length(), name);
                    }
                    text.replace(wordStart, i-wordStart, newWord);
                    //text = text.left(wordStart)+newWord+text.mid(i);
                    i = wordStart;
                    lenLine = text.length();
                    word = "";
                    continue;
                } else
                    newLine += newWord;
                word = "";
            }
            if (i<lenLine) {
                ch = text[i];
                newLine += text[i];
                switch (ch.unicode()) {
                case '"':
                    switch (currentType) {
                    case ContentType::String:
                        currentType=ContentType::Other;
                        break;
                    case ContentType::RawString:
                        if (QStringView(text.constData(),i).endsWith(')'+delimiter))
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
                    break;
                case 'R':
                    if (currentType == ContentType::Other && i+1<lenLine && text[i+1]=='"') {
                        i++;
                        newLine += text[i];
                        currentType=ContentType::RawStringPrefix;
                        delimiter = "";
                    }
                    break;
                case '(':
                    switch(currentType) {
                    case ContentType::RawStringPrefix:
                        currentType = ContentType::RawString;
                        break;
                    default:
                        break;
                    }
                    break;
                case '\\':
                    switch (currentType) {
                    case ContentType::String:
                    case ContentType::Character:
                        i++;
                        newLine += text[i];
                        break;
                    default:
                        break;
                    }
                    break;
                }
            }
        }
        Q_ASSERT(currentType==ContentType::Other || word.isEmpty());
        i++;
    }
    return newLine;
}

QString CppPreprocessor::expandMacros(QString text, const QSet<QString>& macrosToBeIgnored) const
{
    QSet<QString> dummySet;
    return const_cast<CppPreprocessor*>(this)->expandMacros(text, false, macrosToBeIgnored);
}

QString CppPreprocessor::expandMacro(QString &text, const QString &word, int &i, bool handleBuffer, const QSet<QString> &macrosToBeIgnored, QSet<QString> &macrosUsed){
    if (macrosToBeIgnored.contains(word)) {
        return word;
    }
    int lenLine = text.length();
    PDefine define = getDefine(word);
    if (define && define->args=="" ) {
        macrosUsed.insert(word);
        return define->value;
    } else if (define && (define->args!="")) {
        int oldI = i;
        int oldIndex = mIndex;
        //skip spaces;
        if (handleBuffer) {
            while (true) {
                while ((i<lenLine) && (text[i] == ' ' || text[i]=='\t'))
                    i++;
                if (i>=lenLine && mIndex+1<mBuffer.count()) {
                    mIndex++;
                    text += " "+mBuffer[mIndex];
                    lenLine += mBuffer[mIndex].length()+1;
                } else
                    break;
            }
        } else {
            while ((i<lenLine) && (text[i] == ' ' || text[i]=='\t'))
                i++;
        }
        int argStart=-1;
        int argEnd=-1;
        if ((i<lenLine) && (text[i]=='(')) {
            argStart =i+1;
            int level=0;
            bool inString=false;
            while (i<lenLine) {
                switch(text[i].unicode()) {
                case '\\':
                    if (inString)
                        i++;
                break;
                case '"':
                    inString = !inString;
                break;
                case '(':
                    if (!inString)
                        level++;
                break;
                case ')':
                    if (!inString)
                        level--;
                }
                i++;
                if (level==0)
                    break;
                if (handleBuffer) {
                    while (i>=lenLine && mIndex+1<mBuffer.count()) {
                        mIndex++;
                        text += " "+mBuffer[mIndex];
                        lenLine += mBuffer[mIndex].length()+1;
                    }
                }
            }
            if (level==0) {
                argEnd = i-2;
                QString args = text.mid(argStart,argEnd-argStart+1).trimmed();
                QString formattedValue = expandFunctionLikeMacro(define,args,macrosToBeIgnored);
                macrosUsed.insert(word);
                return formattedValue;
            }
        }
        mIndex = oldIndex;
        i=oldI;
    }
    return word;
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
    if (mStopForParserReset)
        return;
    PParsedFileInfo fileInfo = findFileInfo(fileName);
    if (fileInfo) {
        fileName = fileInfo->fileName();
    } else {
        fileName.squeeze();
        fileInfo = std::make_shared<ParsedFileInfo>(fileName);
    }
    if (mIncludeStack.size()>0) {
        bool alreadyIncluded = false;
        for (PParsedFile& parsedFile:mIncludeStack) {
            if (parsedFile->fileName == fileName) {
                //prevent recursive including;
                return;
            }
            if (parsedFile->fileInfo->including(fileName)) {
                alreadyIncluded = true;
            }
            parsedFile->fileInfo->addInclude(fileName);
            foreach (const QString& includedFileName,fileInfo->includes()) {
                parsedFile->fileInfo->addInclude(includedFileName);
            }
        }
        PParsedFile innerMostFile = mIncludeStack.back();
        innerMostFile->fileInfo->addDirectInclude(fileName);
        if (alreadyIncluded && !mFilesCouldRepeatInclude.contains(fileName))
            return;
        // Backup old position if we're entering a new file
        innerMostFile->index = mIndex;
        innerMostFile->branches = mBranchResults.count();
    }


    // Don't parse stuff that no need to parse again
    if (mScannedFiles.contains(fileName) && !mFilesCouldRepeatInclude.contains(fileName)) {
        //add defines of already parsed including headers;
        addDefinesInFile(fileName);
        return;
    }
//    if (mFilesCouldRepeatInclude.contains(fileName)) {
//        qDebug()<<"++ "<<fileName;
//    }
    // Create and add new buffer/position
    PParsedFile parsedFile = std::make_shared<ParsedFile>();
    parsedFile->index = 0;
    parsedFile->fileName = fileName;
    parsedFile->branches = 0;
    // parsedFile->buffer; it's auto initialized


    // Keep track of files we include here
    // Only create new items for files we have NOT scanned yet
    mCurrentFileInfo = fileInfo;
    mFileInfos.insert(fileName,mCurrentFileInfo);
    parsedFile->fileInfo = mCurrentFileInfo;
    //if not Assigned(Stream) then
    mScannedFiles.insert(fileName);

    if (mFileCache.contains(fileName))
        parsedFile->buffer = mFileCache.value(fileName);
    else {
        // Only load up the file if we are allowed to parse it
        bool isSystemFile = isSystemHeaderFile(fileName, mIncludePaths) || isSystemHeaderFile(fileName, mProjectIncludePaths);
        if ((mParseSystem && isSystemFile) || (mParseLocal && !isSystemFile)) {
            QStringList bufferedText;
            if (mOnGetFileStream && mOnGetFileStream(fileName,bufferedText)) {
                parsedFile->buffer  = bufferedText;
            } else {
                parsedFile->buffer = readFileToLines(fileName);
            }
            combineLinesEndingWithBackslash(parsedFile->buffer);
            replaceCommentsBySpaceChar(parsedFile->buffer);
        }
    }
    mIncludeStack.append(parsedFile);

    // Process it
    mIndex = parsedFile->index;
    mFileName = parsedFile->fileName;
    mBuffer = parsedFile->buffer;

//    for (int i=0;i<mBuffer.count();i++) {
//        mBuffer[i] = mBuffer[i].trimmed();
//    }

    // Update result file
    if (mIncludeStack.count()>1) {
        // include from within a file
        QString includeLine = "#include " + fileName + ":-1";
        mResult[mPreProcIndex] = includeLine;
        mIndex = -1; //mIndex would +1 in processBuffer();
    } else {
        QString includeLine = "#include " + fileName + ":-1";
        mResult.append(includeLine);
    }
    mFileJustOpenned = true;
    mFileIncludeOnceToken = "";
    //qDebug()<<"+ "<<fileName;
    if (!mFileOnlyIncludeOnce)
        mFilesCouldRepeatInclude.insert(fileName);
}


void CppPreprocessor::closeInclude()
{
    if (mIncludeStack.isEmpty())
        return;
    PParsedFile lastFile = mIncludeStack.back();
    if (mFilesCouldRepeatInclude.contains(lastFile->fileName))
        mFileCache.insert(lastFile->fileName, lastFile->buffer);
    mIncludeStack.pop_back();

    if (mIncludeStack.isEmpty())
        return;
    PParsedFile parsedFile = mIncludeStack.back();

    // Continue where we left off
    mIndex = parsedFile->index+1;
    mFileName = parsedFile->fileName;
    // Point to previous buffer and start past the include we walked into
    mBuffer = parsedFile->buffer;
    while (mBranchResults.count()>parsedFile->branches) {
        mBranchResults.pop_back();
    }


    // Start augmenting previous include list again
    mCurrentFileInfo = parsedFile->fileInfo;

    // Update result file (we've left the previous file)
    mResult.append(
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

CppPreprocessor::BranchResult CppPreprocessor::calcUnsupportedElseBranchResult(BranchResult oldResult)
{
    switch(oldResult) {
    case BranchResult::isTrue: return BranchResult::isFalse_but_trued;
    case BranchResult::isFalse: return BranchResult::isFalse;
    case BranchResult::isFalse_but_trued: return BranchResult::isFalse_but_trued;
    case BranchResult::parentIsFalse: return BranchResult::parentIsFalse;
    }
    Q_ASSERT( false ); //We should fail here.
    return BranchResult::isFalse;
}

void CppPreprocessor::addDefinesInFile(const QString &fileName)
{
    if (mProcessed.contains(fileName))
        return;
    mProcessed.insert(fileName);

    // then add the defines defined in it
    PDefineMap defineList = mFileDefines.value(fileName, PDefineMap());
    if (defineList) {
        foreach (const PDefine& define, defineList->values()) {
            mDefines.insert(define->name,define);
        }
    }

    PParsedFileInfo fileInfo = findFileInfo(fileName);
    if (fileInfo) {
        foreach (const QString& file, fileInfo->includes()) {
            addDefinesInFile(file);
        }
    }
}

void CppPreprocessor::parseArgs(PDefine define)
{
    QString args=define->args.mid(1,define->args.length()-2).trimmed(); // remove '(' ')'

    if(args=="")
        return ;
    QStringList argList = args.split(',');
    for (int i=0;i<argList.size();i++) {
        argList[i]=argList[i].trimmed();
        define->argUsed.append(false);
        define->argNotExpand.append(false);
    }
    QList<PDefineArgToken> tokens = tokenizeValue(define->value);

    QString formatStr = "";
    DefineArgTokenType lastTokenType=DefineArgTokenType::Other;
    int index;
    foreach (const PDefineArgToken& token, tokens) {
        switch(token->type) {
        case DefineArgTokenType::Identifier:
            if (token->value == "__VA_ARGS__") {
                index = argList.indexOf("...");
                define->varArgIndex = index;
            } else {
                index = argList.indexOf(token->value);
            }
            if (index>=0) {
                define->argUsed[index] = true;
                if (lastTokenType == DefineArgTokenType::Sharp) {
                    formatStr+= "\"%"+QString("%1").arg(index+1)+"\"";
                    define->argNotExpand[index] = true;
                    break;
                } else {
                    if (lastTokenType == DefineArgTokenType::DSharp) {
                        define->argNotExpand[index] = true;
                        if (index>0)
                            define->argNotExpand[index-1] = true;
                    }
                    formatStr+= "%"+QString("%1").arg(index+1);
                    break;
                }
            }
            formatStr += token->value;
            break;
        case DefineArgTokenType::DSharp:
        case DefineArgTokenType::Sharp:
            break;
        case DefineArgTokenType::Space:
        case DefineArgTokenType::Symbol:
            formatStr+=token->value;
            break;
        default:
            break;
        }
        lastTokenType = token->type;
    }
    define->formatValue = formatStr;
    define->formatValue.squeeze();
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
            if (token->type!=DefineArgTokenType::Other
                    && token->type!=DefineArgTokenType::Space) {
                tokens.append(token);
                token = std::make_shared<DefineArgToken>();
            }
            if ((i+1<value.length()) && (value[i+1]=='#')) {
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
                if (currentType == ContentType::RawStringPrefix ) {
                    delimiter += ch;
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
    return mSupportCPP23;
}

void CppPreprocessor::preprocessBuffer()
{
    while (mIncludeStack.count() > 0) {
        QString s;
        do {
            if (mStopForParserReset) {
                mBuffer.clear();
                return;
            }
            s = getNextPreprocessor();
            if (s.startsWith('#')) {
                QString command;
                command.reserve(s.length());
                QString tokens;
                tokens.reserve(s.length());
                auto it=s.constBegin();
                ++it; // skip  '#'
                while(it!=s.constEnd() && (*it==' ' || *it=='\t'))
                    ++it; // skip spaces;
                while(it!=s.constEnd()) {
                    if (*it>='a' && *it<='z' || *it=='_')
                        command.append(*it);
                    else
                        break;
                    ++it;
                }
                while(it!=s.constEnd() && (*it==' ' || *it=='\t'))
                    ++it; // skip spaces;
                while(it!=s.constEnd()) {
                    tokens.append(*it);
                    ++it; // skip spaces;
                }
                if (!command.isEmpty()) {
                    handlePreprocessor(command, tokens);
                }
            }
            // Step over
            mIndex++;
        } while (!s.isEmpty());
        closeInclude();
    }
//    qDebug()<<"Files could repeat include:";
//    foreach( const QString& fileName, mFilesCouldRepeatInclude) {
//        qDebug()<<fileName;
//    }
}

void CppPreprocessor::skipToPreprocessor()
{
    int bufferCount = mBuffer.count();
// Increment until a line begins with a #
    while ((mIndex < bufferCount) && !mBuffer[mIndex].startsWith('#')) {
        if (getCurrentBranch()==BranchResult::isTrue) { // if not skipping, expand current macros
            int startIndex = mIndex;
            QString expanded = expandMacros(mBuffer[mIndex],true);
            mResult.append(expanded);
            for (int i=startIndex;i<mIndex;i++) {
                mResult.append("");
            }
            //mResult.append(expandMacros(mBuffer[mIndex],1));
        } else // If skipping due to a failed branch, clear line
            mResult.append("");
        mIndex++;
    }
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
    QString newLine = expandMacrosInConditioningExpression(line); // replace FOO by numerical value of FOO
    bool result = evaluateExpression(newLine);
    //qDebug()<<newLine<<line<<result;
    return  result;
}

QString CppPreprocessor::expandMacrosInConditioningExpression(QString line) const
{
    QString newLine;
    int searchPos = 0;
    int lineLen = line.length();
    QMultiHash<int, QString> usedMacros;
    while (searchPos < line.length()) {
        QChar ch = line[searchPos];
        // We have found an identifier. It is not a number suffix. Try to expand it
        if (isDigit(ch)) {
            int head = searchPos;
            while (searchPos<lineLen && isNumberChar(line[searchPos])) {
                searchPos++;
            }
            newLine += QStringView(line.constData()+head, searchPos-head);
        } else if (isMacroIdentStartChar(line[searchPos])) {
            int head = searchPos;
            // Get identifier name (numbers are allowed, but not at the start
            while ((searchPos < line.length()) && isWordChar(line[searchPos]))
                searchPos++;
            QStringView name(line.constData()+head, searchPos-head);
            if (name == QLatin1String("defined")) {
                //expand define
                skipSpaces(line, searchPos);
                // Skip over its arguments
                if ((searchPos < lineLen) && (line[searchPos]=='(')) {
                    searchPos++; // skip '(';
                    //braced argument (next word)
                    skipSpaces(line, searchPos);
                    if (searchPos>=lineLen) //ill-formed
                        return "";
                    int defineStart = searchPos;
                    while ((searchPos < lineLen) && isWordChar(line[searchPos]))
                        searchPos++;
                    name = QStringView(line.constData()+defineStart, searchPos-defineStart);
                    skipSpaces(line, searchPos);
                    if (searchPos>=lineLen || line[searchPos]!=')')  //ill-formed
                        return "";
                    searchPos++; // skip ')'
                } else {
                    //none braced argument (next word)
                    skipSpaces(line, searchPos);
                    if (searchPos>=lineLen)
                        return "";
                    int defineStart = searchPos;
                    while ((searchPos < lineLen) && isWordChar(line[searchPos]))
                        searchPos++;
                    name = QStringView(line.constData()+defineStart, searchPos-defineStart);
                }

                PDefine define = getDefine(name.toString());
                QString insertValue;
                if (!define) {
                    insertValue = "0";
                } else {
                    insertValue = "1";
                }
                newLine += insertValue;
            } else if (name == QLatin1String("__has_include")) {
                //expand define
                skipSpaces(line, searchPos);
                // Skip over its arguments
                if ((searchPos < lineLen) && (line[searchPos]=='(')) {
                    searchPos ++;// skip '('
                    int argHead=searchPos;
                    if (!skipParenthesis(line, searchPos)) // ill-formed;
                        return "";
                    QString args = line.mid(argHead,searchPos-argHead).trimmed();
                    searchPos++; // skip ')';
                    Q_ASSERT(!mIncludeStack.isEmpty());
                    if (!args.startsWith('<') && !args.startsWith('\"'))
                        args = expandMacros(args);

                    QString fileName = getHeaderFilename(
                            mCurrentFileInfo->fileName(),
                            args,
                            mIncludePathList,
                            mProjectIncludePathList);
                    newLine += (mIncludeStack.front()->fileInfo->including(fileName))?"1":"0";
                } else {
                    // ill-formed
                    return "";
                }
            } else if (name == QLatin1String("and")) {
                newLine += "&&";
            } else if (name == QLatin1String("or")) {
                newLine += "||";
            }  else {
                 // We have found a regular define. Replace it by its value
                // Does it exist in the database?
                PDefine define = getDefine(name.toString());
                QString insertValue;
                if (!define) {
                    newLine += "0";
                } else {
                    QSet<QString> macrosToBeIgnored;
                    auto it = usedMacros.begin();
                    while (it!=usedMacros.end()) {
                        if (it.key()>=searchPos) {
                            macrosToBeIgnored.insert(it.value());
                        }
                        ++it;
                    }

                    if (macrosToBeIgnored.contains(define->name)) {
                        newLine += name;
                        continue;
                    }
                    skipSpaces(line, searchPos);
                    // It is a function. Expand arguments
                    if ((searchPos < lineLen) && (line[searchPos] == '(')) {
                        searchPos++; // skip '('
                        int argHead=searchPos;
                        if (skipParenthesis(line, searchPos)) {
                            QString args = line.mid(argHead,searchPos-argHead);
                            if (!macrosToBeIgnored.contains(define->name)) {
                                insertValue = expandFunctionLikeMacro(define,args, macrosToBeIgnored);
                            }
                            searchPos++; //skip ')'
                        } else {
                            line = "";// broken line
                            break;
                        }
                        // Replace regular define
                    } else {
                        if (!define->value.isEmpty())
                            insertValue = define->value;
                    }
                }
                bool isNumber=false;
                if (insertValue.length()==0)
                    isNumber=true;
                else if (isDigit(insertValue[0])){
                    isNumber=true;
                    for(int i=1;i<insertValue.length();i++)
                        if (!isNumberChar(insertValue[i])) {
                            isNumber = false;
                            break;
                        }
                }
                if (isNumber) {
                    newLine += insertValue;
                } else {
                    // Insert found value at place
                    //qDebug()<<"reparsed!"<<insertValue<<name;
                    line = insertValue + line.mid(searchPos);
                    lineLen = line.length();
                    QMultiHash tempMacros=usedMacros;
                    usedMacros.clear();
                    auto it = tempMacros.begin();
                    while (it!=tempMacros.end()) {
                        if (it.key()<=searchPos) {
                            usedMacros.insert(it.key()-searchPos,it.value());
                        }
                        ++it;
                    }
                    searchPos = 0;
                    usedMacros.insert(insertValue.length(), define->name);
                }
            }
        } else {
            searchPos ++ ;
            newLine += ch;
        }
    }
    return newLine;
}

bool CppPreprocessor::skipParenthesis(const QString &line, int &index, int step) const
{
    int level = 1;
    while ((index >= 0) && (index < line.length())) { // Find the corresponding opening brace
        if (line[index] == '(') {
            level++;
        } else if (line[index] == ')') {
            level--;
        }
        if (level == 0)
            return true;
        index+=step;
    }
    return false;
}

QString CppPreprocessor::expandFunctionLikeMacro(PDefine define, const QString &args, const QSet<QString> &macrosToBeIgnored) const
{
    // Replace function by this string
    QString result = define->formatValue;
//    if (args.startsWith('(') && args.endsWith(')')) {
//        qDebug()<<define->name<<args;
//        args = args.mid(1,args.length()-2);
//    }

    if (define->argUsed.length()==0) {
        result = define->formatValue;
    } else {
        QStringList argValues;
        int i=0;
        bool inString = false;
        bool inChar = false;
        int lastSplit=0;
        int level=0;
        while (i<args.length()) {
            switch(args[i].unicode()) {
            case '\\':
                if (inString || inChar)
                    i++;
            break;
            case '(':
            case '{':
                if (!inString && !inChar)
                    level++;
                break;
            case ')':
            case '}':
                if (!inString && !inChar)
                    level--;
                break;
            case '"':
                if (!inChar)
                    inString = !inString;
            break;
            case '\'':
                if (!inString)
                    inChar = !inChar;
                break;
            case ',':
                if (!inString && !inChar && level == 0) {
                    argValues.append(args.mid(lastSplit,i-lastSplit));
                    lastSplit=i+1;
                }
            break;
            }
            i++;
        }
        argValues.append(args.mid(lastSplit,i-lastSplit));
#ifdef QT_DEBUG
        if (
                (define->varArgIndex==-1 && argValues.length() != define->argUsed.length())
                || (define->varArgIndex!=-1 && argValues.length() < define->argUsed.length()-1)
                ) {
            qDebug()<<"*** Expand Macro error ***";
            qDebug()<<"Macro: "<<define->name<<define->args;
            qDebug()<<"Actual param: "<<args;
            qDebug()<<"Params splitted: "<<argValues;
            qDebug()<<"**********";
        }
#endif
        if (argValues.length() >= define->argUsed.length()
                && argValues.length()>0) {
            QStringList varArgs;
            for (int i=0;i<argValues.length();i++) {
                QString argValue = argValues[i];
                if (define->varArgIndex != -1
                     && i >= define->varArgIndex ) {
                    if (!define->argNotExpand[define->varArgIndex])
                        argValue = expandMacros(argValue,macrosToBeIgnored);
                    varArgs.append(argValue.trimmed());
                } else if (i<define->argUsed.length()
                            && define->argUsed[i]) {                    
                    if (!define->argNotExpand[i])
                        argValue = expandMacros(argValue,macrosToBeIgnored);
                    result=result.arg(argValue.trimmed());
                }
            }
            if (!varArgs.isEmpty() && define->varArgIndex != -1) {
                result=result.arg(varArgs.join(","));
            }
        }
    }
    result.replace("%%","%");

    return result;
}

bool CppPreprocessor::skipSpaces(const QString &expr, int &pos) const
{
    while (pos<expr.length() && isSpaceChar(expr[pos]))
        pos++;
    return pos<expr.length();
}

bool CppPreprocessor::evalNumber(const QString &expr, int &result, int &pos) const
{
    if (!skipSpaces(expr,pos))
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
        s.resize(s.length()-1,1);
        result = s.toUInt(&ok, 0);
    } else {
        result = s.toInt(&ok, 0);
    }
    return ok;
}

bool CppPreprocessor::evalTerm(const QString &expr, int &result, int &pos) const
{
    if (!skipSpaces(expr,pos))
        return false;
    if (expr[pos]=='(') {
        pos++;
        if (!evalExpr(expr,result,pos))
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
bool CppPreprocessor::evalUnaryExpr(const QString &expr, int &result, int &pos) const
{
    if (!skipSpaces(expr,pos))
        return false;
    if (expr[pos]=='+') {
        pos++;
        if (!evalTerm(expr,result,pos))
            return false;
    } else if (expr[pos]=='-') {
        pos++;
        if (!evalTerm(expr,result,pos))
            return false;
        result = -result;
    } else if (expr[pos]=='~') {
        pos++;
        if (!evalTerm(expr,result,pos))
            return false;
        result = ~result;
    } else if (expr[pos]=='!') {
        pos++;
        if (!evalTerm(expr,result,pos))
            return false;
        result = !result;
    } else {
        return evalTerm(expr,result,pos);
    }
    return true;
}

/*
 * mul_expr = unary_expr
     | mul_expr '*' unary_expr
     | mul_expr '/' unary_expr
     | mul_expr '%' unary_expr
 */
bool CppPreprocessor::evalMulExpr(const QString &expr, int &result, int &pos) const
{
    if (!evalUnaryExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        int rightResult;
        if (expr[pos]=='*') {
            pos++;
            if (!evalUnaryExpr(expr,rightResult,pos))
                return false;
            result *= rightResult;
        } else if (expr[pos]=='/') {
            pos++;
            if (!evalUnaryExpr(expr,rightResult,pos))
                return false;
            if (rightResult != 0)
                result /= rightResult;
            else
                result = 0;
        } else if (expr[pos]=='%') {
            pos++;
            if (!evalUnaryExpr(expr,rightResult,pos))
                return false;
            if (rightResult != 0)
                result %= rightResult;
            else
                result = 0;
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
bool CppPreprocessor::evalAddExpr(const QString &expr, int &result, int &pos) const
{
    if (!evalMulExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        int rightResult;
        if (expr[pos]=='+') {
            pos++;
            if (!evalMulExpr(expr,rightResult,pos))
                return false;
            result += rightResult;
        } else if (expr[pos]=='-') {
            pos++;
            if (!evalMulExpr(expr,rightResult,pos))
                return false;
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
bool CppPreprocessor::evalShiftExpr(const QString &expr, int &result, int &pos) const
{
    if (!evalAddExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        int rightResult;
        if (pos+1<expr.length() && expr[pos] == '<' && expr[pos+1]=='<') {
            pos += 2;
            if (!evalAddExpr(expr,rightResult,pos))
                return false;
            result = (result << rightResult);
        } else if (pos+1<expr.length() && expr[pos] == '>' && expr[pos+1]=='>') {
            pos += 2;
            if (!evalAddExpr(expr,rightResult,pos))
                return false;
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
bool CppPreprocessor::evalRelationExpr(const QString &expr, int &result, int &pos) const
{
    if (!evalShiftExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        int rightResult;
        if (expr[pos]=='<') {
            if (pos+1<expr.length() && expr[pos+1]=='=') {
                pos+=2;
                if (!evalShiftExpr(expr,rightResult,pos))
                    return false;
                result = (result <= rightResult);
            } else {
                pos++;
                if (!evalShiftExpr(expr,rightResult,pos))
                    return false;
                result = (result < rightResult);
            }
        } else if (expr[pos]=='>') {
            if (pos+1<expr.length() && expr[pos+1]=='=') {
                pos+=2;
                if (!evalShiftExpr(expr,rightResult,pos))
                    return false;
                result = (result >= rightResult);
            } else {
                pos++;
                if (!evalShiftExpr(expr,rightResult,pos))
                    return false;
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
bool CppPreprocessor::evalEqualExpr(const QString &expr, int &result, int &pos) const
{
    if (!evalRelationExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (pos+1<expr.length() && expr[pos]=='!' && expr[pos+1]=='=') {
            pos+=2;
            int rightResult;
            if (!evalRelationExpr(expr,rightResult,pos))
                return false;
            result = (result != rightResult);
        } else if (pos+1<expr.length() && expr[pos]=='=' && expr[pos+1]=='=') {
            pos+=2;
            int rightResult;
            if (!evalRelationExpr(expr,rightResult,pos))
                return false;
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
bool CppPreprocessor::evalBitAndExpr(const QString &expr, int &result, int &pos) const
{
    if (!evalEqualExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (expr[pos]=='&'
                && (pos+1 == expr.length()
                || expr[pos+1]!='&')) {
            pos++;
            int rightResult;
            if (!evalEqualExpr(expr,rightResult,pos))
                return false;
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
bool CppPreprocessor::evalBitXorExpr(const QString &expr, int &result, int &pos) const
{
    if (!evalBitAndExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (expr[pos]=='^') {
            pos++;
            int rightResult;
            if (!evalBitAndExpr(expr,rightResult,pos))
                return false;
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
bool CppPreprocessor::evalBitOrExpr(const QString &expr, int &result, int &pos) const
{
    if (!evalBitXorExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (expr[pos] == '|'
                && (pos+1 == expr.length()
                || expr[pos+1]!='|')) {
            pos++;
            int rightResult;
            if (!evalBitXorExpr(expr,rightResult,pos))
                return false;
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
bool CppPreprocessor::evalLogicAndExpr(const QString &expr, int &result, int &pos) const
{
    if (!evalBitOrExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (pos+1<expr.length() && expr[pos]=='&' && expr[pos+1] =='&') {
            if (!result) { // short-circuiting
                skipParenthesis(expr,pos);
                return true;
            }
            pos+=2;
            int rightResult;
            if (!evalBitOrExpr(expr,rightResult,pos))
                return false;
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
bool CppPreprocessor::evalLogicOrExpr(const QString &expr, int &result, int &pos) const
{
    if (!evalLogicAndExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (pos+1<expr.length() && expr[pos]=='|' && expr[pos+1] =='|') {
            if (result) { // short-circuiting
                skipParenthesis(expr,pos);
                return true;
            }
            pos+=2;
            int rightResult;
            if (!evalLogicAndExpr(expr,rightResult,pos))
                return false;
            result = result || rightResult;
        } else {
            break;
        }
    }
    return true;
}

bool CppPreprocessor::evalConnditionalExpr(const QString &expr, int &result, int &pos) const
{
    if (!evalLogicOrExpr(expr,result,pos))
        return false;
    if (!skipSpaces(expr,pos))
        return true;
    if (expr[pos] == '?') {
        pos++; // skip '?'
        int condition = result;
        int result1,result2;
        if (!evalExpr(expr,result1,pos))
            return false;
        if (!skipSpaces(expr,pos))
            return false;
        if (expr[pos] != ':')
            return false;
        pos++; // skip ':'
        if (!evalExpr(expr,result2,pos))
            return false;
        result =(condition)?result1:result2;
    }
    return true;
}

bool CppPreprocessor::evalExpr(const QString &expr, int &result, int &pos) const
{
    return evalConnditionalExpr(expr,result,pos);
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
    int result;
    bool ok = evalExpr(line,result,pos);
    if (!ok)
        return false;
    //expr not finished
    if (skipSpaces(line,pos))
        return false;
    return result;
}

bool CppPreprocessor::fileOnlyIncludeOnce() const
{
    return mFileOnlyIncludeOnce;
}

void CppPreprocessor::setFileOnlyIncludeOnce(bool newFileOnlyIncludeOnce)
{
    mFileOnlyIncludeOnce = newFileOnlyIncludeOnce;
}


