#include "compilerinfo.h"
#include <QObject>

CompilerInfo::CompilerInfo(const QString &name):
    mName(name)
{
    init();
}

const QList<PCompilerOption> &CompilerInfo::compilerOptions() const
{
    return mCompilerOptionList;
}

const QString &CompilerInfo::name() const
{
    return mName;
}

PCompilerOption CompilerInfo::getCompilerOption(const QString &key) const
{
    return mCompilerOptions.value(key,PCompilerOption());
}

bool CompilerInfo::hasCompilerOption(const QString &key) const
{
    return mCompilerOptions.contains(key);
}

void CompilerInfo::addOption(const QString &key, const QString &name, const QString section, bool isC, bool isCpp, bool isLinker, const QString &setting, const CompileOptionChoiceList &choices)
{
    PCompilerOption pOption = std::make_shared<CompilerOption>();
    pOption->key = key;
    pOption->name = name;
    pOption->section = section;
    pOption->isC = isC;
    pOption->isCpp = isCpp;
    pOption->isLinker = isLinker;
    pOption->setting= setting;
    pOption->choices = choices;
    mCompilerOptions.insert(key,pOption);
    mCompilerOptionList.append(pOption);
}

void CompilerInfo::init()
{
    prepareCompilerOptions();
}

void CompilerInfo::prepareCompilerOptions()
{
    QList<QPair<QString,QString>> sl;
    QString groupName;
//    // C options
//    groupName = QObject::tr("C options");
//    addOption(CC_CMD_OPT_ANSI, QObject::tr("Support all ANSI standard C programs (-ansi)"), groupName, true, true, false, "-ansi");
//    addOption(CC_CMD_OPT_NO_ASM, QObject::tr("Do not recognize asm,inline or typeof as a keyword (-fno-asm)"), groupName, true, true, false, "-fno-asm");
//    addOption(CC_CMD_OPT_TRADITIONAL_CPP, QObject::tr("Imitate traditional C preprocessors (-traditional-cpp)"), groupName, true, true, false, "-traditional-cpp");

    groupName = QObject::tr("Code Generation");
    // Optimization
    sl.clear();
    sl.append(QPair<QString,QString>("Low (-O1)","1"));
    sl.append(QPair<QString,QString>("Med (-O2)","2"));
    sl.append(QPair<QString,QString>("High (-O3)","3"));
    sl.append(QPair<QString,QString>("Highest (-Ofast)","fast"));
    sl.append(QPair<QString,QString>("Size (-Os)","s"));
    sl.append(QPair<QString,QString>("Debug (-Og)","g"));
    addOption(CC_CMD_OPT_OPTIMIZE, QObject::tr("Optimization level (-Ox)"), groupName, true, true, false, "-O", sl);

    // C++ Language Standards
    sl.clear();
    sl.append(QPair<QString,QString>("ISO C++","c++98"));
    sl.append(QPair<QString,QString>("ISO C++11","c++11"));
    sl.append(QPair<QString,QString>("ISO C++14","c++14"));
    sl.append(QPair<QString,QString>("ISO C++17","c++17"));
    sl.append(QPair<QString,QString>("ISO C++20","c++2a"));
    sl.append(QPair<QString,QString>("ISO C++23","c++2b"));
    sl.append(QPair<QString,QString>("GNU C++","gnu++98"));
    sl.append(QPair<QString,QString>("GNU C++11","gnu++11"));
    sl.append(QPair<QString,QString>("GNU C++14","gnu++14"));
    sl.append(QPair<QString,QString>("GNU C++17","gnu++17"));
    sl.append(QPair<QString,QString>("GNU C++20","gnu++2a"));
    sl.append(QPair<QString,QString>("GNU C++23","gnu++2b"));
    addOption(CC_CMD_OPT_STD, QObject::tr("C++ Language standard (-std)"), groupName, false, true, false, "-std=", sl);

    sl.clear();
    sl.append(QPair<QString,QString>("ISO C90","c90"));
    sl.append(QPair<QString,QString>("ISO C99","c99"));
    sl.append(QPair<QString,QString>("ISO C11","c11"));
    sl.append(QPair<QString,QString>("ISO C17","c17"));
    sl.append(QPair<QString,QString>("GNU C90","gnu90"));
    sl.append(QPair<QString,QString>("GNU C99","gnu99"));
    sl.append(QPair<QString,QString>("GNU C11","gnu11"));
    sl.append(QPair<QString,QString>("GNU C17","gnu17"));
    addOption(C_CMD_OPT_STD, QObject::tr("C Language standard (-std)"), groupName, true, false, false, "-std=", sl);

    // Optimization for cpu type
//    sl.clear();
//    sl.append(QPair<QString,QString>(QObject::tr("This CPU"),"native"));
//    sl.append(QPair<QString,QString>("i386","i386"));
//    sl.append(QPair<QString,QString>("i486","i486"));
//    sl.append(QPair<QString,QString>("i586","i586"));
//    sl.append(QPair<QString,QString>("i686","i686"));
//    sl.append(QPair<QString,QString>("Pentium","pentium"));
//    sl.append(QPair<QString,QString>("Pentium MMX","pentium-mmx"));
//    sl.append(QPair<QString,QString>("Pentium Pro","pentiumpro"));
//    sl.append(QPair<QString,QString>("Pentium 2","pentium2"));
//    sl.append(QPair<QString,QString>("Pentium 3","pentium3"));
//    sl.append(QPair<QString,QString>("Pentium 4","pentium4"));
//    sl.append(QPair<QString,QString>("Conroe","core2"));
//    sl.append(QPair<QString,QString>("Nehalem","corei7"));
//    sl.append(QPair<QString,QString>("Sandy","corei7-avx"));
//    sl.append(QPair<QString,QString>("K6","k6"));
//    sl.append(QPair<QString,QString>("K6-2","k6-2"));
//    sl.append(QPair<QString,QString>("K6-3","k6-3"));
//    sl.append(QPair<QString,QString>("Athlon","athlon"));
//    sl.append(QPair<QString,QString>("Athlon Tbird","athlon-tbird"));
//    sl.append(QPair<QString,QString>("Athlon 4","athlon-4"));
//    sl.append(QPair<QString,QString>("Athlon XP","athlon-xp"));
//    sl.append(QPair<QString,QString>("Athlon MP","athlon-mp"));
//    sl.append(QPair<QString,QString>("K8","k8"));
//    sl.append(QPair<QString,QString>("K8 Rev.E","k8-sse3"));
//    sl.append(QPair<QString,QString>("K10","barcelona"));
//    sl.append(QPair<QString,QString>("Bulldozer","bdver1"));
//    addOption(CC_CMD_OPT_ARCH, QObject::tr("Optimize for the following machine (-march)"), groupName, true, true, false, "-march=", sl);
//    addOption(CC_CMD_OPT_TUNE, QObject::tr("Optimize less, while maintaining full compatibility (-tune)"), groupName, true, true, false, "-mtune=", sl);

    // Enable use of the specific instructions
    sl.clear();
    sl.append(QPair<QString,QString>("MMX","mmx"));
    sl.append(QPair<QString,QString>("3D Now","3dnow"));
    sl.append(QPair<QString,QString>("SSE","sse"));
    sl.append(QPair<QString,QString>("SSE2","sse2"));
    sl.append(QPair<QString,QString>("SSE3","sse3"));
    sl.append(QPair<QString,QString>("SSSE3","ssse3"));
    sl.append(QPair<QString,QString>("SSE4","sse4"));
    sl.append(QPair<QString,QString>("SSE4A","sse4a"));
    sl.append(QPair<QString,QString>("SSE4.1","sse4.1"));
    sl.append(QPair<QString,QString>("SSE4.2","sse4.2"));
    sl.append(QPair<QString,QString>("AVX","avx"));
    sl.append(QPair<QString,QString>("AVX2","avx2"));
    sl.append(QPair<QString,QString>("FMA4","fma4"));
    sl.append(QPair<QString,QString>("XOP","xop"));
    sl.append(QPair<QString,QString>("AES","aes"));
    addOption(CC_CMD_OPT_INSTRUCTION,QObject::tr("Enable use of specific instructions (-mx)"), groupName, true, true, false, "-m", sl);

    // 32bit/64bit
    sl.clear();
    sl.append(QPair<QString,QString>("32bit","32"));
    sl.append(QPair<QString,QString>("64bit","64"));
    addOption(CC_CMD_OPT_POINTER_SIZE, QObject::tr("Compile with the following pointer size (-mx)"), groupName, true, true, true, "-m", sl);

    addOption(CC_CMD_OPT_DEBUG_INFO, QObject::tr("Generate debugging information (-g3)"), groupName, true, true, false, "-g3");
    addOption(CC_CMD_OPT_PROFILE_INFO, QObject::tr("Generate profiling info for analysis (-pg)"), groupName, true, true, true, "-pg");
    addOption(CC_CMD_OPT_SYNTAX_ONLY, QObject::tr("Only check the code for syntax errors (-fsyntax-only)"), groupName, true, true, false, "-fsyntax-only");

    // Warnings
    groupName = QObject::tr("Warnings");
    addOption(CC_CMD_OPT_INHIBIT_ALL_WARNING, QObject::tr("Inhibit all warning messages (-w)"), groupName, true, true, false, "-w");
    addOption(CC_CMD_OPT_WARNING_ALL,QObject::tr("Show most warnings (-Wall)"), groupName, true, true, false, "-Wall");
    addOption(CC_CMD_OPT_WARNING_EXTRA,QObject::tr("Show some more warnings (-Wextra)"), groupName, true, true, false, "-Wextra");
    addOption(CC_CMD_OPT_CHECK_ISO_CONFORMANCE, QObject::tr("Check ISO C/C++ conformance (-pedantic)"), groupName, true, true, false, "-pedantic");
    addOption(CC_CMD_OPT_WARNING_AS_ERROR, QObject::tr("Make all warnings into errors (-Werror)"), groupName, true, true, false, "-Werror");
    addOption(CC_CMD_OPT_ABORT_ON_ERROR , QObject::tr("Abort compilation on first error (-Wfatal-errors)"), groupName, true, true, false, "-Wfatal-errors");
    sl.clear();
    sl.append(QPair<QString,QString>("Normal"," "));
    sl.append(QPair<QString,QString>("Strong","-strong"));
    sl.append(QPair<QString,QString>("All","-all"));
    addOption(CC_CMD_OPT_STACK_PROTECTOR , QObject::tr("Check for stack smashing attacks (-fstack-protector)"), groupName, false, false, true, "-fstack-protector",sl);
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    sl.clear();
    sl.append(QPair<QString,QString>("Address","address"));
    sl.append(QPair<QString,QString>("Thread","thread"));
    sl.append(QPair<QString,QString>("Leak","leak"));
    sl.append(QPair<QString,QString>("Undefined","undefined"));
    addOption(CC_CMD_OPT_ADDRESS_SANITIZER , QObject::tr("Enable Sanitizer (-fsanitize=)"), groupName, true, true, true, "-fsanitize=",sl);
#endif

    // Output
    //groupName = QObject::tr("Output");
    //addOption(CC_CMD_OPT_VERBOSE_ASM, QObject::tr("Put comments in generated assembly code (-fverbose-asm)"), groupName, true, true, false, "-fverbose-asm");
    //addOption(CC_CMD_OPT_ONLY_GEN_ASM_CODE, QObject::tr("Do not assemble, compile and generate the assemble code (-S)"), groupName, true, true, false, "-S");
    //addOption(CC_CMD_OPT_STOP_AFTER_PREPROCESSING, QObject::tr("Do not compile, stop after the preprocessing stage (-E)"), groupName, true, true, false, "-E");

    // Linker
    groupName = QObject::tr("Linker");
    addOption(CC_CMD_OPT_USE_PIPE, QObject::tr("Use pipes instead of temporary files during compilation (-pipe)"), groupName, true, true, false, "-pipe");
    //addOption(LINK_CMD_OPT_LINK_OBJC, QObject::tr("Link an Objective C program (-lobjc)"), groupName, false, false, true, "-lobjc");
    addOption(LINK_CMD_OPT_NO_LINK_STDLIB,QObject::tr("Do not use standard system libraries (-nostdlib)"), groupName, false, false, true, "-nostdlib");
    addOption(LINK_CMD_OPT_NO_CONSOLE, QObject::tr("Do not create a console window (-mwindows)"), groupName,false, false, true, "-mwindows");
    addOption(LINK_CMD_OPT_STRIP_EXE, QObject::tr("Strip executable (-s)"), groupName, false, false, true, "-s");
}

