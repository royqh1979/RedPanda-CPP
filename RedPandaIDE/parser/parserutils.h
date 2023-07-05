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
#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H
#include <QMap>
#include <QObject>
#include <QSet>
#include <QVector>
#include <memory>

using GetFileStreamCallBack = std::function<bool (const QString&, QStringList&)>;

enum ParserLanguage {
    C,
    CPlusPlus
};

struct CodeSnippet {
    QString caption; //Name
    QString prefix; //Prefix used in code suggestion
    QString code;  //Code body
    QString desc;  //Description
    int section;  //Section in the menu
};

using PCodeSnippet = std::shared_ptr<CodeSnippet>;

// preprocess/ macro define
struct Define {
    QString name;
    QString args;
    QString value;
    QString filename;
    bool hardCoded;// if true, don't free memory (points to hard defines)
    QStringList argList; // args list to format values
    QList<bool> argUsed;
    QString formatValue; // format template to format values
};

using PDefine = std::shared_ptr<Define>;

using DefineMap = QHash<QString,PDefine>;
using PDefineMap = std::shared_ptr<DefineMap>;

enum class KeywordType {
    SkipItself,  // skip itself
    SkipNextSemicolon, // move to ; and skip it
    SkipNextColon, // move to : and skip it
    SkipNextParenthesis, // move to ) and skip it
    MoveToLeftBrace,// move to {
//    MoveToRightBrace, // move to }
    For, //for
    Catch, //catch
    Public, // public
    Private,
    Protected,
    Friend,
    Struct, // struct/class/enum
    Enum, //enum
    Inline, // inline
    Namespace, //namespace
    Typedef, //typedef
    Using, //using
    DeclType, // decltype
    Operator, //operator
    None, // It's a keyword but don't process here
    NotKeyword
};

//It will be used as hash key. DONT make it enum class!!!!!
enum StatementKind  {
  skUnknown,
  skNamespace,
  skNamespaceAlias,
  skClass,
  skPreprocessor,
  skEnumType,
  skEnumClassType,
  skTypedef,
  skConstructor,
  skDestructor,
  skFunction,
  skVariable,
  skGlobalVariable,
  skLocalVariable,
  skEnum,
  skOperator,
  skParameter,
  skBlock,
  skUserCodeSnippet,  // user code template
  skKeyword, // keywords
  skKeywordType, //keywords for type (for color management)
  skAlias
};

using StatementKindSet = QSet<StatementKind>;

enum class StatementScope {
    Global,
    Local,
    ClassLocal
};

enum class StatementAccessibility {
  None,
  Private,
  Protected,
  Public
};

enum class MemberOperatorType {
  Arrow,
  Dot,
  DColon,
  Other
};

enum class EvalStatementKind {
    Namespace,
    Type,
    Variable,
    Literal,
    Function
};

struct StatementMatchPosition{
    int start;
    int end;
};

enum StatementProperty {
    spNone =                0x0,
    spStatic =              0x0001,
    spHasDefinition =       0x0002,
    spInProject =           0x0004,
    spInSystemHeader =      0x0008,
    spInherited =           0x0010,
    spVirtual =             0x0020,
    spOverride =            0x0040,
    spConstexpr =           0x0080,
    spFunctionPointer =     0x0100,
    spOperatorOverloading = 0x0200
};

Q_DECLARE_FLAGS(StatementProperties, StatementProperty)

Q_DECLARE_OPERATORS_FOR_FLAGS(StatementProperties)



using PStatementMathPosition = std::shared_ptr<StatementMatchPosition>;

struct Statement;
using PStatement = std::shared_ptr<Statement>;
using StatementList = QList<PStatement>;
using PStatementList = std::shared_ptr<StatementList>;
using StatementMap = QMultiMap<QString, PStatement>;
struct Statement {
//    Statement();
//    ~Statement();
    std::weak_ptr<Statement> parentScope; // parent class/struct/namespace scope, use weak pointer to prevent circular reference
    QString type; // type "int"
    QString command; // identifier/name of statement "foo"
    QString args; // args "(int a,float b)"
    QString value; // Used for macro defines/typedef, "100" in "#defin COUNT 100"
    StatementKind kind; // kind of statement class/variable/function/etc
    StatementScope scope; // global/local/classlocal
    StatementAccessibility accessibility; // protected/private/public
    int line; // declaration
    int definitionLine; // definition
    QString fileName; // declaration
    QString definitionFileName; // definition
    StatementMap children; // functions can be overloaded,so we use list to save children with the same name
    QSet<QString> friends; // friend class / functions
    QString fullName; // fullname(including class and namespace), ClassA::foo
    QSet<QString> usingList; // using namespaces
    QString noNameArgs;// Args without name
    StatementProperties properties;

    // fields for code completion
    int usageCount; //Usage Count
    int matchPosTotal; // total of matched positions
    int matchPosSpan; // distance between the first match pos and the last match pos;
    int firstMatchLength; // length of first match;
    int caseMatched; // if match with case
    QList<PStatementMathPosition> matchPositions;

