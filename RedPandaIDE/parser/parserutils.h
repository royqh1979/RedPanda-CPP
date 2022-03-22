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

enum class SkipType {
    skItself,  // skip itself
    skToSemicolon, // skip to ;
    skToColon, // skip to :
    skToRightParenthesis, // skip to )
    skToLeftBrace,// Skip to {
    skToRightBrace, // skip to }
    skNone // It's a keyword but don't process here
};


enum StatementKind  {
  skUnknown,
  skPreprocessor,
  skEnumType,
  skEnumClassType,
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
  skUserCodeSnippet,  // user code template
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

enum class EvalStatementKind {
    Namespace,
    Type,
    Variable,
    Literal,
    Function
};

using PRemovedStatement = std::shared_ptr<RemovedStatement>;

struct StatementMatchPosition{
    int start;
    int end;
};

using PStatementMathPosition = std::shared_ptr<StatementMatchPosition>;


struct Statement;
using PStatement = std::shared_ptr<Statement>;
using StatementList = QList<PStatement>;
using PStatementList = std::shared_ptr<StatementList>;
using StatementMap = QMultiMap<QString, PStatement>;
struct Statement {
    std::weak_ptr<Statement> parentScope; // parent class/struct/namespace scope, don't use auto pointer to prevent circular reference
    QString hintText; // text to force display when using PrettyPrintStatement
    QString type; // type "int"
    QString command; // identifier/name of statement "foo"
    QString args; // args "(int a,float b)"
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
    QSet<QString> usingList; // using namespaces
    QString noNameArgs;// Args without name
    // fields for code completion
    int usageCount; //Usage Count
    int matchPosTotal; // total of matched positions
    int matchPosSpan; // distance between the first match pos and the last match pos;
    int firstMatchLength; // length of first match;
    int caseMatched; // if match with case
    QList<PStatementMathPosition> matchPositions;
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
    EvalStatementKind kind; // namespace / type / variable / function / literal
    int pointerLevel; // 0 for "int", 1 for "int *", 2 for "int **"...
    PStatement baseStatement; // if not literal or primitive type, the base statement
    PStatement effectiveTypeStatement;
public:
    EvalStatement (const QString& baseType,
                      EvalStatementKind kind,
                      const PStatement& baseStatement,
                      const PStatement& typeStatement,
                      int pointerLevel = 0);
    void assignType(const PEvalStatement& typeStatement);

};


struct UsingNamespace {
    QStringList namespaces; // List['std','foo'] for using namespace std::foo;
    QString filename;
    int line;
    int endLine;
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
    QMap<QString,bool> includeFiles; // true means the file is directly included, false means included indirectly
    QSet<QString> usings; // namespaces it usings
    StatementMap statements; // but we don't save temporary statements (full name as key)
    StatementMap declaredStatements; // statements declared in this file (full name as key)
    CppScopes scopes; // int is start line of the statement scope
    QSet<QString> dependingFiles; // The files I depeneds on
    QSet<QString> dependedFiles; // the files depends on me
};
using PFileIncludes = std::shared_ptr<FileIncludes>;

extern QStringList CppDirectives;
extern QStringList JavadocTags;
extern QMap<QString,SkipType> CppKeywords;
extern QSet<QString> CppControlKeyWords;
extern QSet<QString> CKeywords;
extern QSet<QString> CppTypeKeywords;
extern QSet<QString> STLPointers;
extern QSet<QString> STLContainers;
extern QSet<QString> STLElementMethods;
extern QSet<QString> MemberOperators;

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
MemberOperatorType getOperatorType(const QString& phrase, int index);
QStringList getOwnerExpressionAndMember(
        const QStringList expression,
        QString& memberOperator,
        QStringList& memberExpression);
bool isMemberOperator(QString token);
StatementKind getKindOfStatement(const PStatement& statement);

#endif // PARSER_UTILS_H
