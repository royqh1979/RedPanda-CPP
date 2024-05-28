#ifndef COMPILERINFO_H
#define COMPILERINFO_H

#include <QString>
#include <QMap>
#include <memory>
#define COMPILER_CLANG "Clang"
#define COMPILER_GCC "GCC"
#define COMPILER_GCC_UTF8 "GCC_UTF8"
#ifdef ENABLE_SDCC
#define COMPILER_SDCC "SDCC"
#endif

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
#define LINK_CMD_OPT_STACK_SIZE "link_cmd_opt_stack_size"
#define CC_CMD_OPT_DEBUG_INFO "cc_cmd_opt_debug_info"
#define CC_CMD_OPT_ADDRESS_SANITIZER "cc_cmd_opt_address_sanitizer"
#define CC_CMD_OPT_STACK_PROTECTOR "cc_cmd_opt_stack_protector"

#define CC_CMD_OPT_VERBOSE_ASM "cc_cmd_opt_verbose_asm"
#define CC_CMD_OPT_ONLY_GEN_ASM_CODE "cc_cmd_opt_only_gen_asm_code"
#define CC_CMD_OPT_STOP_AFTER_PREPROCESSING "cc_cmd_opt_stop_after_preprocessing"
#define CC_CMD_OPT_USE_PIPE "cc_cmd_opt_use_pipe"

#ifdef ENABLE_SDCC
#define SDCC_CMD_OPT_PROCESSOR "sdcc_cmd_opt_processor"
#define SDCC_CMD_OPT_STD "sdcc_cmd_opt_std"
#define SDCC_OPT_MEMORY_MODEL "sdcc_opt_memory_model"
#define SDCC_OPT_XSTACK "sdcc_opt_xstack"
#define SDCC_OPT_XRAM_MOVC "sdcc_opt_xram_movc"
#define SDCC_OPT_ACALL_AJMP "sdcc_opt_acall_ajmp"
#define SDCC_OPT_NO_XINIT_OPT "sdcc_opt_no_xinit_opt"

#define SDCC_OPT_NOSTARTUP "sdcc_opt_nostartup"
#define SDCC_OPT_IRAM_SIZE "sdcc_opt_iram_size"
#define SDCC_OPT_XRAM_SIZE "sdcc_opt_xram_size"
#define SDCC_OPT_XRAM_LOC "sdcc_opt_xram_loc"
#define SDCC_OPT_XSTACK_LOC "sdcc_opt_xstack_loc"
#define SDCC_OPT_CODE_LOC "sdcc_opt_code_loc"
#define SDCC_OPT_CODE_SIZE "sdcc_opt_code_size"
#define SDCC_OPT_STACK_LOC "sdcc_opt_stack_loc"
#define SDCC_OPT_DATA_LOC "sdcc_opt_data_loc"
#define SDCC_OPT_NOSTARTUP "sdcc_opt_nostartup"
#endif

#define COMPILER_OPTION_ON "on"
#define COMPILER_OPTION_OFF ""

enum class CompilerType {
    GCC,
    GCC_UTF8,
    Clang,
#ifdef ENABLE_SDCC
    SDCC,
#endif
    Unknown
};

enum class CompilerOptionType {
    Checkbox,
    Choice,
    Input,
    Number
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
    CompilerOptionType type;
    CompileOptionChoiceList choices; // replaces "Yes/No" standard choices (max 30 different choices)
    /* for spin control */
    int scale; //Scale
    QString suffix;  //suffix
    int defaultValue;
    int minValue;
    int maxValue;
} CompilerOption;

using PCompilerOption = std::shared_ptr<CompilerOption>;

using CompilerOptionMap=QMap<QString,PCompilerOption>;

class CompilerInfo
{
public:
    CompilerInfo(const QString& name);
    CompilerInfo(const CompilerInfo&)=delete;
    CompilerInfo& operator=(const CompilerInfo&)=delete;

    const QList<PCompilerOption> &compilerOptions() const;
    const QString &name() const;
    PCompilerOption getCompilerOption(const QString& key) const;
    bool hasCompilerOption(const QString& key) const;
    void init();

    virtual bool supportConvertingCharset()=0;
    virtual bool forceUTF8InDebugger()=0;
    virtual bool forceUTF8InMakefile()=0;
    virtual bool supportStaticLink()=0;
    virtual bool supportSyntaxCheck();
protected:
    PCompilerOption addOption(const QString& key,
                   const QString& name,
                   const QString section,
                   bool isC,
                   bool isCpp,
                   bool isLinker,
                   const QString& setting,
                   CompilerOptionType type = CompilerOptionType::Checkbox,
                   const CompileOptionChoiceList& choices = CompileOptionChoiceList());
    PCompilerOption addNumberOption(const QString& key,
                   const QString& name,
                   const QString section,
                   bool isC,
                   bool isCpp,
                   bool isLinker,
                   const QString& setting,
                   const QString& suffix,
                   int scale,
                   int defaultValue,
                   int minValue,
                   int maxValue
                    );
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
    static bool supportStaticLink(CompilerType compilerType);
    static bool supportSyntaxCheck(CompilerType compilerType);
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
    bool supportStaticLink() override;
};

class GCCCompilerInfo: public CompilerInfo{
public:
    GCCCompilerInfo();
    bool supportConvertingCharset() override;
    bool forceUTF8InDebugger() override;
    bool forceUTF8InMakefile() override;
    bool supportStaticLink() override;
};

class GCCUTF8CompilerInfo: public CompilerInfo{
public:
    GCCUTF8CompilerInfo();
    bool supportConvertingCharset() override;
    bool forceUTF8InDebugger() override;
    bool forceUTF8InMakefile() override;
    bool supportStaticLink() override;
};

#ifdef ENABLE_SDCC
class SDCCCompilerInfo: public CompilerInfo{
public:
    SDCCCompilerInfo();
    bool supportConvertingCharset() override;
    bool forceUTF8InDebugger() override;
    bool forceUTF8InMakefile() override;
    bool supportStaticLink() override;
    bool supportSyntaxCheck() override;
protected:
    void prepareCompilerOptions() override;
};
#endif


#endif // COMPILERINFO_H
