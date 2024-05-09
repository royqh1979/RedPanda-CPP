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
#include "parserutils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QGlobalStatic>
#include "../utils.h"

QStringList CppDirectives;
QStringList JavadocTags;
QMap<QString,KeywordType> CppKeywords;
#ifdef ENABLE_SDCC
QMap<QString,KeywordType> SDCCKeywords;
QSet<QString> SDCCTypeKeywords;
#endif
QSet<QString> CppControlKeyWords;
QSet<QString> CppTypeKeywords;
QSet<QString> CKeywords;
QSet<QString> STLPointers;
QSet<QString> STLContainers;
QSet<QString> STLMaps;
QSet<QString> STLElementMethods;
QSet<QString> STLIterators;
QSet<QString> MemberOperators;
QSet<QString> IOManipulators;
QSet<QString> AutoTypes;

Q_GLOBAL_STATIC(QSet<QString>,CppHeaderExts)
Q_GLOBAL_STATIC(QSet<QString>,CppSourceExts)

void initParser()
{
    CppHeaderExts->insert("h");
    CppHeaderExts->insert("hpp");
    CppHeaderExts->insert("rh");
    CppHeaderExts->insert("hh");
    CppHeaderExts->insert("hxx");
    CppHeaderExts->insert("inl");
    CppHeaderExts->insert("");

    CppSourceExts->insert("c");
    CppSourceExts->insert("cpp");
    CppSourceExts->insert("cc");
    CppSourceExts->insert("cxx");
    CppSourceExts->insert("c++");
    CppSourceExts->insert("cp");
    // skip itself
    CppKeywords.insert("and",KeywordType::SkipItself);
    CppKeywords.insert("and_eq",KeywordType::SkipItself);
    CppKeywords.insert("bitand",KeywordType::SkipItself);
    CppKeywords.insert("bitor",KeywordType::SkipItself);
    CppKeywords.insert("break",KeywordType::SkipItself);
    CppKeywords.insert("compl",KeywordType::SkipItself);
    CppKeywords.insert("constexpr",KeywordType::SkipItself);
    CppKeywords.insert("const_cast",KeywordType::SkipItself);
    CppKeywords.insert("continue",KeywordType::SkipItself);
    CppKeywords.insert("dynamic_cast",KeywordType::SkipItself);
    CppKeywords.insert("else",KeywordType::SkipItself);
    CppKeywords.insert("explicit",KeywordType::SkipItself);
    CppKeywords.insert("export",KeywordType::SkipItself);
    CppKeywords.insert("false",KeywordType::SkipItself);
    CppKeywords.insert("__extension__",KeywordType::SkipItself);

    //CppKeywords.insert("for",SkipType::skItself);
    CppKeywords.insert("mutable",KeywordType::SkipItself);
    CppKeywords.insert("noexcept",KeywordType::SkipItself);
    CppKeywords.insert("not",KeywordType::SkipItself);
    CppKeywords.insert("not_eq",KeywordType::SkipItself);
    CppKeywords.insert("nullptr",KeywordType::SkipItself);
    CppKeywords.insert("or",KeywordType::SkipItself);
    CppKeywords.insert("or_eq",KeywordType::SkipItself);
    CppKeywords.insert("register",KeywordType::SkipItself);
    CppKeywords.insert("reinterpret_cast",KeywordType::SkipItself);
    CppKeywords.insert("static_cast",KeywordType::SkipItself);
    CppKeywords.insert("template",KeywordType::SkipItself);
    //CppKeywords.insert("this",SkipType::skItself);
    CppKeywords.insert("thread_local",KeywordType::SkipItself);
    CppKeywords.insert("true",KeywordType::SkipItself);
    CppKeywords.insert("typename",KeywordType::SkipItself);
    CppKeywords.insert("virtual",KeywordType::SkipItself);
    CppKeywords.insert("volatile",KeywordType::SkipItself);
    CppKeywords.insert("xor",KeywordType::SkipItself);
    CppKeywords.insert("xor_eq",KeywordType::SkipItself);


    //CppKeywords.insert("catch",SkipType::skItself);
    CppKeywords.insert("do",KeywordType::SkipItself);
    CppKeywords.insert("try",KeywordType::SkipItself);

    // Skip to ;
    CppKeywords.insert("delete",KeywordType::SkipNextSemicolon);
    CppKeywords.insert("delete[]",KeywordType::SkipNextSemicolon);
    CppKeywords.insert("goto",KeywordType::SkipNextSemicolon);
    CppKeywords.insert("new",KeywordType::SkipNextSemicolon);
    CppKeywords.insert("return",KeywordType::SkipNextSemicolon);
    CppKeywords.insert("throw",KeywordType::SkipNextSemicolon);
  //  CppKeywords.insert("using",SkipType::skToSemicolon); //won't use it

    // Skip to :
    CppKeywords.insert("case",KeywordType::SkipNextColon);
    CppKeywords.insert("default",KeywordType::SkipNextColon);

    // Skip to )
    CppKeywords.insert("__attribute__",KeywordType::SkipNextParenthesis);
    CppKeywords.insert("__attribute",KeywordType::SkipNextParenthesis);
    CppKeywords.insert("alignas",KeywordType::SkipNextParenthesis);  // not right
    CppKeywords.insert("alignof",KeywordType::SkipNextParenthesis);  // not right
    CppKeywords.insert("if",KeywordType::SkipNextParenthesis);
    CppKeywords.insert("sizeof",KeywordType::SkipNextParenthesis);
    CppKeywords.insert("switch",KeywordType::SkipNextParenthesis);
    CppKeywords.insert("typeid",KeywordType::SkipNextParenthesis);
    CppKeywords.insert("while",KeywordType::SkipNextParenthesis);
    CppKeywords.insert("static_assert",KeywordType::SkipNextParenthesis);
    CppKeywords.insert("_Pragma",KeywordType::SkipNextParenthesis);

    // Skip to }
    CppKeywords.insert("asm",KeywordType::SkipNextParenthesis);
    CppKeywords.insert("__asm",KeywordType::SkipNextParenthesis);
    // Skip to {

    CppKeywords.insert("requires",KeywordType::Requires);
    CppKeywords.insert("concept",KeywordType::Concept);

    // wont handle

    //Not supported yet
    CppKeywords.insert("atomic_cancel",KeywordType::None);
    CppKeywords.insert("atomic_commit",KeywordType::None);
    CppKeywords.insert("atomic_noexcept",KeywordType::None);
    CppKeywords.insert("consteval",KeywordType::None);
    CppKeywords.insert("constinit",KeywordType::None);
    CppKeywords.insert("co_await",KeywordType::None);
    CppKeywords.insert("co_return",KeywordType::None);
    CppKeywords.insert("co_yield",KeywordType::None);
    CppKeywords.insert("reflexpr",KeywordType::None);
    // its a type
    CppKeywords.insert("auto",KeywordType::None);
    CppKeywords.insert("bool",KeywordType::None);
    CppKeywords.insert("char",KeywordType::None);
    CppKeywords.insert("char8_t",KeywordType::None);
    CppKeywords.insert("char16_t",KeywordType::None);
    CppKeywords.insert("char32_t",KeywordType::None);
    CppKeywords.insert("double",KeywordType::None);
    CppKeywords.insert("float",KeywordType::None);
    CppKeywords.insert("int",KeywordType::None);
    CppKeywords.insert("long",KeywordType::None);
    CppKeywords.insert("short",KeywordType::None);
    CppKeywords.insert("signed",KeywordType::None);
    CppKeywords.insert("unsigned",KeywordType::None);
    CppKeywords.insert("void",KeywordType::None);
    CppKeywords.insert("wchar_t",KeywordType::None);

#ifdef ENABLE_SDCC
    SDCCKeywords.insert("__sfr",KeywordType::None);
    SDCCKeywords.insert("__sfr16",KeywordType::None);
    SDCCKeywords.insert("__sfr32",KeywordType::None);
    SDCCKeywords.insert("__sbit",KeywordType::None);
    SDCCKeywords.insert("__bit",KeywordType::None);
    SDCCKeywords.insert("__data",KeywordType::SkipItself);
    SDCCKeywords.insert("__near",KeywordType::SkipItself);
    SDCCKeywords.insert("__xdata",KeywordType::SkipItself);
    SDCCKeywords.insert("__far",KeywordType::SkipItself);
    SDCCKeywords.insert("__idata",KeywordType::SkipItself);
    SDCCKeywords.insert("__pdata",KeywordType::SkipItself);
    SDCCKeywords.insert("__code",KeywordType::SkipItself);
    SDCCKeywords.insert("__banked",KeywordType::SkipItself);
    SDCCKeywords.insert("__at",KeywordType::SkipNextParenthesis);
    SDCCKeywords.insert("__reentrant",KeywordType::SkipItself);
    SDCCKeywords.insert("__interrupt",KeywordType::SkipItself);
    SDCCKeywords.insert("__using",KeywordType::SkipItself);
    SDCCKeywords.insert("__critical",KeywordType::SkipItself);
    SDCCKeywords.insert("__trap",KeywordType::SkipItself);
    SDCCKeywords.insert("__asm",KeywordType::SkipItself);
    SDCCKeywords.insert("__endasm",KeywordType::SkipItself);
    SDCCKeywords.insert("__naked",KeywordType::SkipItself);

    SDCCTypeKeywords.insert("__sfr");
    SDCCTypeKeywords.insert("__sfr16");
    SDCCTypeKeywords.insert("__sfr32");
    SDCCTypeKeywords.insert("__sbit");
    SDCCTypeKeywords.insert("__bit");
#endif
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
    CppTypeKeywords.insert("signed");
    CppTypeKeywords.insert("unsigned");

    // it's part of type info
    CppKeywords.insert("const",KeywordType::None);
    CppKeywords.insert("extern",KeywordType::Extern);

    CppKeywords.insert("operator",KeywordType::Operator);

    // handled elsewhere
    CppKeywords.insert("static",KeywordType::None);

    //struct/class/union
    CppKeywords.insert("class",KeywordType::Struct);
    CppKeywords.insert("struct",KeywordType::Struct);
    CppKeywords.insert("union",KeywordType::Struct);


    CppKeywords.insert("for",KeywordType::For);
    CppKeywords.insert("catch",KeywordType::Catch);
    CppKeywords.insert("private",KeywordType::Private);
    CppKeywords.insert("public",KeywordType::Public);
    CppKeywords.insert("enum",KeywordType::Enum);
    CppKeywords.insert("namespace",KeywordType::Namespace);
    CppKeywords.insert("inline",KeywordType::Inline);
    CppKeywords.insert("typedef",KeywordType::Typedef);
    CppKeywords.insert("using",KeywordType::Using);
    CppKeywords.insert("protected",KeywordType::Protected);
    CppKeywords.insert("friend",KeywordType::Friend);
    CppKeywords.insert("decltype",KeywordType::DeclType); // not right


    // nullptr is value
    CppKeywords.insert("nullptr",KeywordType::None);

    //C Keywords
    CKeywords.insert("auto");
    CKeywords.insert("break");
    CKeywords.insert("case");
    CKeywords.insert("char");
    CKeywords.insert("const");
    CKeywords.insert("continue");
    CKeywords.insert("default");
    CKeywords.insert("do");
    CKeywords.insert("double");
    CKeywords.insert("else");
    CKeywords.insert("enum");
    CKeywords.insert("extern");
    CKeywords.insert("float");
    CKeywords.insert("for");
    CKeywords.insert("goto");
    CKeywords.insert("if");
    CKeywords.insert("inline");
    CKeywords.insert("int");
    CKeywords.insert("long");
    CKeywords.insert("register");
    CKeywords.insert("restrict");
    CKeywords.insert("return");
    CKeywords.insert("short");
    CKeywords.insert("signed");
    CKeywords.insert("sizeof");
    CKeywords.insert("static");
    CKeywords.insert("struct");
    CKeywords.insert("switch");
    CKeywords.insert("typedef");
    CKeywords.insert("union");
    CKeywords.insert("unsigned");
    CKeywords.insert("void");
    CKeywords.insert("volatile");
    CKeywords.insert("while");

    CppControlKeyWords.insert("for");
    CppControlKeyWords.insert("if");
    CppControlKeyWords.insert("catch");

    //STL Containers
    STLContainers.insert("std::array");
    STLContainers.insert("std::vector");
    STLContainers.insert("std::deque");
    STLContainers.insert("std::forward_list");
    STLContainers.insert("std::list");

    STLContainers.insert("std::set");
//    STLContainers.insert("std::map");
//    STLContainers.insert("std::multilist");
//    STLContainers.insert("std::multimap");

    STLContainers.insert("std::unordered_set");
//    STLContainers.insert("std::unordered_map");
    STLContainers.insert("std::unordered_multiset");
//    STLContainers.insert("std::unordered_multimap");

    STLContainers.insert("std::stack");
    STLContainers.insert("std::queue");
    STLContainers.insert("std::priority_queue");

    STLContainers.insert("std::span");

    STLMaps.insert("std::map");
    STLMaps.insert("std::multilist");
    STLMaps.insert("std::multimap");
    STLMaps.insert("std::unordered_map");
    STLMaps.insert("std::unordered_multimap");

    //STL element access methods
    STLElementMethods.insert("at");
    STLElementMethods.insert("back");
    STLElementMethods.insert("front");
    STLElementMethods.insert("top");

    //STL iterator
    STLIterators.insert("iterator");
    STLIterators.insert("const_iterator");
    STLIterators.insert("const_local_iterator");
    STLIterators.insert("local_iterator");
    STLIterators.insert("reverse_iterator");
    STLIterators.insert("const_reverse_iterator");

    //STL pointers
    STLPointers.insert("std::unique_ptr");
    STLPointers.insert("std::auto_ptr");
    STLPointers.insert("std::shared_ptr");
    STLPointers.insert("std::weak_ptr");
    //STLPointers.insert("__gnu_cxx::__normal_iterator");
//    STLPointers.insert("std::reverse_iterator");
//    STLPointers.insert("std::iterator");
//    STLPointers.insert("std::const_iterator");
//    STLPointers.insert("std::const_reverse_iterator");

    AutoTypes.insert("auto");
    AutoTypes.insert("auto &");
    AutoTypes.insert("auto &&");
    AutoTypes.insert("const auto");
    AutoTypes.insert("const auto &");

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
    CppDirectives.append("#undef");

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

    MemberOperators.insert(".");
    MemberOperators.insert("::");
    MemberOperators.insert("->");
    MemberOperators.insert("->*");
    MemberOperators.insert(".*");

    IOManipulators.insert("std::boolalpha");
    IOManipulators.insert("std::noboolalpha");
    IOManipulators.insert("std::showbase");
    IOManipulators.insert("std::noshowbase");
    IOManipulators.insert("std::showpoint");
    IOManipulators.insert("std::noshowpoint");
    IOManipulators.insert("std::showpos");
    IOManipulators.insert("std::noshowpos");
    IOManipulators.insert("std::skipws");
    IOManipulators.insert("std::noskipws");

    IOManipulators.insert("std::uppercase");
    IOManipulators.insert("std::nouppercase");
    IOManipulators.insert("std::unitbuf");
    IOManipulators.insert("std::nounitbuf");
    IOManipulators.insert("std::left");
    IOManipulators.insert("std::right");
    IOManipulators.insert("std::internal");
    IOManipulators.insert("std::dec");
    IOManipulators.insert("std::hex");
    IOManipulators.insert("std::oct");

    IOManipulators.insert("std::fixed");
    IOManipulators.insert("std::scientific");
    IOManipulators.insert("std::hexfloat");
    IOManipulators.insert("std::defaultfloat");
    IOManipulators.insert("std::ws");
    IOManipulators.insert("std::ends");
    IOManipulators.insert("std::flush");
    IOManipulators.insert("std::endl");

}

