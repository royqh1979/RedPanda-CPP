#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H
#include <QObject>
#include <QSet>
#include <memory>
// preprocess/ macro define
struct Define {
    QString Name;
    QString args;
    QString value;
    QString filename;
    bool isMultiLine; // if true the expanded macro will span multiline
    bool hardCoded;// if true, don't free memory (points to hard defines)
    QStringList argList; // args list to format values
    QString formatValue; // format template to format values
};

using PDefine = std::shared_ptr<Define>;

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
    int definitionLine; // definition
    QString fileName; // declaration
    QString definitionFileName: AnsiString; // definition
    _InProject: boolean; // statement in project
    _InSystemHeader: boolean; // statement in system header (#include <>)
    _Children: TList; // Children Statement to speedup search
    _ChildrenIndex: TDevStringHash; // children statements index to speedup search
    _Friends: TStringHash; // friend class / functions
    _Static: boolean; // static function / variable
    _Inherited: boolean; // inherted member;
    _FullName: AnsiString; // fullname(including class and namespace)
    _Usings: TStringList;
    _Node: Pointer;    // the Node TStatementList used to save this statement
    _UsageCount : integer; //Usage Count, used by TCodeCompletion
    _FreqTop: integer; // Usage Count Rank, used by TCodeCompletion
    _NoNameArgs: AnsiString; // Args without name
#endif // PARSER_UTILS_H
