#include "utils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

QStringList CppDirectives;
QStringList JavadocTags;
QMap<QString,SkipType> CppKeywords;
QSet<QString> CppTypeKeywords;
QSet<QString> STLPointers;
QSet<QString> STLContainers;
QSet<QString> STLElementMethods;

void initParser()
{
    // skip itself
    CppKeywords.insert("and",SkipType::skItself);
    CppKeywords.insert("and_eq",SkipType::skItself);
    CppKeywords.insert("bitand",SkipType::skItself);
    CppKeywords.insert("bitor",SkipType::skItself);
    CppKeywords.insert("break",SkipType::skItself);
    CppKeywords.insert("compl",SkipType::skItself);
    CppKeywords.insert("constexpr",SkipType::skItself);
    CppKeywords.insert("const_cast",SkipType::skItself);
    CppKeywords.insert("continue",SkipType::skItself);
    CppKeywords.insert("dynamic_cast",SkipType::skItself);
    CppKeywords.insert("else",SkipType::skItself);
    CppKeywords.insert("explicit",SkipType::skItself);
    CppKeywords.insert("export",SkipType::skItself);
    CppKeywords.insert("false",SkipType::skItself);
    //CppKeywords.insert("for",SkipType::skItself);
    CppKeywords.insert("mutable",SkipType::skItself);
    CppKeywords.insert("noexcept",SkipType::skItself);
    CppKeywords.insert("not",SkipType::skItself);
    CppKeywords.insert("not_eq",SkipType::skItself);
    CppKeywords.insert("nullptr",SkipType::skItself);
    CppKeywords.insert("or",SkipType::skItself);
    CppKeywords.insert("or_eq",SkipType::skItself);
    CppKeywords.insert("register",SkipType::skItself);
    CppKeywords.insert("reinterpret_cast",SkipType::skItself);
    CppKeywords.insert("static_assert",SkipType::skItself);
    CppKeywords.insert("static_cast",SkipType::skItself);
    CppKeywords.insert("template",SkipType::skItself);
    CppKeywords.insert("this",SkipType::skItself);
    CppKeywords.insert("thread_local",SkipType::skItself);
    CppKeywords.insert("true",SkipType::skItself);
    CppKeywords.insert("typename",SkipType::skItself);
    CppKeywords.insert("virtual",SkipType::skItself);
    CppKeywords.insert("volatile",SkipType::skItself);
    CppKeywords.insert("xor",SkipType::skItself);
    CppKeywords.insert("xor_eq",SkipType::skItself);


    //CppKeywords.insert("catch",SkipType::skItself);
    CppKeywords.insert("do",SkipType::skItself);
    CppKeywords.insert("try",SkipType::skItself);

    // Skip to ;
    CppKeywords.insert("delete",SkipType::skToSemicolon);
    CppKeywords.insert("delete[]",SkipType::skToSemicolon);
    CppKeywords.insert("goto",SkipType::skToSemicolon);
    CppKeywords.insert("new",SkipType::skToSemicolon);
    CppKeywords.insert("return",SkipType::skToSemicolon);
    CppKeywords.insert("throw",SkipType::skToSemicolon);
  //  CppKeywords.insert("using",SkipType::skToSemicolon); //won't use it

    // Skip to :
    CppKeywords.insert("case",SkipType::skToColon);
    CppKeywords.insert("default",SkipType::skToColon);

    // Skip to )
    CppKeywords.insert("__attribute__",SkipType::skToRightParenthesis);
    CppKeywords.insert("alignas",SkipType::skToRightParenthesis);  // not right
    CppKeywords.insert("alignof",SkipType::skToRightParenthesis);  // not right
    CppKeywords.insert("decltype",SkipType::skToRightParenthesis); // not right
    CppKeywords.insert("if",SkipType::skToRightParenthesis);
    CppKeywords.insert("sizeof",SkipType::skToRightParenthesis);
    CppKeywords.insert("switch",SkipType::skToRightParenthesis);
    CppKeywords.insert("typeid",SkipType::skToRightParenthesis);
    CppKeywords.insert("while",SkipType::skToRightParenthesis);

    // Skip to }
    CppKeywords.insert("asm",SkipType::skToRightBrace);
    //CppKeywords.insert("namespace",SkipType::skToLeftBrace); // won't process it
    // Skip to {

    // wont handle

    //Not supported yet
    CppKeywords.insert("atomic_cancel",SkipType::skNone);
    CppKeywords.insert("atomic_commit",SkipType::skNone);
    CppKeywords.insert("atomic_noexcept",SkipType::skNone);
    CppKeywords.insert("concept",SkipType::skNone);
    CppKeywords.insert("consteval",SkipType::skNone);
    CppKeywords.insert("constinit",SkipType::skNone);
    CppKeywords.insert("co_wait",SkipType::skNone);
    CppKeywords.insert("co_return",SkipType::skNone);
    CppKeywords.insert("co_yield",SkipType::skNone);
    CppKeywords.insert("reflexpr",SkipType::skNone);
    CppKeywords.insert("requires",SkipType::skNone);

    // its a type
    CppKeywords.insert("auto",SkipType::skNone);
    CppKeywords.insert("bool",SkipType::skNone);
    CppKeywords.insert("char",SkipType::skNone);
    CppKeywords.insert("char8_t",SkipType::skNone);
    CppKeywords.insert("char16_t",SkipType::skNone);
    CppKeywords.insert("char32_t",SkipType::skNone);
    CppKeywords.insert("double",SkipType::skNone);
    CppKeywords.insert("float",SkipType::skNone);
    CppKeywords.insert("int",SkipType::skNone);
    CppKeywords.insert("long",SkipType::skNone);
    CppKeywords.insert("short",SkipType::skNone);
    CppKeywords.insert("signed",SkipType::skNone);
    CppKeywords.insert("unsigned",SkipType::skNone);
    CppKeywords.insert("void",SkipType::skNone);
    CppKeywords.insert("wchar_t",SkipType::skNone);

    // type keywords
    CppTypeKeywords.insert("auto");
    CppTypeKeywords.insert("bool");
    CppTypeKeywords.insert("char");
    CppTypeKeywords.insert("char8_t");
    CppTypeKeywords.insert("char16_t");
    CppTypeKeywords.insert("char32_t");
    CppTypeKeywords.insert("double");
    CppTypeKeywords.insert("float");
    CppTypeKeywords.insert("int");
    CppTypeKeywords.insert("long");
    CppTypeKeywords.insert("short");
    //CppTypeKeywords.insert("signed");
    //CppTypeKeywords.insert("unsigned");
    CppTypeKeywords.insert("void");
    CppTypeKeywords.insert("wchar_t");

    // it's part of type info
    CppKeywords.insert("const",SkipType::skNone);
    CppKeywords.insert("extern",SkipType::skNone);
    CppKeywords.insert("inline",SkipType::skNone);

    // handled elsewhere
    CppKeywords.insert("class",SkipType::skNone);
    CppKeywords.insert("enum",SkipType::skNone);
    CppKeywords.insert("friend",SkipType::skNone);
    CppKeywords.insert("operator",SkipType::skNone);
    CppKeywords.insert("private",SkipType::skNone);
    CppKeywords.insert("protected",SkipType::skNone);
    CppKeywords.insert("public",SkipType::skNone);
    CppKeywords.insert("static",SkipType::skNone);
    CppKeywords.insert("struct",SkipType::skNone);
    CppKeywords.insert("typedef",SkipType::skNone);
    CppKeywords.insert("union",SkipType::skNone);
    // namespace
    CppKeywords.insert("namespace",SkipType::skNone);
    CppKeywords.insert("using",SkipType::skNone);

    CppKeywords.insert("for",SkipType::skNone);
    CppKeywords.insert("catch",SkipType::skNone);



    // nullptr is value
    CppKeywords.insert("nullptr",SkipType::skNone);



    //STL Containers
    STLContainers.insert("std::array");
    STLContainers.insert("std::vector");
    STLContainers.insert("std::deque");
    STLContainers.insert("std::forward_list");
    STLContainers.insert("std::list");

    STLContainers.insert("std::set");
    STLContainers.insert("std::map");
    STLContainers.insert("std::multilist");
    STLContainers.insert("std::multimap");

    STLContainers.insert("std::unordered_set");
    STLContainers.insert("std::unordered_map");
    STLContainers.insert("std::unordered_multiset");
    STLContainers.insert("std::unordered_multimap");

    STLContainers.insert("std::stack");
    STLContainers.insert("std::queue");
    STLContainers.insert("std::priority_queue");

    STLContainers.insert("std::span");

    //STL element access methods
    STLElementMethods.insert("at");
    STLElementMethods.insert("back");
    STLElementMethods.insert("front");
    STLElementMethods.insert("top");

    //STL pointers
    STLPointers.insert("std::unique_ptr");
    STLPointers.insert("std::auto_ptr");
    STLPointers.insert("std::shared_ptr");
    STLPointers.insert("std::weak_ptr");
    STLPointers.insert("__gnu_cxx::__normal_iterator");
    STLPointers.insert("std::reverse_iterator");
    STLPointers.insert("std::iterator");

    //C/CPP preprocessor directives
    CppDirectives.append("#include");
    CppDirectives.append("#if");
    CppDirectives.append("#ifdef");
    CppDirectives.append("#ifndef");
    CppDirectives.append("#else");
    CppDirectives.append("#elif");
    CppDirectives.append("#endif");
    CppDirectives.append("#define");
    CppDirectives.append("#error");
    CppDirectives.append("#pragma");
    CppDirectives.append("#line");

    // javadoc tags
    JavadocTags.append("@author");
    JavadocTags.append("@code");
    JavadocTags.append("@docRoot");
    JavadocTags.append("@deprecated");
    JavadocTags.append("@exception");
    JavadocTags.append("@inheritDoc");
    JavadocTags.append("@link");
    JavadocTags.append("@linkplain");
    JavadocTags.append("@literal");
    JavadocTags.append("@param");
    JavadocTags.append("@return");
    JavadocTags.append("@see");
    JavadocTags.append("@serial");
    JavadocTags.append("@serialData");
    JavadocTags.append("@serialField");
    JavadocTags.append("@since");
    JavadocTags.append("@throws");
    JavadocTags.append("@value");
    JavadocTags.append("@version");
}

