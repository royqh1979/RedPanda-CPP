/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#include <QDateTime>
#include <memory>
#include <functional>
#include "pptoken.h"
#include "cpptypes.h"
#include "cppscopes.h"

using GetFileStreamFunc = std::function<bool (const QString&, QStringList&)>;

enum class ParserLanguage {
    None,
    C,
    CPlusPlus,
#ifdef ENABLE_SDCC
    SDCC,
#endif
};

inline uint qHash(const ParserLanguage& value, uint seed) {
    return qHash((int)value, seed);
}

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
    // fields for the pp-token expansion engine (phase 0.6)
    QStringList paramNames; // parameter names in order ("..." for variadics)
    ReplTokenList replacementList; // precompiled replacement list
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
    Concept, //concept
    If, //If
    Requires, //requires
    None, // It's a keyword but don't process here
    Extern,
    NotKeyword
};

enum class StatementKind  {
    Unknown,
    Namespace,
    NamespaceAlias,
    Class,
    Preprocessor,
    EnumType,
    EnumClassType,
    Typedef,
    Constructor,
    Destructor,
    Function,
    OverloadedOperator,
    LiteralOperator,
    Variable,
    GlobalVariable,
    LocalVariable,
    Enum,
    Operator,
    Parameter,
    Block,
    Lambda,
    Label,
    UserCodeSnippet,  // user code template
    Keyword, // keywords
    KeywordType, //keywords for type (for color management)
    Alias, // using alias
};

inline uint qHash(const StatementKind& value, uint seed) {
    return qHash((int)value, seed);
}

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
    Function,
    Label,
    Preprocessor
};

struct StatementMatchPosition{
    uint16_t start;
    uint16_t end;
};

enum class StatementProperty {
    None =                0x0,
    Static =              0x0001,
    HasDefinition =       0x0002,
    InProject =           0x0004,
    InSystemHeader =      0x0008,
    Inherited =           0x0010,
    Virtual =             0x0020,
    Override =            0x0040,
    Constexpr =           0x0080,
    FunctionPointer =     0x0100,
    OperatorOverloading = 0x0200,
    DummyStatement     =  0x0400
};

Q_DECLARE_FLAGS(StatementProperties, StatementProperty)

Q_DECLARE_OPERATORS_FOR_FLAGS(StatementProperties)

using PStatementMathPosition = std::shared_ptr<StatementMatchPosition>;

struct Statement;
using PStatement = std::shared_ptr<Statement>;
using StatementList = QList<PStatement>;
using PStatementList = std::shared_ptr<StatementList>;
using StatementMap = QMultiMap<QString, std::weak_ptr<Statement>>;
struct Statement {
    std::weak_ptr<Statement> parentScope; // parent class/struct/namespace scope, use weak pointer to prevent circular reference
    TypeInfo typeInfo;
    QString command; // identifier/name of statement "foo"
    QString args; // args "(int a,float b)"
    QVector<ParameterInfo> parameters;
    QString value; // Used for macro defines/typedef, "100" in "#defin COUNT 100"
    TemplateDeclarationKind templateKind = TemplateDeclarationKind::None;
    QVector<TemplateParameterInfo> templateParameters;
    DeclSpecifierProperties declSpecifierProperties;
    StatementKind kind; // kind of statement class/variable/function/etc
    ClassKey classKey = ClassKey::Class;
    StatementScope scope; // global/local/classlocal
    StatementAccessibility accessibility; // protected/private/public
    SourceRange declarationRange;
    SourceRange definitionRange;
    QString fileName; // declaration
    QString declarationBoundaryAncestor;
    QString definitionFileName; // definition
    QString definitionBoundaryAncestor;
    StatementMap children;
    QSet<QString> friends; // friend class / functions
    QString fullName; // fullname(including class and namespace), ClassA::foo
    QSet<QString> usingList; // using namespaces
    QStringList baseClasses;
    QString noNameArgs;// Args without name
    QSet<QString> lambdaCaptures;
    StatementProperties properties;