QString getHeaderFilename(const QString &relativeTo, const QString &line,
                          const QStringList& includePaths, const QStringList& projectIncludePaths) {
    QString result = "";

    // Handle <>
    int openTokenPos = line.indexOf('<');
    if (openTokenPos >= 0) {
        int closeTokenPos = line.indexOf('>',openTokenPos+1);
        if (closeTokenPos >=0) {
            QString fileName = line.mid(openTokenPos + 1, closeTokenPos - openTokenPos - 1);
            //project settings is preferred
            result = getSystemHeaderFilename(fileName, projectIncludePaths);
            if (result.isEmpty()) {
                result = getSystemHeaderFilename(fileName, includePaths);
            }
        }
    } else {
        // Try ""
        openTokenPos = line.indexOf('"');
        if (openTokenPos >= 0) {
            int closeTokenPos = line.indexOf('"', openTokenPos+1);
            if (closeTokenPos >= 0) {
                QString fileName = line.mid(openTokenPos + 1, closeTokenPos - openTokenPos - 1);
                result = getLocalHeaderFilename(relativeTo, fileName);
                //project settings is preferred
                if (result.isEmpty()) {
                    result = getSystemHeaderFilename(fileName, projectIncludePaths);
                }
                if (result.isEmpty()) {
                    result = getSystemHeaderFilename(fileName, includePaths);
                }
            }
        }
    }

    return result;
}