CompilerInfoManager::CompilerInfoManager()
{
    mInfos.insert(CompilerType::Clang, std::make_shared<ClangCompilerInfo>());
    mInfos.insert(CompilerType::GCC, std::make_shared<GCCCompilerInfo>());
    mInfos.insert(CompilerType::GCC_UTF8, std::make_shared<GCCUTF8CompilerInfo>());
}

bool CompilerInfoManager::supportSyntaxCheck(CompilerType compilerType)
{
    switch(compilerType) {
    case CompilerType::GCC:
    case CompilerType::GCC_UTF8:
    case CompilerType::Clang:
        return true;
    default:
        return false;
    }
}

PCompilerInfo CompilerInfoManager::getInfo(CompilerType compilerType)
{
    return getInstance()->mInfos.value(compilerType,PCompilerInfo());
}

bool CompilerInfoManager::hasCompilerOption(CompilerType compilerType, const QString &optKey)
{
    PCompilerInfo pInfo = getInfo(compilerType);
    if (!pInfo)
        return false;
    return pInfo->hasCompilerOption(optKey);
}

PCompilerOption CompilerInfoManager::getCompilerOption(CompilerType compilerType, const QString &optKey)
{
    PCompilerInfo pInfo = getInfo(compilerType);
    if (!pInfo)
        return PCompilerOption();
    return pInfo->getCompilerOption(optKey);
}

