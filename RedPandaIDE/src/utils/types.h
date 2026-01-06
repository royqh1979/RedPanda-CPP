#ifndef UTILS_TYPES_H
#define UTILS_TYPES_H
#include <QString>
#include <functional>

enum class FileType{
    None,
    ATTASM, // deprecated: AT&T Style GNU assembler source file (.s)
    INTELASM, // deprecated: Intel Style GNU assembler source file (.s)
    LUA, // lua file (.lua)
    CSource, // c source file (.c)
    CppSource, // c++ source file (.cpp)
    CCppHeader, // c header (.h)
    PreprocessedSource, //(.p)
    GIMPLE, // gcc gimple file (.gimple)
    WindowsResourceSource, // resource source (.res)
    Project, //Red Panda C++ Project (.dev)
    Text, // text file
    FragmentShader,
    VerticeShader,
    ModuleDef, // Windows Module Definition
    MakeFile,
    Other, // any others
    NASM, // NASM Files
    GAS, //GAS Files
};

enum class SearchFileScope {
    currentFile,
    wholeProject,
    openedFiles,
    Folder
};

enum AutoSaveTarget {
    astCurrentFile,
    astAllOpennedFiles,
    astAllProjectFiles
};

enum AutoSaveStrategy {
    assOverwrite,
    assAppendUnixTimestamp,
    assAppendFormatedTimeStamp
};

enum FormatterBraceStyle {
    fbsDefault,
    fbsAllman,
    fbsJava,
    fbsKR,
    fbsStroustrup,
    fbsWitesmith,
    fbsVtk,
    fbsRatliff,
    fbsGNU,
    fbsLinux,
    fbsHorstmann,
    fbs1TBS,
    fbsGoogle,
    fbsMozilla,
    fbsWebkit,
    fbsPico,
    fbsLisp
};

enum FormatterOperatorAlign {
    foaNone,
    foaType,
    foaMiddle,
    foaName
};

enum FormatterIndentType {
    fitSpace,
    fitTab
};

enum class SplitProcessCommandQuoteType {
    None,
    Single,
    Double
};

enum class ProblemCaseValidateType {
    Exact,
    IgnoreLeadingTrailingSpaces,
    IgnoreSpaces
};

using LoggerFunc = std::function<void (const QString&)>;

#endif // UTILS_TYPES_H
