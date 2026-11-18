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
#include <QMultiHash>
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
    QVector<PpToken> valueTokens; // tokenized body of an object-like macro
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
    InlineAlias, // automatically generated inline namespace projection
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
    DummyStatement     =  0x0400,
    Const =               0x0800,
    Volatile =            0x1000,
    LvalueReference =     0x2000,
    RvalueReference =     0x4000
};

Q_DECLARE_FLAGS(StatementProperties, StatementProperty)

Q_DECLARE_OPERATORS_FOR_FLAGS(StatementProperties)

using PStatementMathPosition = std::shared_ptr<StatementMatchPosition>;

struct AbstractStatement;
using PStatement = std::shared_ptr<AbstractStatement>;
using StatementList = QList<PStatement>;
using PStatementList = std::shared_ptr<StatementList>;
using StatementMap = QMultiMap<QString, std::weak_ptr<AbstractStatement>>;

inline uint qHash(const PStatement& statement, uint seed) {
    return qHash(statement.get(),seed);
}

struct StatementOccurrence {
    QString fileName;
    SourceRange range;
    QString boundaryAncestor;
    DeclSpecifierProperties declSpecifierProperties;
};

struct AbstractStatement {
    std::weak_ptr<AbstractStatement> parentScope; // parent class/struct/namespace scope, use weak pointer to prevent circular reference
    QString command; // identifier/name of statement "foo"
    StatementKind kind; // kind of statement class/variable/function/etc
    StatementScope scope; // global/local/classlocal
    StatementAccessibility accessibility; // protected/private/public
    StatementOccurrence definition;
    QString fullName; // fullname(including class and namespace), ClassA::foo
    StatementProperties properties;
    QMultiHash<QString, StatementOccurrence> declarations;

    // fields for code completion
    int usageCount; //Usage Count
    uint16_t matchPosTotal; // total of matched positions
    uint16_t matchPosSpan; // distance between the first match pos and the last match pos;
    uint16_t firstMatchLength; // length of first match;
    uint16_t caseMatched; // if match with case
    QList<PStatementMathPosition> matchPositions;

    virtual ~AbstractStatement() = default;
};

struct Statement : AbstractStatement {
    TypeInfo typeInfo;
    QString args; // args "(int a,float b)"
    QVector<ParameterInfo> parameters;
    QString value; // Macro/typedef text or a constexpr variable template initializer
    TemplateDeclarationKind templateKind = TemplateDeclarationKind::None;
    QString templateRequiresClause;
    QString requiresClause;
    TypeInfo templatePattern;
    QVector<TemplateParameterInfo> templateParameters;
    QVector<QString> inheritedDefaultArgumentTexts;
    DeclSpecifierProperties declSpecifierProperties;
    ClassKey classKey = ClassKey::Class;
    StatementMap children;
    QSet<QString> friends; // friend class / functions
    QSet<QString> usingList; // using namespaces
    QStringList baseClasses;
    QString normalizedArgs;// Canonical parameter signature without ordinary parameter names
    QSet<QString> lambdaCaptures;
};

struct NamespaceStatement : AbstractStatement {
    DeclSpecifierProperties declSpecifierProperties;
    StatementMap children;
    QSet<QString> usingList; // using namespaces
};

struct InlineAliasStatement : AbstractStatement {
    std::weak_ptr<AbstractStatement> target;
};

inline bool statementHasDefinition(const PStatement &statement) {
    return statement->properties.testFlag(StatementProperty::HasDefinition);
}

inline void setStatementHasDefinition(const PStatement &statement, bool on) {
    statement->properties.setFlag(StatementProperty::HasDefinition, on);
}

inline StatementOccurrence statementDefinitionForNavigation(const PStatement &statement) {
    return statementHasDefinition(statement) ? statement->definition : StatementOccurrence{};
}

