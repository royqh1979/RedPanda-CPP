#include "settings.h"
#include <QApplication>
#include <QTextCodec>
#include <algorithm>
#include "utils.h"
#include <QDir>
#include "systemconsts.h"

const char ValueToChar[28] = {'0', '1', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                              'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                              's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};


Settings* pSettings;

Settings::Settings(const QString &filename):
    mSettings(filename,QSettings::IniFormat),
    mDirs(this),
    mEditor(this),
    mCompilerSets(this)
{
    // default values for editors
    mEditor.setDefault(SETTING_EDITOR_DEFAULT_ENCODING, QTextCodec::codecForLocale()->name());
    mEditor.setDefault(SETTING_EDITOR_AUTO_INDENT,true);

}

void Settings::setDefault(const QString&group,const QString &key, const QVariant &value) {
    mSettings.beginGroup(group);
    auto act = finally([this] {
        this->mSettings.endGroup();
    });
    if (!mSettings.contains(key)) {
        mSettings.setValue(key,value);
    }
}

void Settings::setValue(const QString& group, const QString &key, const QVariant &value) {
    mSettings.beginGroup(group);
    auto act = finally([this] {
        this->mSettings.endGroup();
    });
    mSettings.setValue(key,value);
}

QVariant Settings::value(const QString& group, const QString &key) {
    mSettings.beginGroup(group);
    auto act = finally([this] {
        this->mSettings.endGroup();
    });
    return mSettings.value(key);
}

Settings::Dirs &Settings::dirs()
{
    return mDirs;
}

Settings::Editor &Settings::editor()
{
    return mEditor;
}

Settings::CompilerSets &Settings::compilerSets()
{
    return mCompilerSets;
}

Settings::Dirs::Dirs(Settings *settings):
    _Base(settings, SETTING_DIRS)
{
}

const QString Settings::Dirs::app() const
{
    QApplication::instance()->applicationDirPath();
}

Settings::_Base::_Base(Settings *settings, const QString &groupName):
    mSettings(settings),
    mGroup(groupName)
{

}

void Settings::_Base::setDefault(const QString &key, const QVariant &value)
{
    mSettings->setDefault(mGroup,key,value);
}

void Settings::_Base::setValue(const QString &key, const QVariant &value)
{
    mSettings->setValue(mGroup,key,value);
}

QVariant Settings::_Base::value(const QString &key)
{
    return mSettings->value(mGroup,key);
}

Settings::Editor::Editor(Settings *settings): _Base(settings, SETTING_EDITOR)
{

}

QByteArray Settings::Editor::defaultEncoding()
{
    return value(SETTING_EDITOR_DEFAULT_ENCODING).toByteArray();
}

void Settings::Editor::setDefaultEncoding(const QByteArray &encoding)
{
    setValue(SETTING_EDITOR_DEFAULT_ENCODING,encoding);
}

bool Settings::Editor::autoIndent()
{
    return value(SETTING_EDITOR_AUTO_INDENT).toBool();
}

void Settings::Editor::setAutoIndent(bool indent)
{
    setValue(SETTING_EDITOR_AUTO_INDENT,indent);
}



Settings::CompilerSet::CompilerSet(const QString& compilerFolder):
    mStaticLink(true),
    mAutoAddCharsetParams(true)
{
    if (!compilerFolder.isEmpty()) {
        setProperties(compilerFolder+"/bin");

        setExecutables();

        //manually set the directories
        setDirectories();

        setUserInput();
    }
    setOptions();
}

Settings::CompilerSet::CompilerSet(const Settings::CompilerSet &set):
    mCCompilerName(set.mCCompilerName),
    mCppCompilerName(set.mCppCompilerName),
    mMakeName(set.mMakeName),
    mDebuggerName(set.mDebuggerName),
    mProfilerName(set.mProfilerName),
    mResourceCompilerName(set.mResourceCompilerName),
    mBinDirs(set.mBinDirs),
    mCIncludeDirs(set.mCIncludeDirs),
    mCppIncludeDirs(set.mCppIncludeDirs),
    mLibDirs(set.mLibDirs),
    mDumpMachine(set.mDumpMachine),
    mVersion(set.mVersion),
    mType(set.mType),
    mName(set.mName),
    mFolder(set.mFolder),
    mDefines(set.mDefines),
    mTarget(set.mTarget),
    mUseCustomCompileParams(set.mUseCustomCompileParams),
    mUseCustomLinkParams(set.mUseCustomLinkParams),
    mCustomCompileParams(set.mCustomCompileParams),
    mCustomLinkParams(set.mCustomLinkParams),
    mStaticLink(set.mStaticLink),
    mAutoAddCharsetParams(set.mAutoAddCharsetParams)
{
    // Executables, most are hardcoded
    for (PCompilerOption pOption:set.mOptions) {
        PCompilerOption p=std::make_shared<CompilerOption>();
        *p=*pOption;
        mOptions.push_back(pOption);
    }
}