    QString type() const {
        QString result;
        if (declSpecifierProperties.testFlag(DeclSpecifierProperty::Static))
            result = "static ";
        else if (declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern))
            result = "extern ";
        if (declSpecifierProperties.testFlag(DeclSpecifierProperty::Register))
            result += "register ";
        if (declSpecifierProperties.testFlag(DeclSpecifierProperty::ThreadLocal))
            result += "thread_local ";
        if (declSpecifierProperties.testFlag(DeclSpecifierProperty::Mutable))
            result += "mutable ";
        TypeInfo compatibleType = typeInfo;
        bool hasArray = false;
        bool hasPointer = false;
        for (const TypeDerivation &derivation : compatibleType.derivations) {
            hasArray |= derivation.kind == TypeDerivationKind::Array;
            hasPointer |= derivation.kind == TypeDerivationKind::Pointer;
        }
        for (int i = compatibleType.derivations.count() - 1; i >= 0; --i) {
            if (properties.testFlag(StatementProperty::FunctionPointer)
                    && compatibleType.derivations[i].kind == TypeDerivationKind::Function) {
                compatibleType.derivations.remove(0, i + 1);
                break;
            }
            if (compatibleType.derivations[i].kind == TypeDerivationKind::Array
                    || (hasArray && compatibleType.derivations[i].kind == TypeDerivationKind::Pointer))
                compatibleType.derivations.remove(i);
        }
        QString typeText = compatibleType.toString();
        if (hasArray && hasPointer)
            typeText += " (*)";
        return result + typeText;
    }
    void setType(const TypeInfo &type) { typeInfo = type; }

    // fields for code completion
    int usageCount; //Usage Count
    uint16_t matchPosTotal; // total of matched positions
    uint16_t matchPosSpan; // distance between the first match pos and the last match pos;
    uint16_t firstMatchLength; // length of first match;
    uint16_t caseMatched; // if match with case
    QList<PStatementMathPosition> matchPositions;

    // definiton line/filename is valid
    bool hasDefinition() {
        return properties.testFlag(StatementProperty::HasDefinition);
    }
    void setHasDefinition(bool on) {
        properties.setFlag(StatementProperty::HasDefinition,on);
    }
    // statement in project
    bool inProject() {
        return properties.testFlag(StatementProperty::InProject);
    }
    void setInProject(bool on) {
        properties.setFlag(StatementProperty::InProject, on);
    }
    // statement in system header (#include <>)
    bool inSystemHeader() {
        return properties.testFlag(StatementProperty::InSystemHeader);
    }
    void setInSystemHeader(bool on) {
        properties.setFlag(StatementProperty::InSystemHeader, on);
    }
    bool isStatic() {
        return properties.testFlag(StatementProperty::Static);
    } // static function / variable
    void setIsStatic(bool on) {
        properties.setFlag(StatementProperty::Static, on);
    }
    bool isInherited() {
        return properties.testFlag(StatementProperty::Inherited);
    } // inherted member;

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
    EvalStatement (const QString& newBaseType,
                   EvalStatementKind newKind,
                   const PStatement& newBaseStatement,
                   const PStatement& newTypeStatement,
                   const PStatement& newEffectiveTypeStatement,
                   int newPointerLevel=0,
                   const QString& newTemplateParams=QString());
    void assignType(const PEvalStatement& typeStmt);

};


struct UsingNamespace {
    QStringList namespaces; // List['std','foo'] for using namespace std::foo;
    QString filename;
    int line;
    bool fromHeader;
};
using PUsingNamespace = std::shared_ptr<UsingNamespace>;

struct ClassInheritanceInfo {
    std::weak_ptr<Statement> derivedClass;
    QString file;
    QString parentClassName;
    bool isGlobal;
    bool isStruct;
    StatementAccessibility visibility;
//    QString parentClassFilename;
    bool handled;
};

using PClassInheritanceInfo = std::shared_ptr<ClassInheritanceInfo>;