QString getLocalHeaderFilename(const QString &relativeTo, const QString &fileName)
{
    QFileInfo relativeFile(relativeTo);
    QDir dir = relativeFile.dir();
    // Search local directory
    if (dir.exists(fileName)) {
        return cleanPath(dir.absoluteFilePath(fileName));
    }
    return "";
}

QString getSystemHeaderFilename(const QString &fileName, const QStringList& includePaths)
{

    // Search compiler include directories
    for (const QString& path:includePaths) {
        QDir dir(path);
        if (dir.exists(fileName)) {
            return cleanPath(dir.absoluteFilePath(fileName));
        }
    }
    //not found
    return "";
}

bool isSystemHeaderFile(const QString &fileName, const QSet<QString> &includePaths)
{
    if (fileName.isEmpty())
        return false;
    if (includePaths.isEmpty())
        return false;
    bool isFullName = false;

#ifdef Q_OS_WIN
    isFullName = fileName.startsWith("/") || (fileName.length()>2 && fileName[1]==':');
#else
    isFullName = fileName.startsWith("/");
#endif
    if (isFullName) {
        QFileInfo info(fileName);
        // If it's a full file name, check if its directory is an include path
        if (info.exists()) { // full file name
            QDir dir = info.dir();
            QString absPath = includeTrailingPathDelimiter(dir.absolutePath());
            foreach (const QString& incPath, includePaths) {
                if (absPath.startsWith(incPath))
                    return true;
            }
        }
    } else {
        //check if it's in the include dir
        for (const QString& includePath: includePaths) {
            QDir dir(includePath);
            if (dir.exists(fileName))
                return true;
        }
    }
    return false;
}