QString getHeaderFileName(const QString &relativeTo, const QString &line,
                          std::shared_ptr<QStringList>includePaths, std::shared_ptr<QStringList> projectIncludePaths) {
    QString result = "";

    // Handle <>
    int openTokenPos = line.indexOf('<');
    if (openTokenPos >= 0) {
        int closeTokenPos = line.indexOf('>',openTokenPos+1);
        if (closeTokenPos >=0) {
            QString fileName = line.mid(openTokenPos + 1, closeTokenPos - openTokenPos - 1);
            //project settings is preferred
            result = getSystemHeaderFileName(fileName, projectIncludePaths);
            if (result.isEmpty()) {
                result = getSystemHeaderFileName(fileName, includePaths);
            }
        }
    } else {
        // Try ""
        openTokenPos = line.indexOf('"');
        if (openTokenPos >= 0) {
            int closeTokenPos = line.indexOf('"', openTokenPos+1);
            if (closeTokenPos >= 0) {
                QString fileName = line.mid(openTokenPos + 1, closeTokenPos - openTokenPos - 1);
                result = getLocalHeaderFileName(relativeTo, fileName);
                //project settings is preferred
                if (result.isEmpty()) {
                    result = getSystemHeaderFileName(fileName, projectIncludePaths);
                }
                if (result.isEmpty()) {
                    result = getSystemHeaderFileName(fileName, includePaths);
                }
            }
        }
    }
}

QString getLocalHeaderFileName(const QString &relativeTo, const QString &fileName)
{
    QFileInfo relativeFile(relativeTo);
    QDir dir = relativeFile.dir();
    // Search local directory
    if (dir.exists(fileName)) {
        return dir.absoluteFilePath(fileName);
    }
    return "";
}

QString getSystemHeaderFileName(const QString &fileName, std::shared_ptr<QStringList> includePaths)
{
    if (!includePaths)
        return "";

    // Search compiler include directories
    for (QString path:*includePaths) {
        QDir dir(path);
        if (dir.exists(fileName))
            return dir.absoluteFilePath(fileName);
    }
    //not found
    return "";
}