inline StatementOccurrence statementDeclarationForNavigation(const PStatement &statement,
                                                              const QString &currentFile) {
    for (const StatementOccurrence &occurrence : statement->declarations) {
        if (occurrence.fileName == currentFile)
            return occurrence;
    }
    const QList<StatementOccurrence> contextOccurrences = statement->declarations.values(currentFile);
    if (!contextOccurrences.isEmpty())
        return contextOccurrences.first();
    if (!statement->declarations.isEmpty())
        return *statement->declarations.constBegin();
    return statementDefinitionForNavigation(statement);
}

inline bool statementInProject(const PStatement &statement) {
    return statement->properties.testFlag(StatementProperty::InProject);
}

inline void setStatementInProject(const PStatement &statement, bool on) {
    statement->properties.setFlag(StatementProperty::InProject, on);
}

inline bool statementInSystemHeader(const PStatement &statement) {
    return statement->properties.testFlag(StatementProperty::InSystemHeader);
}

inline void setStatementInSystemHeader(const PStatement &statement, bool on) {
    statement->properties.setFlag(StatementProperty::InSystemHeader, on);
}

inline bool statementIsStatic(const PStatement &statement) {
    return statement->properties.testFlag(StatementProperty::Static);
}

inline void setStatementIsStatic(const PStatement &statement, bool on) {
    statement->properties.setFlag(StatementProperty::Static, on);
}

inline bool statementIsInherited(const PStatement &statement) {
    return statement->properties.testFlag(StatementProperty::Inherited);
}