bool isCppKeyword(const QString &word)
{
    return CppKeywords.contains(word);
}

bool isHFile(const QString& filename)
{
    if (filename.isEmpty())
        return false;
    QFileInfo fileInfo(filename);
    return CppHeaderExts->contains(fileInfo.suffix().toLower());

}

bool isCFile(const QString& filename)
{
    if (filename.isEmpty())
        return false;

    QFileInfo fileInfo(filename);
    return CppSourceExts->contains(fileInfo.suffix().toLower());
}

PStatement CppScopes::findScopeAtLine(int line) const
{
    if (mScopes.isEmpty())
        return PStatement();
    int start = 0;
    int end = mScopes.size()-1;
    while (start<=end) {
        int mid = (start+end)/2;
        PCppScope midScope = mScopes[mid];
        if (midScope->startLine == line) {
            while (mid<end && (mScopes[mid+1]->startLine == line)) {
                mid++;
            }
            return mScopes[mid]->statement;
        } else if (midScope->startLine > line) {
            end = mid-1;
        } else {
            start = mid+1;
        }
    }
    if (end>=0)
        return mScopes[end]->statement;
    else
        return PStatement();
}

void CppScopes::addScope(int line, PStatement scopeStatement)
{
    PCppScope scope = std::make_shared<CppScope>();
    scope->startLine = line;
    scope->statement = scopeStatement;
    mScopes.append(scope);
#ifdef QT_DEBUG
    if (!mScopes.isEmpty() && mScopes.back()->startLine > line) {
        qDebug()<<QString("Error: new scope %1 at %2 which is less that last scope %3")
                  .arg(scopeStatement->fullName)
                  .arg(line)
                  .arg(mScopes.back()->startLine);
    }
#endif
}