void Settings::CompilerSet::addOption(const QString &name, const QString section, bool isC,
    bool isCpp, bool isLinker, int value, const QString &setting, const QStringList &choices)
{
    PCompilerOption pOption = std::make_shared<CompilerOption>();
    pOption->name = name;
    pOption->section = section;
    pOption->isC = isC;
    pOption->isCpp = isCpp;
    pOption->isLinker = isLinker;
    pOption->value = value;
    pOption->setting= setting;
    pOption->choices = choices;
    mOptions.push_back(pOption);
}

PCompilerOption Settings::CompilerSet::findOption(const QString &setting)
{
    for (PCompilerOption pOption : mOptions) {
        if (pOption->setting == setting) {
            return pOption;
        }
    }
    return PCompilerOption();
}

char Settings::CompilerSet::getOptionValue(const QString &setting)
{
    PCompilerOption pOption = findOption(setting);
    if (pOption) {
        return ValueToChar[pOption->value];
    } else {
        return '0';
    }
}

void Settings::CompilerSet::setOption(const QString &setting, char valueChar)
{
    PCompilerOption pOption = findOption(setting);
    if (pOption) {
        setOption(pOption,valueChar);
    }
}

void Settings::CompilerSet::setOption(PCompilerOption &option, char valueChar)
{
    option->value = charToValue(valueChar);
}

const QString &Settings::CompilerSet::CCompilerName() const
{
    return mCCompilerName;
}

void Settings::CompilerSet::setCCompilerName(const QString &name)
{
    mCCompilerName = name;
}

const QString &Settings::CompilerSet::cppCompilerName() const
{
    return mCppCompilerName;
}

void Settings::CompilerSet::setCppCompilerName(const QString &name)
{
    mCppCompilerName = name;
}

const QString &Settings::CompilerSet::makeName() const
{
    return mMakeName;
}

void Settings::CompilerSet::setMakeName(const QString &name)
{
    mMakeName = name;
}

const QString &Settings::CompilerSet::debuggerName() const
{
    return mDebuggerName;
}

void Settings::CompilerSet::setDebuggerName(const QString &name)
{
    mDebuggerName = name;
}

const QString &Settings::CompilerSet::profilerName() const
{
    return mProfilerName;
}

void Settings::CompilerSet::setProfilerName(const QString &name)
{
    mProfilerName = name;
}

const QString &Settings::CompilerSet::resourceCompilerName() const
{
    return mResourceCompilerName;
}

void Settings::CompilerSet::setResourceCompilerName(const QString &name)
{
    mResourceCompilerName = name;
}

QStringList &Settings::CompilerSet::binDirs()
{
    return mBinDirs;
}

QStringList &Settings::CompilerSet::CIncludeDirs()
{
    return mCIncludeDirs;
}

QStringList &Settings::CompilerSet::CppIncludeDirs()
{
    return mCppIncludeDirs;
}

QStringList &Settings::CompilerSet::LibDirs()
{
    return mLibDirs;
}

const QString &Settings::CompilerSet::dumpMachine()
{
    return mDumpMachine;
}

void Settings::CompilerSet::setDumpMachine(const QString &value)
{
    mDumpMachine = value;
}

const QString &Settings::CompilerSet::version()
{
    return mVersion;
}

void Settings::CompilerSet::setVersion(const QString &value)
{
    mVersion = value;
}

const QString &Settings::CompilerSet::type()
{
    return mType;
}

void Settings::CompilerSet::setType(const QString& value)
{
    mType = value;
}

const QString &Settings::CompilerSet::name()
{
    return mName;
}

void Settings::CompilerSet::setName(const QString &value)
{
    mName = value;
}

const QString &Settings::CompilerSet::folder()
{
    return mFolder;
}

void Settings::CompilerSet::setFolder(const QString &value)
{
    mFolder = value;
}

QStringList& Settings::CompilerSet::defines()
{
    return mDefines;
}

const QString &Settings::CompilerSet::target()
{
    return mTarget;
}

void Settings::CompilerSet::setTarget(const QString &value)
{
    mTarget = value;
}

void Settings::CompilerSet::setUseCustomCompileParams(bool value)
{
    mUseCustomCompileParams = value;
}