QList<PCompilerOption> CompilerInfoManager::getCompilerOptions(CompilerType compilerType)
{
    PCompilerInfo pInfo = getInfo(compilerType);
    if (!pInfo)
        return QList<PCompilerOption>();
    return pInfo->compilerOptions();
}

bool CompilerInfoManager::supportCovertingCharset(CompilerType compilerType)
{
    PCompilerInfo pInfo = getInfo(compilerType);
    if (!pInfo)
        return false;
    return pInfo->supportConvertingCharset();
}

bool CompilerInfoManager::forceUTF8InDebugger(CompilerType compilerType)
{
    PCompilerInfo pInfo = getInfo(compilerType);
    if (!pInfo)
        return false;
    return pInfo->forceUTF8InDebugger();
}

PCompilerInfoManager CompilerInfoManager::instance;

PCompilerInfoManager CompilerInfoManager::getInstance()
{
    if (!instance) {
        instance = std::make_shared<CompilerInfoManager>();
    }
    return instance;
}

void CompilerInfoManager::addInfo(CompilerType compilerType, PCompilerInfo info)
{
    getInstance()->mInfos.insert(compilerType,info);
}

ClangCompilerInfo::ClangCompilerInfo():CompilerInfo(COMPILER_CLANG)
{
}

bool ClangCompilerInfo::supportConvertingCharset()
{
    return false;
}

bool ClangCompilerInfo::forceUTF8InDebugger()
{
    return true;
}

bool ClangCompilerInfo::forceUTF8InMakefile()
{
    return false;
}

GCCCompilerInfo::GCCCompilerInfo():CompilerInfo(COMPILER_GCC)
{
}

bool GCCCompilerInfo::supportConvertingCharset()
{
    return true;
}

bool GCCCompilerInfo::forceUTF8InDebugger()
{
    return false;
}

bool GCCCompilerInfo::forceUTF8InMakefile()
{
    return false;
}

GCCUTF8CompilerInfo::GCCUTF8CompilerInfo():CompilerInfo(COMPILER_GCC_UTF8)
{
}

bool GCCUTF8CompilerInfo::supportConvertingCharset()
{
    return true;
}

bool GCCUTF8CompilerInfo::forceUTF8InDebugger()
{
    return true;
}

bool GCCUTF8CompilerInfo::forceUTF8InMakefile()
{
    return true;
}
