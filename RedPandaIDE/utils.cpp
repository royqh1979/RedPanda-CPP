#include "utils.h"
#include "systemconsts.h"
#include <QDate>
#include <QDateTime>
#include "editor.h"
#include "editorlist.h"
#include "settings.h"
#include "mainwindow.h"
#include "project.h"
#include "parser/cppparser.h"
#ifdef Q_OS_WIN
#include <QMimeDatabase>
#include <windows.h>
#endif

QStringList splitProcessCommand(const QString &cmd)
{
    QStringList result;
    SplitProcessCommandQuoteType quoteType = SplitProcessCommandQuoteType::None;
    int i=0;
    QString current;
    while (i<cmd.length()) {
        switch (cmd[i].unicode()) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            if (quoteType == SplitProcessCommandQuoteType::None) {
                if (!current.isEmpty()) {
                    result.append(current);
                }
                current = "";
            } else {
                current += cmd[i];
            }
            i++;
            break;
        case '\"':
            switch(quoteType) {
            case SplitProcessCommandQuoteType::None:
                quoteType = SplitProcessCommandQuoteType::Double;
                break;
            case SplitProcessCommandQuoteType::Double:
                quoteType = SplitProcessCommandQuoteType::None;
                break;
            default:
                current+=cmd[i];
            }
            i++;
            break;
        case '\'':
            switch(quoteType) {
            case SplitProcessCommandQuoteType::None:
                quoteType = SplitProcessCommandQuoteType::Single;
                break;
            case SplitProcessCommandQuoteType::Single:
                quoteType = SplitProcessCommandQuoteType::None;
                break;
            default:
                current+=cmd[i];
            }
            i++;
            break;
        case '\\':
            current += cmd[i];
            i++;
            if  (i<cmd.length()) {
                current += cmd[i];
                i++;
            }
            break;
        default:
            current += cmd[i];
            i++;
        }
    }
    if (!current.isEmpty())
        result.append(current);
    return result;
}