bool Settings::CompilerSet::useCustomLinkParams()
{
    return mUseCustomLinkParams;
}

void Settings::CompilerSet::setUseCustomLinkParams(bool value)
{
    mUseCustomLinkParams = value;
}

const QString &Settings::CompilerSet::customCompileParams()
{
    return mCustomCompileParams;
}

void Settings::CompilerSet::setCustomCompileParams(const QString &value)
{
    mCustomCompileParams = value;
}

const QString &Settings::CompilerSet::customLinkParams()
{
    return mCustomLinkParams;
}

void Settings::CompilerSet::setCustomLinkParams(const QString &value)
{
    mCustomLinkParams = value;
}

bool Settings::CompilerSet::staticLink()
{
    return mStaticLink;
}

void Settings::CompilerSet::setStaticLink(bool value)
{
    mStaticLink = value;
}

bool Settings::CompilerSet::autoAddCharsetParams()
{
    return mAutoAddCharsetParams;
}

void Settings::CompilerSet::setAutoAddCharsetParams(bool value)
{
    mAutoAddCharsetParams = value;
}

CompilerOptionList &Settings::CompilerSet::options()
{
    return mOptions;
}

int Settings::CompilerSet::charToValue(char valueChar)
{
    if (valueChar == '1') {
        return 1;
    } else if ( (valueChar>='a') && (valueChar<='z')) {
        return (valueChar-'a')+2;
    } else {
        return 0;
    }
}

static void addExistingDirectory(QStringList& dirs, const QString& directory) {
    if (!directoryExists(directory))
        return;
    QFileInfo dirInfo(directory);
    QString dirPath = dirInfo.absoluteFilePath();
    if (dirs.contains(dirPath))
        return;
    dirs.append(dirPath);
}

