#ifndef COMPILERINFO_H
#define COMPILERINFO_H

#include <QString>
#include <QMap>
#include <memory>
#include <QMutex>
#define COMPILER_CLANG "Clang"
#define COMPILER_GCC "GCC"
#define COMPILER_GCC_UTF8 "GCC_UTF8"

#define C_CMD_OPT_STD "c_cmd_opt_std"

#define CC_CMD_OPT_ANSI "cc_cmd_opt_ansi"
#define CC_CMD_OPT_NO_ASM "cc_cmd_opt_no_asm"
#define CC_CMD_OPT_TRADITIONAL_CPP "cc_cmd_opt_traditional_cpp"

#define CC_CMD_OPT_ARCH "cc_cmd_opt_arch"
#define CC_CMD_OPT_TUNE "cc_cmd_opt_tune"
#define CC_CMD_OPT_INSTRUCTION "cc_cmd_opt_instruction"
#define CC_CMD_OPT_OPTIMIZE "cc_cmd_opt_optimize"
#define CC_CMD_OPT_POINTER_SIZE "cc_cmd_opt_pointer_size"
#define CC_CMD_OPT_STD "cc_cmd_opt_std"

#define CC_CMD_OPT_INHIBIT_ALL_WARNING "cc_cmd_opt_inhibit_all_warning"
#define CC_CMD_OPT_WARNING_ALL "cc_cmd_opt_warning_all"
#define CC_CMD_OPT_WARNING_EXTRA "cc_cmd_opt_warning_extra"
#define CC_CMD_OPT_CHECK_ISO_CONFORMANCE "cc_cmd_opt_check_iso_conformance"
#define CC_CMD_OPT_SYNTAX_ONLY "cc_cmd_opt_syntax_only"
#define CC_CMD_OPT_WARNING_AS_ERROR "cc_cmd_opt_warning_as_error"
#define CC_CMD_OPT_ABORT_ON_ERROR "cc_cmd_opt_abort_on_error"

#define CC_CMD_OPT_PROFILE_INFO "cc_cmd_opt_profile_info"

#define LINK_CMD_OPT_LINK_OBJC "link_cmd_opt_link_objc"
#define LINK_CMD_OPT_NO_LINK_STDLIB "link_cmd_opt_no_link_stdlib"
#define LINK_CMD_OPT_NO_CONSOLE "link_cmd_opt_no_console"
#define LINK_CMD_OPT_STRIP_EXE "link_cmd_opt_strip_exe"
#define CC_CMD_OPT_DEBUG_INFO "cc_cmd_opt_debug_info"

#define CC_CMD_OPT_VERBOSE_ASM "cc_cmd_opt_verbose_asm"
#define CC_CMD_OPT_ONLY_GEN_ASM_CODE "cc_cmd_opt_only_gen_asm_code"
#define CC_CMD_OPT_STOP_AFTER_PREPROCESSING "cc_cmd_opt_stop_after_preprocessing"
#define CC_CMD_OPT_USE_PIPE "cc_cmd_opt_use_pipe"

#define COMPILER_OPTION_ON "on"

enum class CompilerSetType {
    RELEASE,
    DEBUG,
    PROFILING
};

enum class CompilerType {
    GCC,
    GCC_UTF8,
    Clang
};

using CompileOptionChoiceList = QList<QPair<QString,QString>>;

typedef struct {
    QString key;
    QString name; // "Generate debugging info"
    QString section; // "C options"
    bool isC;
    bool isCpp; // True (C++ option?) - can be both C and C++ option...
    bool isLinker; // Is it a linker param
    QString setting; // "-g3"
    CompileOptionChoiceList choices; // replaces "Yes/No" standard choices (max 30 different choices)
} CompilerOption;

using PCompilerOption = std::shared_ptr<CompilerOption>;

using CompilerOptionMap=QMap<QString,PCompilerOption>;

class CompilerInfo
{
public:
    CompilerInfo(const QString& name);
    const QList<PCompilerOption> &compilerOptions() const;
    const QString &name() const;
    PCompilerOption getCompilerOption(const QString& key) const;
    bool hasCompilerOption(const QString& key) const;

    virtual bool supportConvertingCharset()=0;
    virtual bool forceUTF8InDebugger()=0;
    virtual bool forceUTF8InMakefile()=0;
protected:
    void addOption(const QString& key,
                   const QString& name,
                   const QString section,
                   bool isC,
                   bool isCpp,
                   bool isLinker,
                   const QString& setting,
                   const CompileOptionChoiceList& choices = CompileOptionChoiceList());
    void init();
    virtual void prepareCompilerOptions();
protected:
    CompilerOptionMap mCompilerOptions;
    QList<PCompilerOption> mCompilerOptionList;
    QString mName;
};

using PCompilerInfo = std::shared_ptr<CompilerInfo>;

class CompilerInfoManager;
using PCompilerInfoManager = std::shared_ptr<CompilerInfoManager>;

class CompilerInfoManager {
public:
    CompilerInfoManager();
    static PCompilerInfo getInfo(CompilerType compilerType);
    static bool hasCompilerOption(CompilerType compilerType, const QString& optKey);
    static PCompilerOption getCompilerOption(CompilerType compilerType, const QString& optKey);
    static QList<PCompilerOption> getCompilerOptions(CompilerType compilerType);
    static bool supportCovertingCharset(CompilerType compilerType);
    static bool forceUTF8InDebugger(CompilerType compilerType);
    static PCompilerInfoManager getInstance();
    static void addInfo(CompilerType compilerType, PCompilerInfo info);
private:
    static PCompilerInfoManager instance;
    QMap<CompilerType,PCompilerInfo> mInfos;
};

class ClangCompilerInfo: public CompilerInfo{
public:
    ClangCompilerInfo();
    bool supportConvertingCharset() override;
    bool forceUTF8InDebugger() override;
    bool forceUTF8InMakefile() override;
};

class GCCCompilerInfo: public CompilerInfo{
public:
    GCCCompilerInfo();
    bool supportConvertingCharset() override;
    bool forceUTF8InDebugger() override;
    bool forceUTF8InMakefile() override;
};

class GCCUTF8CompilerInfo: public CompilerInfo{
public:
    GCCUTF8CompilerInfo();
    bool supportConvertingCharset() override;
    bool forceUTF8InDebugger() override;
    bool forceUTF8InMakefile() override;
};



#endif // COMPILERINFO_H
