#include "compilerinfo.h"
#include <QObject>

CompilerInfo::CompilerInfo(const QString &name):
    mName(name)
{
    prepareCompilerOptions();
}

const CompilerOptionMap &CompilerInfo::compilerOptions() const
{
    return mCompilerOptions;
}

const QString &CompilerInfo::name() const
{
    return mName;
}

PCompilerOption CompilerInfo::getCompilerOption(const QString &key) const
{
    return mCompilerOptions.value(key,PCompilerOption());
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
}

void CompilerInfo::prepareCompilerOptions()
{
    // C options
    QString groupName = QObject::tr("C options");
    addOption(CC_CMD_OPT_ANSI, QObject::tr("Support all ANSI standard C programs (-ansi)"), groupName, true, true, false, "-ansi");
    addOption(CC_CMD_OPT_NO_ASM, QObject::tr("Do not recognize asm,inline or typeof as a keyword (-fno-asm)"), groupName, true, true, false, "-fno-asm");
    addOption(CC_CMD_OPT_TRADITIONAL_CPP, QObject::tr("Imitate traditional C preprocessors (-traditional-cpp)"), groupName, true, true, false, "-traditional-cpp");

    // Optimization for cpu type
    groupName = QObject::tr("Code Generation");
    QList<QPair<QString,QString>> sl;
    sl.append(QPair<QString,QString>(QObject::tr("This CPU"),"native"));
    sl.append(QPair<QString,QString>("i386","i386"));
    sl.append(QPair<QString,QString>("i486","i486"));
    sl.append(QPair<QString,QString>("i586","i586"));
    sl.append(QPair<QString,QString>("i686","i686"));
    sl.append(QPair<QString,QString>("Pentium","pentium"));
    sl.append(QPair<QString,QString>("Pentium MMX","pentium-mmx"));
    sl.append(QPair<QString,QString>("Pentium Pro","pentiumpro"));
    sl.append(QPair<QString,QString>("Pentium 2","pentium2"));
    sl.append(QPair<QString,QString>("Pentium 3","pentium3"));
    sl.append(QPair<QString,QString>("Pentium 4","pentium4"));
    sl.append(QPair<QString,QString>("Conroe","core2"));
    sl.append(QPair<QString,QString>("Nehalem","corei7"));
    sl.append(QPair<QString,QString>("Sandy","corei7-avx"));
    sl.append(QPair<QString,QString>("K6","k6"));
    sl.append(QPair<QString,QString>("K6-2","k6-2"));
    sl.append(QPair<QString,QString>("K6-3","k6-3"));
    sl.append(QPair<QString,QString>("Athlon","athlon"));
    sl.append(QPair<QString,QString>("Athlon Tbird","athlon-tbird"));
    sl.append(QPair<QString,QString>("Athlon 4","athlon-4"));
    sl.append(QPair<QString,QString>("Athlon XP","athlon-xp"));
    sl.append(QPair<QString,QString>("Athlon MP","athlon-mp"));
    sl.append(QPair<QString,QString>("K8","k8"));
    sl.append(QPair<QString,QString>("K8 Rev.E","k8-sse3"));
    sl.append(QPair<QString,QString>("K10","barcelona"));
    sl.append(QPair<QString,QString>("Bulldozer","bdver1"));
    addOption("gcc_cmd_opt_arch", QObject::tr("Optimize for the following machine (-march)"), groupName, true, true, false, "-march=", sl);
    addOption("gcc_cmd_opt_tune", QObject::tr("Optimize less, while maintaining full compatibility (-tune)"), groupName, true, true, false, "-mtune=", sl);

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
    addOption("gcc_cmd_opt_instruction",QObject::tr("Enable use of specific instructions (-mx)"), groupName, true, true, false, "-m", sl);

    // Optimization
    sl.clear();
    sl.append(QPair<QString,QString>("Low","1"));
    sl.append(QPair<QString,QString>("Med","2"));
    sl.append(QPair<QString,QString>("High","3"));
    sl.append(QPair<QString,QString>("Highest (fast)","fast"));
    sl.append(QPair<QString,QString>("Size (s)","s"));
    sl.append(QPair<QString,QString>("Debug (g)","g"));
    addOption("gcc_cmd_opt_optimize", QObject::tr("Optimization level (-Ox)"), groupName, true, true, false, "-O", sl);

    // 32bit/64bit
    sl.clear();
    sl.append(QPair<QString,QString>("32bit","32"));
    sl.append(QPair<QString,QString>("64bit","64"));
    addOption("gcc_cmd_opt_pointer_size", QObject::tr("Compile with the following pointer size (-mx)"), groupName, true, true, true, "-m", sl);

    // Language Standards
    sl.clear();
    sl.append(QPair<QString,QString>("ISO C90","c90"));
    sl.append(QPair<QString,QString>("ISO C99","c99"));
    sl.append(QPair<QString,QString>("ISO C11","c11"));
    sl.append(QPair<QString,QString>("ISO C17","c17"));
    sl.append(QPair<QString,QString>("ISO C++","c++98"));
    sl.append(QPair<QString,QString>("ISO C++11","c++11"));
    sl.append(QPair<QString,QString>("ISO C++14","c++14"));
    sl.append(QPair<QString,QString>("ISO C++17","c++17"));
    sl.append(QPair<QString,QString>("ISO C++20","c++2a"));
    sl.append(QPair<QString,QString>("GNU C90","gnu90"));
    sl.append(QPair<QString,QString>("GNU C99","gnu99"));
    sl.append(QPair<QString,QString>("GNU C11","gnu11"));
    sl.append(QPair<QString,QString>("GNU C17","gnu17"));
    sl.append(QPair<QString,QString>("GNU C++","gnu++98"));
    sl.append(QPair<QString,QString>("GNU C++11","gnu++11"));
    sl.append(QPair<QString,QString>("GNU C++14","gnu++14"));
    sl.append(QPair<QString,QString>("GNU C++17","gnu++17"));
    sl.append(QPair<QString,QString>("GNU C++20","gnu++2a"));
    addOption("gcc_cmd_opt_std", QObject::tr("Language standard (-std)"), groupName, true, true, false, "-std=", sl);

    // Warnings
    groupName = QObject::tr("Warnings");
    addOption("gcc_cmd_opt_inhibit_all_warning", QObject::tr("Inhibit all warning messages (-w)"), groupName, true, true, false, "-w");
    addOption("gcc_cmd_opt_warning_all",QObject::tr("Show most warnings (-Wall)"), groupName, true, true, false, "-Wall");
    addOption("gcc_cmd_opt_warning_extra",QObject::tr("Show some more warnings (-Wextra)"), groupName, true, true, false, "-Wextra");
    addOption("gcc_cmd_opt_check_iso_conformance", QObject::tr("Check ISO C/C++/C++0x conformance (-pedantic)"), groupName, true, true, false, "-pedantic");
    addOption("gcc_cmd_opt_syntax_only", QObject::tr("Only check the code for syntax errors (-fsyntax-only)"), groupName, true, true, false, "-fsyntax-only");
    addOption("gcc_cmd_opt_warning_as_error", QObject::tr("Make all warnings into errors (-Werror)"), groupName, true, true, false, "-Werror");
    addOption("gcc_cmd_opt_abort_on_error", QObject::tr("Abort compilation on first error (-Wfatal-errors)"), groupName, true, true, false, "-Wfatal-errors");

    // Profile
    groupName = QObject::tr("Profile");
    addOption("gcc_cmd_opt_profile_info",QObject::tr("Generate profiling info for analysis (-pg)"), groupName, true, true, true, "-pg");

    // Linker
    groupName = QObject::tr("Linker");
    addOption("linker_cmd_opt_link_objc", QObject::tr("Link an Objective C program (-lobjc)"), groupName, false, false, true, "-lobjc");
    addOption("linker_cmd_opt_no_link_stdlib",QObject::tr("Do not use standard system libraries (-nostdlib)"), groupName, false, false, true, "-nostdlib");
    addOption("linker_cmd_opt_no_console", QObject::tr("Do not create a console window (-mwindows)"), groupName,false, false, true, "-mwindows");
    addOption("linker_cmd_opt_strip_exe", QObject::tr("Strip executable (-s)"), groupName, false, false, true, "-s");
    addOption("cc_cmd_opt_debug_info", QObject::tr("Generate debugging information (-g3)"), groupName, true, true, false, "-g3");

    // Output
    groupName = QObject::tr("Output");
    addOption("cc_cmd_opt_verbose_asm", QObject::tr("Put comments in generated assembly code (-fverbose-asm)"), groupName, true, true, false, "-fverbose-asm");
    addOption("cc_cmd_opt_only_gen_asm_code", QObject::tr("Do not assemble, compile and generate the assemble code (-S)"), groupName, true, true, false, "-S");
    addOption("cc_cmd_opt_use_pipe", QObject::tr("Use pipes instead of temporary files during compilation (-pipe)"), groupName, true, true, false, "-pipe");

}

