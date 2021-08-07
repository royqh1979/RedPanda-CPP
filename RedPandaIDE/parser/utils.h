#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H
#include <QMap>
#include <QObject>
#include <QSet>
#include <memory>
// preprocess/ macro define
struct Define {
    QString name;
    QString args;
    QString value;
    QString filename;
    bool isMultiLine; // if true the expanded macro will span multiline
    bool hardCoded;// if true, don't free memory (points to hard defines)
    QStringList argList; // args list to format values
    QString formatValue; // format template to format values
};

using PDefine = std::shared_ptr<Define>;

using DefineMap = QHash<QString,PDefine>;
using PDefineMap = std::shared_ptr<DefineMap>;

enum class SkipType {
    skItself,  // skip itself
    skToSemicolon, // skip to ;
    skToColon, // skip to :
    skToRightParenthesis, // skip to )
    skToLeftBrace,// Skip to {
    skToRightBrace, // skip to }
    skNone // It's a keyword but don't process here
};

enum class StatementKind  {
  skUnknown,
  skPreprocessor,
  skEnumType,
  skEnum,
  skTypedef,
  skClass,
  skFunction,
  skOperator,
  skConstructor,
  skDestructor,
  skVariable,
  skParameter,
  skNamespace,
  skNamespaceAlias,
  skBlock,
  skUserCodeIn,  // user code template
  skKeyword, // keywords
  skGlobalVariable,
  skLocalVariable,
  skAlias
};

using StatementKindSet = QSet<StatementKind>;

enum class StatementScope {
    ssGlobal,
    ssLocal,
    ssClassLocal
};

enum class StatementClassScope {
  scsNone,
  scsPrivate,
  scsProtected,
  scsPublic
};

enum class MemberOperatorType {
  otArrow,
  otDot,
  otDColon,
  otOther
};

struct RemovedStatement{
  QString type; // type "int"
  QString command; // identifier/name of statement "foo"
  int definitionLine; // definition
  QString definitionFileName; // definition
  QString fullName; // fullname(including class and namespace)
  QString noNameArgs; // Args without name
};

using PRemovedStatement = std::shared_ptr<RemovedStatement>;

struct Statement;
using PStatement = std::shared_ptr<Statement>;
using StatementList = QVector<PStatement>;
using PStatementList = std::shared_ptr<StatementList>;
using StatementMap = QHash<QString, PStatementList>;
struct Statement {
    std::weak_ptr<Statement> parentScope; // parent class/struct/namespace scope, don't use auto pointer to prevent circular reference
    QString hintText; // text to force display when using PrettyPrintStatement
    QString type; // type "int"
    QString command; // identifier/name of statement "foo"
    QString args; // args "(int a,float b)"
    QStringList argList;
    QString value; // Used for macro defines/typedef, "100" in "#defin COUNT 100"
    StatementKind kind; // kind of statement class/variable/function/etc
    QList<std::weak_ptr<Statement>> inheritanceList; // list of statements this one inherits from, can be nil
    StatementScope scope; // global/local/classlocal
    StatementClassScope classScope; // protected/private/public
    bool hasDefinition; // definiton line/filename is valid
    int line; // declaration
    int endLine;
    int definitionLine; // definition
    int definitionEndLine;
    QString fileName; // declaration
    QString definitionFileName; // definition
    bool inProject; // statement in project
    bool inSystemHeader; // statement in system header (#include <>)
    StatementMap children; // functions can be overloaded,so we use list to save children with the same name
    QSet<QString> friends; // friend class / functions
    bool isStatic; // static function / variable
    bool isInherited; // inherted member;
    QString fullName; // fullname(including class and namespace), ClassA::foo
    QStringList usingList; // using namespaces
    int usageCount; //Usage Count, used by TCodeCompletion
    int freqTop; // Usage Count Rank, used by TCodeCompletion
    QString noNameArgs;// Args without name
};

struct UsingNamespace {
    QStringList namespaces; // List['std','foo'] for using namespace std::foo;
    QString filename;
    int line;
    int endLine;
    bool fromHeader;
};
using PUsingNamespace = std::shared_ptr<UsingNamespace>;

struct IncompleteClass {
    std::weak_ptr<Statement> statement;
    int count;
};
using PIncompleteClass = std::shared_ptr<IncompleteClass>;

struct FileIncludes {
    QString baseFile;
    QMap<QString,bool> includeFiles; // true means the file is directly included, false means included indirectly
    QSet<QString> usings; // namespaces it usings
    QVector<std::weak_ptr<Statement>> statements; // but we don't save temporary statements
    //StatementsIndex: TDevStringHash;
    QVector<std::weak_ptr<Statement>> declaredStatements; // statements declared in this file
    QMap<int, std::weak_ptr<Statement>> scopes; // int is start line of the statement scope
    QSet<QString> dependingFiles; // The files I depeneds on
    QSet<QString> dependedFiles; // the files depends on me
};
using PFileIncludes = std::shared_ptr<FileIncludes>;


extern QStringList CppDirectives;
extern QStringList JavadocTags;
extern QMap<QString,SkipType> CppKeywords;
extern QSet<QString> CppTypeKeywords;
extern QSet<QString> STLPointers;
extern QSet<QString> STLContainers;
extern QSet<QString> STLElementMethods;

void initParser();
#endif // PARSER_UTILS_H