void Settings::CompilerSet::setProperties(const QString &binDir)
{
    if (!fileExists(binDir,GCC_PROGRAM))
        return;
    // Obtain version number and compiler distro etc
    QStringList arguments;
    arguments.append("-v");
    QByteArray output = getCompilerOutput(binDir,GCC_PROGRAM,arguments);

    //Target
    QByteArray targetStr = "Target: ";
    int delimPos1 = output.indexOf(targetStr);
    if (delimPos1<0)
        return; // unknown binary
    delimPos1+=strlen(targetStr);
    int delimPos2 = delimPos1;
    while (delimPos2<output.length() && !isNonPrintableAsciiChar(output[delimPos2]))
        delimPos2++;
    mTarget = output.mid(delimPos1,delimPos2-delimPos1);

    if (mTarget.contains("x86_64"))
        mTarget = "x86_64";
    else
        mTarget = "i686";

    //Find version number
    targetStr = "gcc version ";
    delimPos1 = output.indexOf(targetStr);
    if (delimPos1<0)
        return; // unknown binary
    delimPos1+=strlen(targetStr);
    delimPos2 = delimPos1;
    while (delimPos2<output.length() && !isNonPrintableAsciiChar(output[delimPos2]))
        delimPos2++;
    mVersion = output.mid(delimPos1,delimPos2-delimPos1);


    // Find compiler builder
    delimPos1 = delimPos2;
    while ((delimPos1 < output.length()) && !(output[delimPos1] == '('))
        delimPos1++;
    while ((delimPos2 < output.length()) && !(output[delimPos2] == ')'))
        delimPos2++;
    mType = output.mid(delimPos1 + 1, delimPos2 - delimPos1 - 1);

    // Assemble user friendly name if we don't have one yet
    if (mName == "") {
        if (mType.contains("tdm64")) {
            mName = "TDM-GCC " + mVersion;
        } else if (mType.contains("tdm")) {
            mName = "TDM-GCC " + mVersion;
        } else if (mType.contains("MSYS2")) {
            mName = "MinGW-w64 GCC " + mVersion;
        } else if (mType.contains("GCC")) {
            mName = "MinGW GCC " + mVersion;
        } else {
            mName = "MinGW GCC " + mVersion;
        }
    }

    // Set compiler folder
    QDir tmpDir(binDir);
    tmpDir.cdUp();
    mFolder = tmpDir.path();

    // Obtain compiler target
    arguments.clear();
    arguments.append("-dumpmachine");
    mDumpMachine = getCompilerOutput(binDir, GCC_PROGRAM, arguments);

    // Add the default directories
    addExistingDirectory(mBinDirs, mFolder + QDir::separator() + "bin");
    addExistingDirectory(mLibDirs, mFolder + QDir::separator() + "lib");
    addExistingDirectory(mCIncludeDirs, mFolder + QDir::separator() + "include");
    addExistingDirectory(mCppIncludeDirs, mFolder + QDir::separator() + "include");

    // Find default directories
    arguments.clear();
    arguments.append("-xc");
    arguments.append("-v");
    arguments.append("-E");
    arguments.append(NULL_FILE);
    output = getCompilerOutput(binDir,GCC_PROGRAM,arguments);

    // C include dirs
    delimPos1 = output.indexOf("#include <...> search starts here:");
    delimPos2 = output.indexOf("End of search list.");
    if (delimPos1 >0 && delimPos2>0 ) {
        delimPos1 += QByteArray("#include <...> search starts here:").length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mCIncludeDirs,trimmedLine);
            }
        }
    }
    // bin dirs
    targetStr = QByteArray("COMPILER_PATH=");
    delimPos1 = output.indexOf(targetStr);
    if (delimPos1>=0) {
        delimPos1+=targetStr.length();
        delimPos2 = delimPos1;
        while (delimPos2 < output.length() && output[delimPos2]!='\n')
            delimPos2+=1;
        QList<QByteArray> lines = output.mid(delimPos1,delimPos2-delimPos1).split(';');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            addExistingDirectory(mBinDirs,trimmedLine);
        }
    }
    // lib dirs
    targetStr = QByteArray("LIBRARY_PATH=");
    delimPos1 = output.indexOf(targetStr);
    if (delimPos1>=0) {
        delimPos1+=targetStr.length();
        delimPos2 = delimPos1;
        while (delimPos2 < output.length() && output[delimPos2]!='\n')
            delimPos2+=1;
        QList<QByteArray> lines = output.mid(delimPos1,delimPos2-delimPos1).split(';');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            addExistingDirectory(mLibDirs,trimmedLine);
        }
    }

    arguments.clear();
    arguments.append("-xc++");
    arguments.append("-E");
    arguments.append("-v");
    arguments.append(NULL_FILE);
    output = getCompilerOutput(binDir,GCC_PROGRAM,arguments);
    //gcc -xc++ -E -v NUL

    // C include dirs
    delimPos1 = output.indexOf("#include <...> search starts here:");
    delimPos2 = output.indexOf("End of search list.");
    if (delimPos1 >0 && delimPos2>0 ) {
        delimPos1 += QByteArray("#include <...> search starts here:").length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mCppIncludeDirs,trimmedLine);
            }
        }
    }

    // get default defines
    arguments.clear();
    arguments.append("-dM");
    arguments.append("-E");
    arguments.append("-x");
    arguments.append("c++");
    arguments.append("-std=c++17");
    arguments.append(NULL_FILE);
    output = getCompilerOutput(binDir,GCC_PROGRAM,arguments);
    // 'cpp.exe -dM -E -x c++ -std=c++17 NUL'

    mDefines.clear();
    QList<QByteArray> lines = output.split('\n');
    for (QByteArray& line:lines) {
        QByteArray trimmedLine = line.trimmed();
        if (!trimmedLine.isEmpty()) {
            mDefines.append(trimmedLine);
        }
    }
}

void Settings::CompilerSet::setExecutables()
{
    mCCompilerName = GCC_PROGRAM;
    mCppCompilerName = GPP_PROGRAM;
    mDebuggerName = GDB_PROGRAM;
    mMakeName = MAKE_PROGRAM;
    mResourceCompilerName = WINDRES_PROGRAM;
    mProfilerName = GPROF_PROGRAM;
}