    // definiton line/filename is valid
    bool hasDefinition() {
        return properties.testFlag(StatementProperty::spHasDefinition);
    };
    void setHasDefinition(bool on) {
        properties.setFlag(StatementProperty::spHasDefinition,on);
    }
    // statement in project
    bool inProject() {
        return properties.testFlag(StatementProperty::spInProject);
    }
    void setInProject(bool on) {
        properties.setFlag(StatementProperty::spInProject, on);
    }
    // statement in system header (#include <>)
    bool inSystemHeader() {
        return properties.testFlag(StatementProperty::spInSystemHeader);
    };
    void setInSystemHeader(bool on) {
        properties.setFlag(StatementProperty::spInSystemHeader, on);
    }
    bool isStatic() {
        return properties.testFlag(StatementProperty::spStatic);
    } // static function / variable
    void setIsStatic(bool on) {
        properties.setFlag(StatementProperty::spStatic, on);
    }
    bool isInherited() {
        return properties.testFlag(StatementProperty::spInherited);
    }; // inherted member;

};

struct EvalStatement;
using PEvalStatement = std::shared_ptr<EvalStatement>;
/**
 * @brief Statement for evaluation result
 * Ex. (Test*)(y+1)
 * it's baseStatement is the statement for y
 * it's effetiveTypeStatement is Test
 */
struct EvalStatement {
    QString baseType; // type "int"
    QString templateParams;
    EvalStatementKind kind; // namespace / type / variable / function / literal
    int pointerLevel; // 0 for "int", 1 for "int *", 2 for "int **"...
    QString definitionString; // std::vector<int> etc...
    PStatement baseStatement; // if not literal or primitive type, the base statement
    PStatement typeStatement;
    PStatement effectiveTypeStatement;
public:
    EvalStatement (const QString& baseType,
                   EvalStatementKind kind,
                   const PStatement& baseStatement,
                   const PStatement& typeStatement,
                   const PStatement& effectiveTypeStatement,
                   int pointerLevel=0,
                   const QString& templateParams=QString());
    void assignType(const PEvalStatement& typeStatement);

};


struct UsingNamespace {
    QStringList namespaces; // List['std','foo'] for using namespace std::foo;
    QString filename;
    int line;
    bool fromHeader;
};
using PUsingNamespace = std::shared_ptr<UsingNamespace>;

struct CppScope {
    int startLine;
    PStatement statement;
};

using PCppScope = std::shared_ptr<CppScope>;
class CppScopes {

public:
    PStatement findScopeAtLine(int line);
    void addScope(int line, PStatement scopeStatement);
    PStatement lastScope();
    void removeLastScope();
    void clear();
private:
    QVector<PCppScope> mScopes;
};

struct FileIncludes {
    QString baseFile;
    QMap<QString, bool> includeFiles; // true means the file is directly included, false means included indirectly
    QStringList directIncludes; //
    QSet<QString> usings; // namespaces it usings
    StatementMap statements; // but we don't save temporary statements (full name as key)
    StatementMap declaredStatements; // statements declared in this file (full name as key)
    CppScopes scopes; // int is start line of the statement scope
};
using PFileIncludes = std::shared_ptr<FileIncludes>;

extern QStringList CppDirectives;
extern QStringList JavadocTags;
extern QMap<QString,KeywordType> CppKeywords;
extern QSet<QString> CppControlKeyWords;
extern QSet<QString> CKeywords;
extern QSet<QString> CppTypeKeywords;
extern QSet<QString> STLPointers;
extern QSet<QString> STLContainers;
extern QSet<QString> STLMaps;
extern QSet<QString> STLElementMethods;
extern QSet<QString> STLIterators;
extern QSet<QString> MemberOperators;
extern QSet<QString> IOManipulators;
extern QSet<QString> AutoTypes;

void initParser();

QString getHeaderFilename(const QString& relativeTo, const QString& line,
                       const QStringList& includePaths, const QStringList& projectIncludePaths);

QString getLocalHeaderFilename(const QString& relativeTo, const QString& fileName);

QString getSystemHeaderFilename(const QString& fileName, const QStringList& includePaths);
bool isSystemHeaderFile(const QString& fileName, const QSet<QString>& includePaths);
bool isHFile(const QString& filename);
bool isCFile(const QString& filename);
bool isCppFile(const QString& filename);
bool isCppKeyword(const QString& word);
bool isCppControlKeyword(const QString& word);
bool isScopeTypeKind(StatementKind kind);
bool isTypeKind(StatementKind kind);
MemberOperatorType getOperatorType(const QString& phrase, int index);
QStringList getOwnerExpressionAndMember(
        const QStringList expression,
        QString& memberOperator,
        QStringList& memberExpression);
bool isMemberOperator(QString token);
StatementKind getKindOfStatement(const PStatement& statement);

#endif // PARSER_UTILS_H