MemberOperatorType getOperatorType(const QString &phrase, int index)
{
    if (index>=phrase.length())
        return MemberOperatorType::Other;
    if (phrase[index] == '.')
        return MemberOperatorType::Dot;
    if (index+1>=phrase.length())
        return MemberOperatorType::Other;
    if ((phrase[index] == '-') && (phrase[index+1] == '>'))
        return MemberOperatorType::Arrow;
    if ((phrase[index] == ':') && (phrase[index+1] == ':'))
        return MemberOperatorType::DColon;
    return MemberOperatorType::Other;
}

bool isScopeTypeKind(StatementKind kind)
{
    switch(kind) {
    case StatementKind::Class:
    case StatementKind::Namespace:
    case StatementKind::EnumType:
    case StatementKind::EnumClassType:
        return true;
    default:
        return false;
    }
}

EvalStatement::EvalStatement(
        const QString &baseType,
        EvalStatementKind kind,
        const PStatement &baseStatement,
        const PStatement& typeStatement,
        const PStatement& effectiveTypeStatement,
        int pointerLevel,
        const QString& templateParams)
{
    this->baseType = baseType;
    this->kind = kind;
    this->baseStatement = baseStatement;
    this->typeStatement = typeStatement;
    this->effectiveTypeStatement = effectiveTypeStatement;
    this->pointerLevel = pointerLevel;
    this->templateParams = templateParams;
}