void Settings::CompilerSet::setDirectories()
{
    // Try to obtain our target/autoconf folder
    if (!mDumpMachine.isEmpty()) {
        //mingw-w64 bin folder
        addExistingDirectory(mBinDirs,
            mFolder + QDir::separator() + "lib"
            + QDir::separator() + "gcc" + mDumpMachine
            + QDir::separator() + mVersion);

        // Regular include folder
        addExistingDirectory(mCIncludeDirs, mFolder + QDir::separator() + mDumpMachine + QDir::separator() + "include");
        addExistingDirectory(mCppIncludeDirs, mFolder + QDir::separator()+ mDumpMachine + QDir::separator() + "include");

        // Other include folder?
        addExistingDirectory(mCIncludeDirs,
            mFolder + QDir::separator() + "lib" + QDir::separator() + "gcc"
            + QDir::separator() + mDumpMachine + QDir::separator()
            + mVersion + QDir::separator() + "include");
        addExistingDirectory(mCppIncludeDirs,
            mFolder + QDir::separator() + "lib" + QDir::separator() + "gcc"
            + QDir::separator() + mDumpMachine + QDir::separator()
            + mVersion + QDir::separator() + "include");

        addExistingDirectory(mCIncludeDirs,
            mFolder + QDir::separator()  + "lib"
                + QDir::separator()  + "gcc" + QDir::separator()  + mDumpMachine
                + QDir::separator() + mVersion + QDir::separator() + "include-fixed");
        addExistingDirectory(mCppIncludeDirs,
            mFolder + QDir::separator()  + "lib"
                + QDir::separator()  + "gcc" + QDir::separator()  + mDumpMachine
                + QDir::separator() + mVersion + QDir::separator() + "include-fixed");

        // C++ only folder (mingw.org)
        addExistingDirectory(mCppIncludeDirs,
            mFolder + QDir::separator() + "lib" + QDir::separator() + "gcc"
            + QDir::separator() + mDumpMachine + QDir::separator() + mVersion
            + QDir::separator() + "include" + QDir::separator() + "c++");
        addExistingDirectory(mCppIncludeDirs,
            mFolder + QDir::separator() + "lib" + QDir::separator() + "gcc"
            + QDir::separator() + mDumpMachine + QDir::separator() + mVersion
            + QDir::separator() + "include" + QDir::separator() + "c++"
            + QDir::separator() + mDumpMachine );
        addExistingDirectory(mCppIncludeDirs,
            mFolder + QDir::separator() + "lib" + QDir::separator() + "gcc"
            + QDir::separator() + mDumpMachine + QDir::separator() + mVersion
            + QDir::separator() + "include" + QDir::separator() + "c++"
            + QDir::separator() + "backward");

        // C++ only folder (Mingw-w64)
        addExistingDirectory(mCppIncludeDirs,
            mFolder + QDir::separator()  + "include" + QDir::separator() + "c++"
            + QDir::separator()  + mVersion );
        addExistingDirectory(mCppIncludeDirs,
            mFolder + QDir::separator()  + "include" + QDir::separator() + "c++"
            + QDir::separator()  + mVersion + QDir::separator() + mDumpMachine );
        addExistingDirectory(mCppIncludeDirs,
            mFolder + QDir::separator()  + "include" + QDir::separator() + "c++"
            + QDir::separator()  + mVersion + QDir::separator() + "backward");
    }
}

void Settings::CompilerSet::setUserInput()
{
    mUseCustomCompileParams = false;
    mUseCustomLinkParams = false;
    mStaticLink = true;
    mAutoAddCharsetParams = true;
}

inline QString tr(const char* str) {
    return QObject::tr(str);
}