class ParsedFileInfo {
public:
    ParsedFileInfo(const QString& fileName): mFileName {fileName} { }
    ParsedFileInfo(const ParsedFileInfo&)=delete;
    ParsedFileInfo& operator=(const ParsedFileInfo&)=delete;
    void insertBranch(int level, bool branchTrue) { mBranches.insert(level, branchTrue); }
    bool isLineVisible(int line) const;
    void addInclude(const QString &fileName) { mIncludes.insert(fileName); }
    void addDirectInclude(const QString &fileName) { mDirectIncludes.append(fileName); }
    bool including(const QString &fileName) const { return mIncludes.contains(fileName); }
    PStatement findScopeAt(const SourcePosition &position) const { return mScopes.findScopeAt(position); }
    PStatement findScopeAtLine(int line) const { return mScopes.findScopeAtLine(line); }
    void addStatement(const PStatement &statement) { mStatements.insert(statement->fullName,statement); }
    void removeStatement(const PStatement &statement) {
        for (auto it = mStatements.find(statement->fullName);
                it != mStatements.end() && it.key() == statement->fullName;) {
            PStatement indexed = it.value().lock();
            if (!indexed || indexed == statement)
                it = mStatements.erase(it);
            else
                ++it;
        }
    }
    void clearStatements() { mStatements.clear(); }
    void addScope(const SourceRange &range, const PStatement &scope, int parentIndex = -1) {
        mScopes.addScope(range, scope, parentIndex);
    }
    void addScope(const SourceRange &range, const PStatement &scope, const PStatement &parentScope) {
        mScopes.addScope(range, scope, parentScope);
    }
    void removeLastScope() { mScopes.removeLastScope(); }
    PStatement lastScope() const { return mScopes.lastScope(); }
    void addUsing(const QString &usingSymbol) { mUsings.insert(usingSymbol); }
    void addHandledInheritances(std::weak_ptr<ClassInheritanceInfo> classInheritanceInfo) { mHandledInheritances.append(classInheritanceInfo); }
    void clearHandledInheritances() { mHandledInheritances.clear(); }

    void setReplayCondition(const QHash<QString,QString> &userMacros,
                            const QStringList &boundaryPrefix) {
        mUserMacros = userMacros;
        mBoundaryPrefix = boundaryPrefix;
        mHasReplayCondition = true;
    }
    bool hasReplayCondition() const { return mHasReplayCondition; }
    const QHash<QString,QString> &userMacros() const { return mUserMacros; }
    const QStringList &boundaryPrefix() const { return mBoundaryPrefix; }
    PDefineMap defines() const { return mDefines; }
    PDefineMap defines() {
        if (!mDefines)
            mDefines = std::make_shared<DefineMap>();
        return mDefines;
    }
    PDefineMap undefines() const { return mUndefines; }
    PDefineMap undefines() {
        if (!mUndefines)
            mUndefines = std::make_shared<DefineMap>();
        return mUndefines;
    }

    QString fileName() const { return mFileName; }
    const StatementMap& statements() const { return mStatements; }
    const QSet<QString>& usings() const { return mUsings; }
    const QStringList& directIncludes() const { return mDirectIncludes; }
    const QSet<QString>& includes() const { return mIncludes; }
    const QList<std::weak_ptr<ClassInheritanceInfo> >& handledInheritances() const { return mHandledInheritances; }

private:
    QString mFileName;
    QSet<QString> mIncludes;
    QStringList mDirectIncludes; //We need order here.
    QSet<QString> mUsings; // namespaces it usings
    StatementMap mStatements; // but we don't save temporary statements (full name as key)
    CppScopes mScopes; // int is start line of the statement scope
    QMap<int,bool> mBranches;
    QList<std::weak_ptr<ClassInheritanceInfo>> mHandledInheritances;
    bool mHasReplayCondition = false;
    QHash<QString,QString> mUserMacros;
    QStringList mBoundaryPrefix;
    PDefineMap mDefines;
    PDefineMap mUndefines;
};

using PParsedFileInfo = std::shared_ptr<ParsedFileInfo>;

extern QStringList cppDirectives;
extern QStringList javadocTags;
extern QMap<QString,KeywordType> CppKeywords;
#ifdef ENABLE_SDCC
extern QMap<QString,KeywordType> SDCCKeywords;
extern QSet<QString> SDCCTypeKeywords;
#endif
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