FileType getFileType(const QString &filename)
{
    if (filename.endsWith(".dev",PATH_SENSITIVITY)) {
        return FileType::Project;
    }
    if (filename.endsWith(".C")) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".CPP")) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".c",PATH_SENSITIVITY)) {
        return FileType::CSource;
    }
    if (filename.endsWith(".cpp",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".cc",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".cxx",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".c++",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".H")) {
        return FileType::CHeader;
    }
    if (filename.endsWith(".h",PATH_SENSITIVITY)) {
        return FileType::CHeader;
    }
    if (filename.endsWith(".hpp",PATH_SENSITIVITY)) {
        return FileType::CppHeader;
    }
    if (filename.endsWith(".hh",PATH_SENSITIVITY)) {
        return FileType::CppHeader;
    }
    if (filename.endsWith(".hxx",PATH_SENSITIVITY)) {
        return FileType::CppHeader;
    }
    if (filename.endsWith(".inl",PATH_SENSITIVITY)) {
        return FileType::CppHeader;
    }
    if (filename.endsWith(".rc",PATH_SENSITIVITY)) {
        return FileType::WindowsResourceSource;
    }
    if (filename.endsWith(".in",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".out",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".txt",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".dat",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    QMimeDatabase db;
    QMimeType mimeType=db.mimeTypeForFile(filename);
    if (mimeType.isValid() && mimeType.name().startsWith("text/")) {
        return FileType::Text;
    }
    return FileType::Other;
}

QString genMakePath(const QString &fileName, bool escapeSpaces, bool encloseInQuotes)
{
    QString result = fileName;

    // Convert backslashes to slashes
    result.replace('\\','/');
    if (escapeSpaces) {
        result.replace(' ',"\\ ");
    }
    if (encloseInQuotes)
        if (result.contains(' '))
            result = '"'+result+'"';
    return result;
}

QString genMakePath1(const QString &fileName)
{
    return genMakePath(fileName, false, true);
}

QString genMakePath2(const QString &fileName)
{
    return genMakePath(fileName, true, false);
}

QString getCompiledExecutableName(const QString& filename)
{
    return changeFileExt(filename,EXECUTABLE_EXT);
}

bool programHasConsole(const QString & filename)
{
#ifdef Q_OS_WIN
    bool result = false;
    HANDLE handle = CreateFile(filename.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle != INVALID_HANDLE_VALUE) {
        IMAGE_DOS_HEADER dos_header;
        DWORD signature;
        DWORD bytesread;
        IMAGE_FILE_HEADER pe_header;
        IMAGE_OPTIONAL_HEADER opt_header;

        ReadFile(handle, &dos_header, sizeof(dos_header), &bytesread, NULL);
        SetFilePointer(handle, dos_header.e_lfanew, NULL, 0);
        ReadFile(handle, &signature, sizeof(signature), &bytesread, NULL);
        ReadFile(handle, &pe_header, sizeof(pe_header), &bytesread, NULL);
        ReadFile(handle, &opt_header, sizeof(opt_header), &bytesread, NULL);

        result = (opt_header.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);
    }
    CloseHandle(handle);
    return result;
#else
    return true;
#endif
}

QString parseMacros(const QString &s)
{
    QString result = s;
    Editor *e = pMainWindow->editorList()->getEditor();

    result.replace("<DEFAULT>", localizePath(QDir::currentPath()));
    result.replace("<DEVCPP>", localizePath(pSettings->dirs().executable()));
    result.replace("<DEVCPPVERSION>", REDPANDA_CPP_VERSION);
    result.replace("<EXECPATH>", localizePath(pSettings->dirs().appDir()));
    QDate today = QDate::currentDate();
    QDateTime now = QDateTime::currentDateTime();

    result.replace("<DATE>", today.toString("yyyy-MM-dd"));
    result.replace("<DATETIME>", now.toString("yyyy-MM-dd hh:mm:ss"));

    Settings::PCompilerSet compilerSet = pSettings->compilerSets().defaultSet();
    if (compilerSet) {
        // Only provide the first cpp include dir
        if (compilerSet->defaultCppIncludeDirs().count()>0)
            result.replace("<INCLUDE>", localizePath(compilerSet->defaultCppIncludeDirs().front()));
        else
            result.replace("<INCLUDE>","");

        // Only provide the first lib dir
        if (compilerSet->defaultLibDirs().count()>0)
            result.replace("<LIB>", localizePath(compilerSet->defaultLibDirs().front()));
        else
            result.replace("<LIB>","");
    }

    if (e!=nullptr && !e->inProject()) { // Non-project editor macros
            result.replace("<EXENAME>", extractFileName(changeFileExt(e->filename(),EXECUTABLE_EXT)));
            result.replace("<EXEFILE>", localizePath(changeFileExt(e->filename(),EXECUTABLE_EXT)));
            result.replace("<PROJECTNAME>", extractFileName(e->filename()));
            result.replace("<PROJECTFILE>", localizePath(e->filename()));
            result.replace("<PROJECTFILENAME>", extractFileName(e->filename()));
            result.replace("<PROJECTPATH>", localizePath(extractFileDir(e->filename())));
    } else if (pMainWindow->project()) {
        result.replace("<EXENAME>", extractFileName(pMainWindow->project()->executable()));
        result.replace("<EXEFILE>", localizePath(pMainWindow->project()->executable()));
        result.replace("<PROJECTNAME>", pMainWindow->project()->name());
        result.replace("<PROJECTFILE>", localizePath(pMainWindow->project()->filename()));
        result.replace("<PROJECTFILENAME>", extractFileName(pMainWindow->project()->filename()));
        result.replace("<PROJECTPATH>", localizePath(pMainWindow->project()->directory()));
    } else {
        result.replace("<EXENAME>", "");
        result.replace("<EXEFILE>", "");
        result.replace("<PROJECTNAME>", "");
        result.replace("<PROJECTFILE>", "");
        result.replace("<PROJECTFILENAME>", "");
        result.replace("<PROJECTPATH>", "");
    }

    // Editor macros
    if (e!=nullptr) {
        result.replace("<SOURCENAME>", extractFileName(e->filename()));
        result.replace("<SOURCEFILE>", localizePath(e->filename()));
        result.replace("<SOURCEPATH>", localizePath(extractFileDir(e->filename())));
        result.replace("<WORDXY>", e->wordAtCursor());
    } else {
        result.replace("<SOURCENAME>", "");
        result.replace("<SOURCEFILE>", "");
        result.replace("<SOURCEPATH>", "");
        result.replace("<WORDXY>", "");
    }
    return result;
}

void resetCppParser(std::shared_ptr<CppParser> parser, int compilerSetIndex)
{
    if (!parser)
        return;
    // Configure parser
    parser->reset();
    //paser->enabled = pSettings-> devCodeCompletion.Enabled;
//    CppParser.ParseLocalHeaders := devCodeCompletion.ParseLocalHeaders;
//    CppParser.ParseGlobalHeaders := devCodeCompletion.ParseGlobalHeaders;
    parser->setEnabled(true);
    parser->setParseGlobalHeaders(true);
    parser->setParseLocalHeaders(true);
    // Set options depending on the current compiler set
    // TODO: do this every time OnCompilerSetChanged
    if (compilerSetIndex<0) {
        compilerSetIndex=pSettings->compilerSets().defaultIndex();
    }
    Settings::PCompilerSet compilerSet = pSettings->compilerSets().getSet(compilerSetIndex);
    parser->clearIncludePaths();
    if (compilerSet) {
        foreach  (const QString& file,compilerSet->CppIncludeDirs()) {
            parser->addIncludePath(file);
        }
        foreach  (const QString& file,compilerSet->CIncludeDirs()) {
            parser->addIncludePath(file);
        }
        foreach  (const QString& file,compilerSet->defaultCppIncludeDirs()) {
            parser->addIncludePath(file);
        }
        foreach  (const QString& file,compilerSet->defaultCIncludeDirs()) {
            parser->addIncludePath(file);
        }
        //TODO: Add default include dirs last, just like gcc does
        // Set defines
        if (parser->language()==ParserLanguage::C) {
            for (QString define:compilerSet->CDefines()) {
                parser->addHardDefineByLine(define); // predefined constants from -dM -E
            }
        } else {
            for (QString define:compilerSet->CppDefines()) {
                parser->addHardDefineByLine(define); // predefined constants from -dM -E
            }
        }
        // add a Red Pand C++ 's own macro
        parser->addHardDefineByLine("#define EGE_FOR_AUTO_CODE_COMPLETETION_ONLY");
        // add C/C++ default macro
        parser->addHardDefineByLine("#define __FILE__  1");
        parser->addHardDefineByLine("#define __LINE__  1");
        parser->addHardDefineByLine("#define __DATE__  1");
        parser->addHardDefineByLine("#define __TIME__  1");
    }
    parser->parseHardDefines();
    pMainWindow->disconnect(parser.get(),
                            &CppParser::onStartParsing,
                            pMainWindow,
                            &MainWindow::onStartParsing);
    pMainWindow->disconnect(parser.get(),
                            &CppParser::onProgress,
                            pMainWindow,
                            &MainWindow::onParserProgress);
    pMainWindow->disconnect(parser.get(),
                            &CppParser::onEndParsing,
                            pMainWindow,
                            &MainWindow::onEndParsing);
    pMainWindow->connect(parser.get(),
                            &CppParser::onStartParsing,
                            pMainWindow,
                            &MainWindow::onStartParsing);
    pMainWindow->connect(parser.get(),
                            &CppParser::onProgress,
                            pMainWindow,
                            &MainWindow::onParserProgress);
    pMainWindow->connect(parser.get(),
                            &CppParser::onEndParsing,
                            pMainWindow,
                            &MainWindow::onEndParsing);
}

int getNewFileNumber()
{
    static int count = 0;
    count++;
    return count;
}