void Settings::CompilerSet::setOptions()
{
    // C options
    QString groupName = QObject::tr("C options");
    addOption(tr("Support all ANSI standard C programs (-ansi)"), groupName, true, true, false, 0, "-ansi");
    addOption(tr("Do not recognize asm,inline or typeof as a keyword (-fno-asm)"), groupName, true, true, false, 0, "-fno-asm");
    addOption(tr("Imitate traditional C preprocessors (-traditional-cpp)"), groupName, true, true, false, 0, "-traditional-cpp");

    // Optimization for cpu type
    groupName = QObject::tr("Code Generation");
    QStringList sl;
    sl.append(""); // /!\ Must contain a starting empty value in order to do not have always to pass the parameter
    sl.append("This CPU=native");
    sl.append("i386=i386");
    sl.append("i486=i486");
    sl.append("i586=i586");
    sl.append("i686=i686");
    sl.append("Pentium=pentium");
    sl.append("Pentium MMX=pentium-mmx");
    sl.append("Pentium Pro=pentiumpro");
    sl.append("Pentium 2=pentium2");
    sl.append("Pentium 3=pentium3");
    sl.append("Pentium 4=pentium4");
    sl.append("Conroe=core2");
    sl.append("Nehalem=corei7");
    sl.append("Sandy=corei7-avx");
    sl.append("K6=k6");
    sl.append("K6-2=k6-2");
    sl.append("K6-3=k6-3");
    sl.append("Athlon=athlon");
    sl.append("Athlon Tbird=athlon-tbird");
    sl.append("Athlon 4=athlon-4");
    sl.append("Athlon XP=athlon-xp");
    sl.append("Athlon MP=athlon-mp");
    sl.append("K8=k8");
    sl.append("K8 Rev.E=k8-sse3");
    sl.append("K10=barcelona");
    sl.append("Bulldozer=bdver1");
    addOption(tr("Optimize for the following machine (-march)"), groupName, true, true, false, 0, "-march=", sl);
    addOption(tr("Optimize less, while maintaining full compatibility (-tune)"), groupName, true, true, false, 0, "-mtune=", sl);

    // Enable use of the specific instructions
    sl.clear();
    sl.append(""); // /!\ Must contain a starting empty value in order to do not have always to pass the parameter
    sl.append("MMX=mmx");
    sl.append("3D Now=3dnow");
    sl.append("SSE=sse");
    sl.append("SSE2=sse2");
    sl.append("SSE3=sse3");
    sl.append("SSSE3=ssse3");
    sl.append("SSE4=sse4");
    sl.append("SSE4A=sse4a");
    sl.append("SSE4.1=sse4.1");
    sl.append("SSE4.2=sse4.2");
    sl.append("AVX=avx");
    sl.append("AVX2=avx2");
    sl.append("FMA4=fma4");
    sl.append("XOP=xop");
    sl.append("AES=aes");
    addOption(tr("Enable use of specific instructions (-mx)"), groupName, true, true, false, 0, "-m", sl);

    // Optimization
    sl.clear();
    sl.append("");
    sl.append("Low=1");
    sl.append("Med=2");
    sl.append("High=3");
    sl.append("Highest (fast)=fast");
    sl.append("Size (s)=s");
    sl.append("Debug (g)=g");
    addOption(tr("Optimization level (-Ox)"), groupName, true, true, false, 0, "-O", sl);

    // 32bit/64bit
    sl.clear();
    sl.append("");
    sl.append("32bit=m32");
    sl.append("64bit=m64");
    addOption(tr("Compile with the following pointer size (-mx)"), groupName, true, true, true, 0, "-", sl);

    // Language Standards
    sl.clear();
    sl.append(""); // Passing nothing effectively lets the compiler decide
    sl.append("ISO C90=c90");
    sl.append("ISO C99=c99");
    sl.append("ISO C11=c11");
    sl.append("ISO C17=c17");
    sl.append("ISO C++=c++98");
    sl.append("ISO C++11=c++11");
    sl.append("ISO C++14=c++14");
    sl.append("ISO C++17=c++17");
    sl.append("ISO C++20=c++2a");
    sl.append("GNU C90=gnu90");
    sl.append("GNU C99=gnu99");
    sl.append("GNU C11=gnu11");
    sl.append("GNU C17=gnu17");
    sl.append("GNU C++=gnu++98");
    sl.append("GNU C++11=gnu++11");
    sl.append("GNU C++14=gnu++14");
    sl.append("GNU C++17=gnu++17");
    sl.append("GNU C++20=gnu++20");
    addOption(tr("Language standard (-std)"), groupName, true, true, false, 0, "-std=", sl);

    // Warnings
    groupName = tr("Warnings");
    addOption(tr("Inhibit all warning messages (-w)"), groupName, true, true, false, 0, "-w");
    addOption(tr("Show most warnings (-Wall)"), groupName, true, true, false, 0, "-Wall");
    addOption(tr("Show some more warnings (-Wextra)"), groupName, true, true, false, 0, "-Wextra");
    addOption(tr("Check ISO C/C++/C++0x conformance (-pedantic)"), groupName, true, true, false, 0, "-pedantic");
    addOption(tr("Only check the code for syntax errors (-fsyntax-only)"), groupName, true, true, false, 0, "-fsyntax-only");
    addOption(tr("Make all warnings into errors (-Werror)"), groupName, true, true, false, 0, "-Werror");
    addOption(tr("Abort compilation on first error (-Wfatal-errors)"), groupName, true, true, false, 0, "-Wfatal-errors");

    // Profiling
    groupName = tr("Profiling");
    addOption(tr("Generate profiling info for analysis (-pg)"), groupName, true, true, true, 0, "-pg");

    // Linker
    groupName = tr("Linker");
    addOption(tr("Link an Objective C program (-lobjc)"), groupName, false, false, true, 0, "-lobjc");
    addOption(tr("Do not use standard system libraries (-nostdlib)"), groupName, true, true, true, 0, "-nostdlib");
    addOption(tr("Do not create a console window (-mwindows)"), groupName,true, true, true, 0, "-mwindows");
    addOption(tr("Strip executable (-s)"), groupName, false, false, true, 0, "-s");
    addOption(tr("Generate debugging information (-g3)"), groupName, true, true, true, 0, "-g3");

    // Output
    groupName = tr("Output");
    addOption(tr("-fverbose-asm"), groupName, true, true, false, 0, "-fverbose-asm");
    addOption(tr("Do not assemble, but output assembler code (-S)"), groupName, true, true, false, 0, "-S");
    addOption(tr("Use pipes instead of temporary files during compilation (-pipe)"), groupName, true, true, false, 0, "-pipe");
}