class StatementFactory {
public:
    static PStatement create(StatementKind kind, const PStatement &parentScope,
                             const QString &command, const QString &fullName) {
        if (kind == StatementKind::Namespace)
            return createNamespace(parentScope, command, fullName);
        return createNormal(kind, parentScope, command, fullName);
    }
    static std::shared_ptr<Statement> createUnknown(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Unknown, parentScope, command, fullName); }
    static std::shared_ptr<NamespaceStatement> createNamespace(const PStatement &parentScope, const QString &command, const QString &fullName) { return createTyped<NamespaceStatement>(StatementKind::Namespace, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createNamespaceAlias(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::NamespaceAlias, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createClass(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Class, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createPreprocessor(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Preprocessor, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createEnumType(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::EnumType, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createEnumClassType(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::EnumClassType, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createTypedef(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Typedef, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createConstructor(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Constructor, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createDestructor(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Destructor, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createFunction(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Function, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createOverloadedOperator(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::OverloadedOperator, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createLiteralOperator(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::LiteralOperator, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createVariable(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Variable, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createGlobalVariable(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::GlobalVariable, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createLocalVariable(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::LocalVariable, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createEnum(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Enum, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createOperator(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Operator, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createParameter(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Parameter, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createBlock(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Block, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createLambda(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Lambda, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createLabel(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Label, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createUserCodeSnippet(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::UserCodeSnippet, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createKeyword(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Keyword, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createKeywordType(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::KeywordType, parentScope, command, fullName); }
    static std::shared_ptr<Statement> createAlias(const PStatement &parentScope, const QString &command, const QString &fullName) { return createNormal(StatementKind::Alias, parentScope, command, fullName); }
    static std::shared_ptr<InlineAliasStatement> createInlineAlias(const PStatement &parentScope, const QString &command, const QString &fullName, const PStatement &target) {
        auto statement = createTyped<InlineAliasStatement>(StatementKind::InlineAlias, parentScope, command, fullName);
        statement->target = target;
        return statement;
    }

private:
    static std::shared_ptr<Statement> createNormal(StatementKind kind, const PStatement &parentScope,
                                                   const QString &command, const QString &fullName) {
        return createTyped<Statement>(kind, parentScope, command, fullName);
    }
    template<typename T>
    static std::shared_ptr<T> createTyped(StatementKind kind, const PStatement &parentScope,
                                          const QString &command, const QString &fullName) {
        auto statement = std::make_shared<T>();
        statement->parentScope = parentScope;
        statement->command = command;
        statement->fullName = fullName;
        statement->kind = kind;
        statement->scope = StatementScope::Global;
        statement->accessibility = StatementAccessibility::None;
        statement->usageCount = 0;
        statement->matchPosTotal = 0;
        statement->matchPosSpan = 0;
        statement->firstMatchLength = 0;
        statement->caseMatched = 0;
        return statement;
    }
};

inline QString statementType(const PStatement &statement) {
    if (!statement || statement->kind == StatementKind::Namespace)
        return QString();
    auto normal = std::dynamic_pointer_cast<Statement>(statement);
    if (!normal)
        return QString();
    QString result;
    if (normal->declSpecifierProperties.testFlag(DeclSpecifierProperty::Static))
        result = "static ";
    else if (normal->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern))
        result = "extern ";
    if (normal->declSpecifierProperties.testFlag(DeclSpecifierProperty::Register))
        result += "register ";
    if (normal->declSpecifierProperties.testFlag(DeclSpecifierProperty::ThreadLocal))
        result += "thread_local ";
    if (normal->declSpecifierProperties.testFlag(DeclSpecifierProperty::Mutable))
        result += "mutable ";
    TypeInfo compatibleType = normal->typeInfo;
    bool hasArray = false;
    bool hasPointer = false;
    for (const TypeDerivation &derivation : compatibleType.derivations) {
        hasArray |= derivation.kind == TypeDerivationKind::Array;
        hasPointer |= derivation.kind == TypeDerivationKind::Pointer;
    }
    for (int i = compatibleType.derivations.count() - 1; i >= 0; --i) {
        if (statement->properties.testFlag(StatementProperty::FunctionPointer)
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
    std::weak_ptr<AbstractStatement> derivedClass;
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
    void setBranchVisibility(const QMap<int, bool> &branches) { mBranches = branches; }
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

    QString fileName() const { return mFileName; }
    const StatementMap& statements() const { return mStatements; }
    const QSet<QString>& usings() const { return mUsings; }
    const QStringList& directIncludes() const { return mDirectIncludes; }
    const QSet<QString>& includes() const { return mIncludes; }

    const QDateTime &lastModified() const { return mLastModified; }
    qint64 fileSize() const { return mFileSize; }
    void setFileMetadata(const QDateTime &lastModified, qint64 fileSize) {
        mLastModified = lastModified;
        mFileSize = fileSize;
    }
    bool includeProtected() const { return mIncludeProtected; }
    void setIncludeProtected(bool protectedFile) { mIncludeProtected = protectedFile; }

private:
    QString mFileName;
    QSet<QString> mIncludes;
    QStringList mDirectIncludes; //We need order here.
    QSet<QString> mUsings; // namespaces it usings
    StatementMap mStatements; // but we don't save temporary statements (full name as key)
    CppScopes mScopes; // int is start line of the statement scope
    QMap<int,bool> mBranches;
    QDateTime mLastModified;
    qint64 mFileSize = -1;
    bool mIncludeProtected = false;
};

using PParsedFileInfo = std::shared_ptr<ParsedFileInfo>;

class ReplayRecord {
public:
    ReplayRecord() = default;
    ReplayRecord(const ReplayRecord &)=delete;
    ReplayRecord& operator=(const ReplayRecord&)=delete;

    void setReplayCondition(const QHash<QString, QString> &userMacros,
                            const QStringList &boundaryPrefix) {
        mUserMacros = userMacros;
        mBoundaryPrefix = boundaryPrefix;
        mHasReplayCondition = true;
    }
    bool hasReplayCondition() const { return mHasReplayCondition; }
    const QHash<QString, QString> &userMacros() const { return mUserMacros; }
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

private:
    bool mHasReplayCondition = false;
    QHash<QString, QString> mUserMacros;
    QStringList mBoundaryPrefix;
    PDefineMap mDefines;
    PDefineMap mUndefines;
};

using PReplayRecord = std::shared_ptr<ReplayRecord>;

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