PCompilerInfo CompilerInfoManager::getInfo(const QString &compilerType)
{
    return getInstance()->mInfos.value(compilerType,PCompilerInfo());
}

bool CompilerInfoManager::hasCompilerOption(const QString &compilerType, const QString &optKey)
{
    PCompilerInfo pInfo = getInfo(compilerType);
    if (!pInfo)
        return false;
    return pInfo->compilerOptions().contains(optKey);
}

PCompilerOption CompilerInfoManager::getCompilerOption(const QString &compilerType, const QString &optKey)
{
    PCompilerInfo pInfo = getInfo(compilerType);
    if (!pInfo)
        return PCompilerOption();
    return pInfo->compilerOptions().value(optKey,PCompilerOption());
}

CompilerOptionMap CompilerInfoManager::getCompilerOptions(const QString &compilerType)
{
    PCompilerInfo pInfo = getInfo(compilerType);
    if (!pInfo)
        return CompilerOptionMap();
    return pInfo->compilerOptions();
}

bool CompilerInfoManager::supportCovertingCharset(const QString &compilerType)
{
    PCompilerInfo pInfo = getInfo(compilerType);
    if (!pInfo)
        return false;
    return pInfo->supportConvertingCharset();
}

PCompilerInfoManager CompilerInfoManager::getInstance()
{
    if (!instance) {
        instance = std::make_shared<CompilerInfoManager>();
    }
    return instance;
}

void CompilerInfoManager::addInfo(const QString &name, PCompilerInfo info)
{
    getInstance()->mInfos.insert(name,info);
}

ClangCompilerInfo::ClangCompilerInfo():CompilerInfo(COMPILER_CLANG)
{

}

bool ClangCompilerInfo::supportConvertingCharset()
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