QByteArray Settings::CompilerSet::iniOptions() const
{
    QByteArray result;
    for (PCompilerOption p:mOptions) {
        result.append(ValueToChar[p->value]);
    }
    return result;
}

void Settings::CompilerSet::setIniOptions(const QByteArray &value)
{
   int i=0;
   for (PCompilerOption p:mOptions) {
       if (i>=value.length()) {
           break;
       }
       p->value = charToValue(value[i]);
   }
}

QByteArray Settings::CompilerSet::getCompilerOutput(const QString &binDir, const QString &binFile, const QStringList &arguments)
{
    QByteArray result = runAndGetOutput(binDir + QDir::separator()+binFile, binDir, arguments);
    return result.trimmed();
}

bool Settings::CompilerSet::useCustomCompileParams()
{
    return mUseCustomCompileParams;
}

Settings::CompilerSets::CompilerSets(Settings *settings):
    mSettings(settings),
    mDefaultIndex(-1)
{

}

Settings::PCompilerSet Settings::CompilerSets::addSet(const Settings::CompilerSet& set)
{
    PCompilerSet p=std::make_shared<CompilerSet>(set);
    mList.push_back(p);
    return p;
}

Settings::PCompilerSet Settings::CompilerSets::addSet(const QString &folder)
{
    PCompilerSet p=std::make_shared<CompilerSet>(folder);
    mList.push_back(p);
    return p;
}

static void setReleaseOptions(Settings::PCompilerSet& pSet) {
    PCompilerOption pOption = pSet->findOption("-O");
    if (pOption) {
        pSet->setOption(pOption,'a');
    }

    pOption = pSet->findOption("-s");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }
}

static void setDebugOptions(Settings::PCompilerSet& pSet) {
    PCompilerOption pOption = pSet->findOption("-g3");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }
    pOption = pSet->findOption("-Wall");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }
    pOption = pSet->findOption("-Wextra");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }
}

static void setProfileOptions(Settings::PCompilerSet& pSet) {
    PCompilerOption pOption = pSet->findOption("-pg");
    if (pOption) {
        pSet->setOption(pOption,'1');
    }
}

void Settings::CompilerSets::addSets(const QString &folder)
{
    if (!directoryExists(folder))
        return;
    if (!fileExists(includeTrailingPathDelimiter(folder)+"bin"+QDir::separator()+GCC_PROGRAM)) {
        return;
    }
    // Default, release profile
    PCompilerSet baseSet = addSet(folder);
    QString baseName = baseSet->name();
    QString platformName;
    if (baseSet->target() == "x86_64") {
        platformName = "64-bit";
    } else {
        platformName = "32-bit";
    }
    baseSet->setName(baseName + " " + platformName + " Release");
    setReleaseOptions(baseSet);

    baseSet = addSet(folder);
    baseSet->setName(baseName + " " + platformName + " Debug");
    setDebugOptions(baseSet);

    baseSet = addSet(folder);
    baseSet->setName(baseName + " " + platformName + " Profiling");
    setProfileOptions(baseSet);

    mDefaultIndex = mList.size() - 2;
}

void Settings::CompilerSets::clearSets()
{
    for (int i=0;i<mList.size();i++) {
        mSettings->mSettings.beginGroup(QString(SETTING_COMPILTER_SET).arg(i));
        mSettings->mSettings.remove("");
        mSettings->mSettings.endGroup();
    }
    mList.clear();
}

void Settings::CompilerSets::findSets()
{
    addSets(includeTrailingPathDelimiter(mSettings->dirs().app())+"MinGW32");
    addSets(includeTrailingPathDelimiter(mSettings->dirs().app())+"MinGW64");
}

void Settings::CompilerSets::saveSets()
{
    for (int i=0;i<mList.size();i++) {
        saveSet(i);
    }
    mSettings->mSettings.beginGroup(SETTING_COMPILTER_SETS);
    mSettings->mSettings.setValue(SETTING_COMPILTER_SETS_DEFAULT_INDEX,mDefaultIndex);
    mSettings->mSettings.setValue(SETTING_COMPILTER_SETS_COUNT,mList.size());
    mSettings->mSettings.endGroup();
}