void EvalStatement::assignType(const PEvalStatement &typeStatement)
{
    Q_ASSERT(typeStatement && typeStatement->kind==EvalStatementKind::Type);
    baseType = typeStatement->baseType;
    pointerLevel = typeStatement->pointerLevel;
    effectiveTypeStatement = typeStatement->effectiveTypeStatement;
}

QStringList getOwnerExpressionAndMember(const QStringList expression, QString &memberOperator, QStringList &memberExpression)
{
    //find position of the last member operator
    int lastMemberOperatorPos = -1;
    int currentMatchingLevel = 0;
    QString matchingSignLeft;
    QString matchingSignRight;
    for (int i=0;i<expression.length();i++) {
        QString token = expression[i];
        if (currentMatchingLevel == 0) {
            if (isMemberOperator(token)) {
                lastMemberOperatorPos = i;
            } else if (token == "(") {
                matchingSignLeft = "(";
                matchingSignRight = ")";
                currentMatchingLevel++;
            } else if (token == "[") {
                matchingSignLeft = "[";
                matchingSignRight = "]";
                currentMatchingLevel++;
            } else if (token == "<") {
                matchingSignLeft = "<";
                matchingSignRight = ">";
                currentMatchingLevel++;
            }
        } else {
            if (token == matchingSignLeft) {
                currentMatchingLevel++;
            } else if (token == matchingSignRight) {
                currentMatchingLevel--;
            }
        }
    }

    QStringList ownerExpression;
    if (lastMemberOperatorPos<0) {
        memberOperator = "";
        memberExpression = expression;
    } else {
        memberOperator = expression[lastMemberOperatorPos];
        memberExpression = expression.mid(lastMemberOperatorPos+1);
        ownerExpression = expression.mid(0,lastMemberOperatorPos);
    }
    if (memberExpression.length()>1) {
        memberExpression = memberExpression.mid(memberExpression.length()-1,1);
    }
    return ownerExpression;

}

bool isMemberOperator(QString token)
{
    return MemberOperators.contains(token);
}


StatementKind getKindOfStatement(const PStatement& statement)
{
    if (!statement)
        return StatementKind::Unknown;
    if (statement->kind == StatementKind::Variable) {
        if (!statement->parentScope.lock()) {
            return StatementKind::GlobalVariable;
        }  else if (statement->scope == StatementScope::Local) {
            return StatementKind::LocalVariable;
        } else {
            return StatementKind::Variable;
        }
    } else if (statement->kind == StatementKind::Parameter) {
        return StatementKind::LocalVariable;
    }
    return statement->kind;
}

bool isCppFile(const QString &filename)
{
    if (isCFile(filename) && !filename.endsWith(".c"))
        return true;
    return false;
}

bool isCppControlKeyword(const QString &word)
{
    return CppControlKeyWords.contains(word);
}

//static int counter=0;
//Statement::Statement()
//{
//    counter++;
//}

//Statement::~Statement()
//{
//    counter--;
//    qDebug()<<"statement deleted:"<<counter<<fullName<<kind<<extractFileName(fileName)<<line;
//}

bool isTypeKind(StatementKind kind)
{
    switch(kind) {
    case StatementKind::Class:
    case StatementKind::Namespace:
    case StatementKind::EnumType:
    case StatementKind::EnumClassType:
    case StatementKind::Typedef:
    case StatementKind::Preprocessor:
        return true;
    default:
        return false;
    }
}

bool ParsedFileInfo::isLineVisible(int line) const
{
    int lastI=-1;
    for(auto it=mBranches.begin();it!=mBranches.end();++it) {
        int i = it.key();
        if (line<i)
            break;
        else
            lastI = i;
    }
    return lastI<0?true:mBranches[lastI];
}
