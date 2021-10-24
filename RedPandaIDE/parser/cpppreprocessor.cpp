#include "cpppreprocessor.h"
#include "../utils.h"

#include <QFile>
#include <QTextCodec>
#include <QDebug>
#include <QMessageBox>

CppPreprocessor::CppPreprocessor()
{
}

void CppPreprocessor::clear()
{
    mIncludes.clear();
    mDefines.clear();
    mHardDefines.clear();
    mProcessed.clear();
    mFileDefines.clear();
    mBranchResults.clear();
    mResult.clear();
    mCurrentIncludes.reset();
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
    if (!args.isEmpty())
        parseArgs(define);
    if (hardCoded)
        mHardDefines.insert(name,define);
    else {
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
    value = removeGCCAttributes(s.mid(i+1).trimmed());
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

PDefine CppPreprocessor::getDefine(const QString &name)
{
    return mDefines.value(name,PDefine());
}

PDefine CppPreprocessor::getHardDefine(const QString &name)
{
    return mHardDefines.value(name,PDefine());
}

void CppPreprocessor::reset()
{
    mResult.clear();

    // Clear extracted data
    mIncludes.clear();
    mBranchResults.clear();
    mCurrentIncludes.reset();
    mProcessed.clear();
    resetDefines(); // do not throw away hardcoded
}

void CppPreprocessor::resetDefines()
{
    mDefines.clear();

    mDefines.insert(mHardDefines);
}

void CppPreprocessor::setScanOptions(bool parseSystem, bool parseLocal)
{
    mParseSystem = parseSystem;
    mParseLocal=parseLocal;
}

void CppPreprocessor::preprocess(const QString &fileName, QStringList buffer)
{
    mFileName = fileName;
    reset();
    openInclude(fileName, buffer);
    //    StringsToFile(mBuffer,"f:\\buffer.txt");
    preprocessBuffer();
    //    StringsToFile(mBuffer,"f:\\buffer.txt");
    //    StringsToFile(mResult,"f:\\log.txt");
}

void CppPreprocessor::invalidDefinesInFile(const QString &fileName)
{
    PDefineMap defineMap = mFileDefines.value(fileName,PDefineMap());
    if (defineMap) {
        foreach (const PDefine& define, *defineMap) {
            const PDefine& p = mDefines.value(define->name);
            if (p == define) {
                mDefines.remove(define->name);
            }
        }
        mFileDefines.remove(fileName);
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
                    .arg(define->hardCoded).arg(define->formatValue)
                 <<Qt::endl;
        }
    }
}

void CppPreprocessor::dumpIncludesListTo(const QString &fileName) const
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        QTextStream stream(&file);
        for (const PFileIncludes& fileIncludes:mIncludesList) {
            stream<<fileIncludes->baseFile<<" : "<<Qt::endl;
            stream<<"\t**includes:**"<<Qt::endl;
            foreach (const QString& s,fileIncludes->includeFiles.keys()) {
                stream<<"\t--"+s<<Qt::endl;
            }
            stream<<"\t**depends on:**"<<Qt::endl;
            foreach (const QString& s,fileIncludes->dependingFiles) {
                stream<<"\t^^"+s<<Qt::endl;
            }
            stream<<"\t**depended by:**"<<Qt::endl;
            foreach (const QString& s,fileIncludes->dependedFiles) {
                stream<<"\t&&"+s<<Qt::endl;
            }
            stream<<"\t**using:**"<<Qt::endl;
            foreach (const QString& s,fileIncludes->usings) {
                stream<<"\t++"+s<<Qt::endl;
            }
            stream<<"\t**statements:**"<<Qt::endl;
            foreach (const PStatement& statement,fileIncludes->statements) {
                if (statement) {
                    stream<<QString("\t**%1 , %2")
                            .arg(statement->command,statement->fullName)<<Qt::endl;
                }
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

void CppPreprocessor::clearIncludePaths()
{
    mIncludePaths.clear();
    mIncludePathList.clear();
}

void CppPreprocessor::clearProjectIncludePaths()
{
    mProjectIncludePaths.clear();
    mProjectIncludePathList.clear();
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

    // Assemble whole line, convert newlines to space
    QString result;
    for (int i=preProcFrom;i<=preProcTo;i++) {
        if (mBuffer[i].endsWith('\\')) {
            result+=mBuffer[i].mid(0,mBuffer[i].size()-1)+' ';
        } else {
            result+=mBuffer[i]+' ';
        }
        mResult.append("");// defines resolve into empty files, except #define and #include
    }
    // Step over
    mIndex++;
    return result;
}

void CppPreprocessor::simplify(QString &output)
{
    // Remove #
    output = output.mid(1).trimmed();
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
            setCurrentBranch( getDefine(name)!=nullptr );

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
            setCurrentBranch( getDefine(name)==nullptr );
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

            bool testResult = evaluateIf(ifLine);
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
            bool testResult = evaluateIf(ifLine);
            setCurrentBranch(testResult);
        }
    } else if (line.startsWith("endif")) {
        removeCurrentBranch();
    }
}

void CppPreprocessor::handleDefine(const QString &line)
{
    if (getCurrentBranch()) {
        addDefineByLine(line, false);
        mResult[mPreProcIndex] = '#' + line; // add define to result file so the parser can handle it
    }
}

void CppPreprocessor::handleInclude(const QString &line, bool fromNext)
{
    if (!getCurrentBranch()) // we're skipping due to a branch failure
        return;

    PParsedFile file = mIncludes.back();
    QString fileName;
    // Get full header file name
    QString currentDir = includeTrailingPathDelimiter(extractFileDir(file->fileName));
    QStringList includes;
    QStringList projectIncludes;
    bool found;
    if (fromNext && mIncludePaths.contains(currentDir))
        found = false;
    else
        found = true;
    foreach(const QString& s, mIncludePathList) {
        if (found) {
            includes.append(s);
        }
        if (s == currentDir)
            found = true;
    }
    if (fromNext && mProjectIncludePaths.contains(currentDir))
        found = false;
    else
        found = true;
    foreach(const QString& s, mProjectIncludePathList) {
        if (found)
            projectIncludes.append(s);
        if (s == currentDir)
            found = true;
    }
    fileName = getHeaderFilename(
                file->fileName,
                line,
                includes,
                projectIncludes);

    if (fileName.isEmpty())
        return;

    //mCurrentIncludes->includeFiles.insert(fileName,true);
    // And open a new entry
    openInclude(fileName);
    mCurrentIncludes->includeFiles.insert(fileName,true);
}

void CppPreprocessor::handlePreprocessor(const QString &value)
{
    if (value.startsWith("define"))
        handleDefine(value);
    else if (value.startsWith("undef"))
        handleUndefine(value);
    else if (value.startsWith("if")
             || value.startsWith("else") || value.startsWith("elif")
             || value.startsWith("endif"))
        handleBranch(value);
    else if (value.startsWith("include_next"))
        handleInclude(value,true);
    else if (value.startsWith("include"))
        handleInclude(value);
}

void CppPreprocessor::handleUndefine(const QString &line)
{
    // Remove undef
    constexpr int UNDEF_LEN = 5;
    QString name = line.mid(UNDEF_LEN).trimmed();

//    //may be defined many times
//    while (true) {
    PDefine define = getDefine(name);
    if (define) {
        //remove the define from defines set
        mDefines.remove(name);
        //remove the define form the file where it defines
        if (define->filename == mFileName) {
            PDefineMap defineMap = mFileDefines.value(mFileName);
            if (defineMap) {
                defineMap->remove(name);
            }
        }
    }
}

QString CppPreprocessor::expandMacros(const QString &line, int depth)
{
    //prevent infinit recursion
    if (depth > MAX_DEFINE_EXPAND_DEPTH)
        return line;
    QString word;
    QString newLine;
    int lenLine = line.length();
    int i=0;
    while (i< lenLine) {
        QChar ch=line[i];
        if (isWordChar(ch)) {
            word += ch;
        } else {
            if (!word.isEmpty()) {
                expandMacro(line,newLine,word,i,depth);
            }
            word = "";
            if (i< lenLine) {
                newLine += line[i];
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
    if (word.startsWith("__")
            && word.endsWith("__")) {
//    if (word == "__attribute__") {
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
        PDefine define = getDefine(word);
        if (define && define->args=="" ) {
            //newLine:=newLine+RemoveGCCAttributes(define^.Value);
            if (define->value != word )
              newLine += expandMacros(define->value,depth+1);
            else
              newLine += word;

        } else if (define && (define->args!="")) {
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
                    QString args = line.mid(argStart,argEnd-argStart+1).trimmed();
                    QString formattedValue = expandFunction(define,args);
                    newLine += expandMacros(formattedValue,depth+1);
                }
            }
        } else {
            newLine += word;
        }
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

PParsedFile CppPreprocessor::getInclude(int index)
{
    return mIncludes[index];
}

void CppPreprocessor::openInclude(const QString &fileName, QStringList bufferedText)
{
    if (mIncludes.size()>0) {
        PParsedFile topFile = mIncludes.front();
        if (topFile->fileIncludes->includeFiles.contains(fileName)) {
            return; //already included
        }
        for (PParsedFile& parsedFile:mIncludes) {
            parsedFile->fileIncludes->includeFiles.insert(fileName,false);
        }
    }
    if (mIncludes.size()>0) {
        // Backup old position if we're entering a new file
        PParsedFile innerMostFile = mIncludes.back();
        innerMostFile->index = mIndex;
        innerMostFile->branches = mBranchResults.count();

        innerMostFile->fileIncludes->includeFiles.insert(fileName,true);
    }

//    // Add the new file to the includes of the current file
//    // Only add items to the include list of the given file if the file hasn't been scanned yet
//    // The above is fixed by checking for duplicates.
//    // The proper way would be to do backtracking of files we have FINISHED scanned.
//    // These are not the same files as the ones in fScannedFiles. We have STARTED scanning these.
//    {
//    if Assigned(fCurrentIncludes) then
//      with fCurrentIncludes^ do
//        if not ContainsText(IncludeFiles, FileName) then
//          IncludeFiles := IncludeFiles + AnsiQuotedStr(FileName, '"') + ',';
//    }

    // Create and add new buffer/position
    PParsedFile parsedFile = std::make_shared<ParsedFile>();
    parsedFile->index = 0;
    parsedFile->fileName = fileName;
    parsedFile->branches = 0;
    // parsedFile->buffer; it's auto initialized


    // Keep track of files we include here
    // Only create new items for files we have NOT scanned yet
    mCurrentIncludes = getFileIncludesEntry(fileName);
    if (!mCurrentIncludes) {
        // do NOT create a new item for a file that's already in the list
        mCurrentIncludes = std::make_shared<FileIncludes>();
        mCurrentIncludes->baseFile = fileName;
        //mCurrentIncludes->includeFiles;
        //mCurrentIncludes->usings;
        //mCurrentIncludes->statements;
        //mCurrentIncludes->declaredStatements;
        //mCurrentIncludes->scopes;
        //mCurrentIncludes->dependedFiles;
        //mCurrentIncludes->dependingFiles;
        mIncludesList.insert(fileName,mCurrentIncludes);
    }

    parsedFile->fileIncludes = mCurrentIncludes;

    // Don't parse stuff we have already parsed
    if ((!bufferedText.isEmpty()) || !mScannedFiles.contains(fileName)) {
        // Parse ONCE
        //if not Assigned(Stream) then
        mScannedFiles.insert(fileName);

        // Only load up the file if we are allowed to parse it
        bool isSystemFile = isSystemHeaderFile(fileName, mIncludePaths);
        if ((mParseSystem && isSystemFile) || (mParseLocal && !isSystemFile)) {
            if (!bufferedText.isEmpty()) {
                parsedFile->buffer  = bufferedText;
            } else {
                parsedFile->buffer = ReadFileToLines(fileName);
            }
        }
    } else {
        //add defines of already parsed including headers;
        addDefinesInFile(fileName);
        PFileIncludes fileIncludes = getFileIncludesEntry(fileName);
        for (PParsedFile& file:mIncludes) {
            file->fileIncludes->includeFiles.insert(fileIncludes->includeFiles);
        }
    }
    mIncludes.append(parsedFile);

    // Process it
    mIndex = parsedFile->index;
    mFileName = parsedFile->fileName;
    parsedFile->buffer = removeComments(parsedFile->buffer);
    mBuffer = parsedFile->buffer;

//    for (int i=0;i<mBuffer.count();i++) {
//        mBuffer[i] = mBuffer[i].trimmed();
//    }

    // Update result file
    QString includeLine = "#include " + fileName + ":1";
    if (mIncludes.count()>1) { // include from within a file
      mResult[mPreProcIndex] = includeLine;
    } else {
      mResult.append(includeLine);
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

bool CppPreprocessor::getCurrentBranch()
{
    if (!mBranchResults.isEmpty())
        return mBranchResults.last();
    else
        return true;
}

void CppPreprocessor::setCurrentBranch(bool value)
{
    mBranchResults.append(value);
}

void CppPreprocessor::removeCurrentBranch()
{
    if (mBranchResults.size()>0)
        mBranchResults.pop_back();
}

QStringList CppPreprocessor::result() const
{
    return mResult;
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

    //May be redefined, so order is important
    //first add the defines in the files it included
    PFileIncludes fileIncludes = getFileIncludesEntry(fileName);
    if (fileIncludes) {
        foreach (const QString& s,fileIncludes->includeFiles.keys()) {
            addDefinesInFile(s);
        }
    }

    // then add the defines defined in it
    PDefineMap defineList = mFileDefines.value(fileName, PDefineMap());
    if (defineList) {
        foreach (const PDefine& define, defineList->values()) {
            mDefines.insert(define->name,define);
        }
    }
}

void CppPreprocessor::parseArgs(PDefine define)
{
    QString args=define->args.mid(1,define->args.length()-2).trimmed(); // remove '(' ')'

    if(args=="")
        return ;
    define->argList = args.split(',');
    for (int i=0;i<define->argList.size();i++) {
        define->argList[i]=define->argList[i].trimmed();
        define->argUsed.append(false);
    }
    QList<PDefineArgToken> tokens = tokenizeValue(define->value);

    QString formatStr = "";
    DefineArgTokenType lastTokenType=DefineArgTokenType::Other;
    int index;
    foreach (const PDefineArgToken& token, tokens) {
        switch(token->type) {
        case DefineArgTokenType::Identifier:
            index = define->argList.indexOf(token->value);
            if (index>=0) {
                define->argUsed[index] = true;
                if (lastTokenType == DefineArgTokenType::Sharp) {
                    formatStr+= "\"%"+QString("%1").arg(index+1)+"\"";
                    break;
                } else {
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

QStringList CppPreprocessor::removeComments(const QStringList &text)
{
    QStringList result;
    ContentType currentType = ContentType::Other;
    QString delimiter;

    for (const QString& line:text) {
        QString s;
        int pos = 0;
        bool stopProcess=false;
        while (pos<line.length()) {
            QChar ch =line[pos];
            if (currentType == ContentType::AnsiCComment) {
                if (ch=='*' && (pos+1<line.length()) && line[pos+1]=='/') {
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
                    if (line.midRef(0,pos).endsWith(')'+delimiter))
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
                if (currentType == ContentType::Other && pos+1<line.length() && line[pos+1]=='"') {
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
                    if (pos+1<line.length() && line[pos+1]=='/') {
                        // line comment , skip all remainings of the current line
                        stopProcess = true;
                        break;
                    } else if (pos+1<line.length() && line[pos+1]=='*') {
                        /* ansi c comment */
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
                    if (pos<line.length()) {
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
            if (stopProcess)
                break;
            pos++;
        }
        result.append(s.trimmed());
    }
    return result;
}

void CppPreprocessor::preprocessBuffer()
{
    while (mIncludes.count() > 0) {
        QString s;
        do {
            s = getNextPreprocessor();
            if (s.startsWith('#')) {
                simplify(s);
                if (!s.isEmpty()) {
                    handlePreprocessor(s);
                }
            }
        } while (!s.isEmpty());
        closeInclude();
    }
}

void CppPreprocessor::skipToEndOfPreprocessor()
{
    // Skip until last char of line is NOT \ anymore
    while ((mIndex < mBuffer.count()) && mBuffer[mIndex].endsWith('\\'))
        mIndex++;
}

void CppPreprocessor::skipToPreprocessor()
{

// Increment until a line begins with a #
    while ((mIndex < mBuffer.count()) && !mBuffer[mIndex].startsWith('#')) {
        if (getCurrentBranch()) // if not skipping, expand current macros
            mResult.append(expandMacros(mBuffer[mIndex],1));
        else // If skipping due to a failed branch, clear line
            mResult.append("");
        mIndex++;
    }
}

bool CppPreprocessor::isWordChar(const QChar &ch)
{
    if (ch=='_'
            // || (ch>='a' && ch<='z') || (ch>='A' && ch<='Z')
            || ch.isLetter()
            || (ch>='0' && ch<='9')) {
        return true;
    }
    return false;
}

bool CppPreprocessor::isIdentChar(const QChar &ch)
{
    if (ch=='_' || ch == '*' || ch == '&' || ch == '~' ||
            ch.isLetter()
            //(ch>='a' && ch<='z') || (ch>='A' && ch<='Z')
            || (ch>='0' && ch<='9')) {
        return true;
    }
    return false;
}

bool CppPreprocessor::isLineChar(const QChar &ch)
{
    return ch=='\r' || ch == '\n';
}

bool CppPreprocessor::isSpaceChar(const QChar &ch)
{
    return ch == ' ' || ch == '\t';
}

bool CppPreprocessor::isOperatorChar(const QChar &ch)
{

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

bool CppPreprocessor::isMacroIdentChar(const QChar &ch)
{
    //return (ch>='A' && ch<='Z') || (ch>='a' && ch<='z')
    return ch.isLetter()
            || ch == '_';
}

bool CppPreprocessor::isDigit(const QChar &ch)
{
    return (ch>='0' && ch<='9');
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

QString CppPreprocessor::lineBreak()
{
    return "\n";
}

bool CppPreprocessor::evaluateIf(const QString &line)
{
    QString newLine = expandDefines(line); // replace FOO by numerical value of FOO
    return  evaluateExpression(newLine);
}

QString CppPreprocessor::expandDefines(QString line)
{
    int searchPos = 0;
    while (searchPos < line.length()) {
        // We have found an identifier. It is not a number suffix. Try to expand it
        if (isMacroIdentChar(line[searchPos]) && (
                    (searchPos == 0) || !isDigit(line[searchPos - 1]))) {
            int head = searchPos;
            int tail = searchPos;

            // Get identifier name (numbers are allowed, but not at the start
            while ((tail < line.length()) && (isMacroIdentChar(line[tail]) || isDigit(line[head])))
                tail++;
//            qDebug()<<"1 "<<head<<tail<<line;
            QString name = line.mid(head,tail-head);
            int nameStart = head;
            int nameEnd = tail;

            if (name == "defined") {
                //expand define
                //tail = searchPos + name.length();
                while ((tail < line.length()) && isSpaceChar(line[tail]))
                    tail++; // skip spaces
                int defineStart;

                // Skip over its arguments
                if ((tail < line.length()) && (line[tail]=='(')) {
                    //braced argument (next word)
                    defineStart = tail+1;
                    if (!skipBraces(line, tail)) {
                        line = ""; // broken line
                        break;
                    }
                } else {
                    //none braced argument (next word)
                    defineStart = tail;
                    if ((tail>=line.length()) || !isMacroIdentChar(line[searchPos])) {
                        line = ""; // broken line
                        break;
                    }
                    while ((tail < line.length()) && (isMacroIdentChar(line[tail]) || isDigit(line[tail])))
                        tail++;
                }
//                qDebug()<<"2 "<<defineStart<<tail<<line;
                name = line.mid(defineStart, tail - defineStart);
                PDefine define = getDefine(name);
                QString insertValue;
                if (!define) {
                    insertValue = "0";
                } else {
                    insertValue = "1";
                }
                // Insert found value at place
                line.remove(searchPos, tail-searchPos+1);
                line.insert(searchPos,insertValue);
            } else if ((name == "and") || (name == "or")) {
                searchPos = tail; // Skip logical operators
            }  else {
                 // We have found a regular define. Replace it by its value
                // Does it exist in the database?
                PDefine define = getDefine(name);
                QString insertValue;
                if (!define) {
                    insertValue = "0";
                } else {
                    while ((tail < line.length()) && isSpaceChar(line[tail]))
                        tail++;// skip spaces
                    // It is a function. Expand arguments
                    if ((tail < line.length()) && (line[tail] == '(')) {
                        head=tail;
                        if (skipBraces(line, tail)) {
//                            qDebug()<<"3 "<<line<<head<<tail;
                            QString args = line.mid(head,tail-head+1);
                            insertValue = expandFunction(define,args);
                            nameEnd = tail+1;
                        } else {
                            line = "";// broken line
                            break;
                        }
                        // Replace regular define
                    } else {
                        if (!define->value.isEmpty())
                            insertValue = define->value;
                        else
                            insertValue = "0";
                    }
                }
                // Insert found value at place
                line.remove(nameStart, nameEnd - nameStart);
                line.insert(searchPos,insertValue);
            }
        } else {
            searchPos ++ ;
        }
    }
    return line;
}

bool CppPreprocessor::skipBraces(const QString &line, int &index, int step)
{
    int level = 0;
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

QString CppPreprocessor::expandFunction(PDefine define, QString args)
{
    // Replace function by this string
    QString result = define->formatValue;
    if (args.startsWith('(') && args.endsWith(')')) {
        args = args.mid(1,args.length()-2);
    }

    QStringList argValues = args.split(",");
    if (argValues.length() == define->argList.length()
            && argValues.length()>0) {
        for (int i=0;i<argValues.length();i++) {
            if (define->argUsed[i]) {
                QString argValue = argValues[i];
                result=result.arg(argValue.trimmed());
            }
        }
    }
    result.replace("%%","%");

    return result;
}

bool CppPreprocessor::skipSpaces(const QString &expr, int &pos)
{
    while (pos<expr.length() && isSpaceChar(expr[pos]))
        pos++;
    return pos<expr.length();
}

bool CppPreprocessor::evalNumber(const QString &expr, int &result, int &pos)
{
    if (!skipSpaces(expr,pos))
        return false;
    QString s;
    while (pos<expr.length() && isNumberChar(expr[pos])) {
        s+=expr[pos];
        pos++;
    }
    bool ok;

    if (s.endsWith("LL",Qt::CaseInsensitive)) {
        s.remove(s.length()-2,2);
        result = s.toLongLong(&ok);
    } else if (s.endsWith("L",Qt::CaseInsensitive)) {
        s.remove(s.length()-1,1);
        result = s.toLong(&ok);
    } else if (s.endsWith("ULL",Qt::CaseInsensitive)) {
        s.remove(s.length()-3,3);
        result = s.toULongLong(&ok);
    } else if (s.endsWith("UL",Qt::CaseInsensitive)) {
        s.remove(s.length()-2,2);
        result = s.toULong(&ok);
    } else if (s.endsWith("U",Qt::CaseInsensitive)) {
        s.remove(s.length()-1,1);
        result = s.toUInt(&ok);
    } else {
        result = s.toInt(&ok);
    }
    return ok;
}

bool CppPreprocessor::evalTerm(const QString &expr, int &result, int &pos)
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
bool CppPreprocessor::evalUnaryExpr(const QString &expr, int &result, int &pos)
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
bool CppPreprocessor::evalMulExpr(const QString &expr, int &result, int &pos)
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
            result /= rightResult;
        } else if (expr[pos]=='%') {
            pos++;
            if (!evalUnaryExpr(expr,rightResult,pos))
                return false;
            result %= rightResult;
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
bool CppPreprocessor::evalAddExpr(const QString &expr, int &result, int &pos)
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
bool CppPreprocessor::evalShiftExpr(const QString &expr, int &result, int &pos)
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
bool CppPreprocessor::evalRelationExpr(const QString &expr, int &result, int &pos)
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
bool CppPreprocessor::evalEqualExpr(const QString &expr, int &result, int &pos)
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
bool CppPreprocessor::evalBitAndExpr(const QString &expr, int &result, int &pos)
{
    if (!evalEqualExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (expr[pos]=='&'
                && (pos == expr.length()
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
bool CppPreprocessor::evalBitXorExpr(const QString &expr, int &result, int &pos)
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
bool CppPreprocessor::evalBitOrExpr(const QString &expr, int &result, int &pos)
{
    if (!evalBitXorExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (expr[pos] == '|'
                && (pos == expr.length()
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
bool CppPreprocessor::evalLogicAndExpr(const QString &expr, int &result, int &pos)
{
    if (!evalBitOrExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (pos+1<expr.length() && expr[pos]=='&' && expr[pos+1] =='&') {
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
bool CppPreprocessor::evalLogicOrExpr(const QString &expr, int &result, int &pos)
{
    if (!evalLogicAndExpr(expr,result,pos))
        return false;
    while (true) {
        if (!skipSpaces(expr,pos))
            break;
        if (pos+1<expr.length() && expr[pos]=='|' && expr[pos+1] =='|') {
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

bool CppPreprocessor::evalExpr(const QString &expr, int &result, int &pos)
{
    return evalLogicOrExpr(expr,result,pos);
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
    */

int CppPreprocessor::evaluateExpression(QString line)
{
    int pos = 0;
    int result;
    bool ok = evalExpr(line,result,pos);
    if (!ok)
        return -1;
    //expr not finished
    if (skipSpaces(line,pos))
        return -1;
    return result;
}

const QList<QString> &CppPreprocessor::projectIncludePathList() const
{
    return mProjectIncludePathList;
}

const QList<QString> &CppPreprocessor::includePathList() const
{
    return mIncludePathList;
}

const DefineMap &CppPreprocessor::hardDefines() const
{
    return mHardDefines;
}

const QSet<QString> &CppPreprocessor::projectIncludePaths()
{
    return mProjectIncludePaths;
}

const QSet<QString> &CppPreprocessor::includePaths()
{
    return mIncludePaths;
}

QSet<QString> &CppPreprocessor::scannedFiles()
{
    return mScannedFiles;
}

QHash<QString, PFileIncludes> &CppPreprocessor::includesList()
{
    return mIncludesList;
}