void Settings::CompilerSets::loadSets()
{
    mList.clear();
    mSettings->mSettings.beginGroup(SETTING_COMPILTER_SETS);
    mDefaultIndex =mSettings->mSettings.value(SETTING_COMPILTER_SETS_DEFAULT_INDEX,-1).toInt();
    int listSize = mSettings->mSettings.value(SETTING_COMPILTER_SETS_COUNT,0).toInt();
    mSettings->mSettings.endGroup();
    for (int i=0;i<listSize;i++) {
        PCompilerSet pSet=loadSet(i);
        mList.push_back(pSet);
    }
}

Settings::CompilerSetList &Settings::CompilerSets::list()
{
    return mList;
}

int Settings::CompilerSets::size() const
{
    return mList.size();
}

int Settings::CompilerSets::defaultIndex() const
{
    return mDefaultIndex;
}

void Settings::CompilerSets::setDefaultIndex(int value)
{
    mDefaultIndex = value;
}

void Settings::CompilerSets::saveSet(int index)
{
    PCompilerSet pSet = mList[index];
    mSettings->mSettings.beginGroup(QString(SETTING_COMPILTER_SET).arg(index));
    mSettings->mSettings.setValue("ccompiler", pSet->CCompilerName());
    mSettings->mSettings.setValue("cppcompiler", pSet->cppCompilerName());
    mSettings->mSettings.setValue("debugger", pSet->debuggerName());
    mSettings->mSettings.setValue("make", pSet->makeName());
    mSettings->mSettings.setValue("windres", pSet->resourceCompilerName());
    mSettings->mSettings.setValue("profiler", pSet->profilerName());

    // Save option string
    mSettings->mSettings.setValue("Options", pSet->iniOptions());

    // Save extra 'general' options
    mSettings->mSettings.setValue("useCustomCompileParams", pSet->useCustomCompileParams());
    mSettings->mSettings.setValue("customCompileParams", pSet->customCompileParams());
    mSettings->mSettings.setValue("useCustomLinkParams", pSet->useCustomLinkParams());
    mSettings->mSettings.setValue("customLinkParams", pSet->customLinkParams());
    mSettings->mSettings.setValue("StaticLink", pSet->staticLink());
    mSettings->mSettings.setValue("AddCharset", pSet->autoAddCharsetParams());

    // Paths
    mSettings->mSettings.setValue("Bins",pSet->binDirs());
    mSettings->mSettings.setValue("C",pSet->CIncludeDirs());
    mSettings->mSettings.setValue("Cpp",pSet->CppIncludeDirs());
    mSettings->mSettings.setValue("Libs",pSet->LibDirs());

    mSettings->mSettings.endGroup();
}

Settings::PCompilerSet Settings::CompilerSets::loadSet(int index)
{
    PCompilerSet pSet = std::make_shared<CompilerSet>();
    mSettings->mSettings.beginGroup(QString(SETTING_COMPILTER_SET).arg(index));
    pSet->setCCompilerName(mSettings->mSettings.value("ccompiler").toString());
    pSet->setCppCompilerName(mSettings->mSettings.value("cppcompiler").toString());
    pSet->setDebuggerName(mSettings->mSettings.value("debugger").toString());
    pSet->setMakeName(mSettings->mSettings.value("make").toString());
    pSet->setResourceCompilerName(mSettings->mSettings.value("windres").toString());
    pSet->setProfilerName(mSettings->mSettings.value("profiler").toString());

    // Save option string
    pSet->setIniOptions(mSettings->mSettings.value("Options").toByteArray());

    // Save extra 'general' options
    pSet->setUseCustomCompileParams(mSettings->mSettings.value("useCustomCompileParams").toBool());
    pSet->setCustomCompileParams(mSettings->mSettings.value("customCompileParams").toString());
    pSet->setUseCustomLinkParams(mSettings->mSettings.value("useCustomLinkParams").toBool());
    pSet->setCustomLinkParams(mSettings->mSettings.value("customLinkParams").toString());
    pSet->setStaticLink(mSettings->mSettings.value("StaticLink").toBool());
    pSet->setAutoAddCharsetParams(mSettings->mSettings.value("AddCharset").toBool());

    // Paths
    pSet->binDirs().clear();
    pSet->binDirs().append(mSettings->mSettings.value("Bins").toStringList());
    pSet->CIncludeDirs().clear();
    pSet->CIncludeDirs().append(mSettings->mSettings.value("C").toStringList());
    pSet->CppIncludeDirs().clear();
    pSet->CppIncludeDirs().append(mSettings->mSettings.value("Cpp").toStringList());
    pSet->LibDirs().clear();
    pSet->LibDirs().append(mSettings->mSettings.value("Libs").toStringList());

    mSettings->mSettings.endGroup();
    return pSet;
}
