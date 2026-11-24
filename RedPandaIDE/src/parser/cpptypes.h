/*
 * Copyright (C) 2026 Roy Qu (royqh1979@gmail.com)
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
#ifndef CPPTYPES_H
#define CPPTYPES_H

#include <QFlags>
#include <QString>
#include <QStringList>
#include <QVector>
#include <memory>

struct AbstractStatement;
struct LambdaInfo;
struct TypeInfo;
using PTypeInfo = std::shared_ptr<TypeInfo>;

enum class CppCompilerType {
    GCC,
    Clang
};

namespace CppVersion {
inline constexpr int Cpp11 = 201103;
inline constexpr int Cpp17 = 201703;
inline constexpr int Cpp20 = 202002;
inline constexpr int Cpp23 = 202302;
}

struct CppEvaluationOptions {
    CppCompilerType compilerType = CppCompilerType::GCC;
    int cppVersion = CppVersion::Cpp17;
};

struct SourcePosition {
    int line = -1;
    int ch = -1;

    bool isValid() const { return line >= 0 && ch >= 0; }
    bool operator==(const SourcePosition &other) const {
        return line == other.line && ch == other.ch;
    }
    bool operator<(const SourcePosition &other) const {
        return line < other.line || (line == other.line && ch < other.ch);
    }
    bool operator<=(const SourcePosition &other) const {
        return *this < other || *this == other;
    }
};

struct SourceRange {
    SourcePosition start;
    SourcePosition end;

    bool isValid() const { return start.isValid() && end.isValid() && start <= end; }
    bool operator==(const SourceRange &other) const {
        return start == other.start && end == other.end;
    }
    bool contains(const SourcePosition &position) const {
        return isValid() && start <= position && position <= end;
    }
};

enum class TypeQualifier {
    None = 0x0,
    Const = 0x1,
    Volatile = 0x2
};
Q_DECLARE_FLAGS(TypeQualifiers, TypeQualifier)
Q_DECLARE_OPERATORS_FOR_FLAGS(TypeQualifiers)

enum class TemplateArgumentKind {
    Type,
    NonType
};

struct TemplateArgument {
    TemplateArgumentKind kind = TemplateArgumentKind::NonType;
    PTypeInfo type;
    QString constantExpression;

    static TemplateArgument fromType(const TypeInfo &type);
    static TemplateArgument fromConstant(const QString &expression);
    QString toString() const;
};

struct ParameterInfo {
    PTypeInfo type;
    QString name;
    SourcePosition position;
    QString defaultArgumentText;
    QString normalizedText;
    bool normalizationChanged = false;
    bool parameterPack = false;

    QString toString(bool includeName = true) const;
};

enum class ReferenceQualifier {
    None,
    Lvalue,
    Rvalue
};

enum class FunctionProperty {
    None = 0x0,
    Variadic = 0x1,
    Override = 0x2
};
Q_DECLARE_FLAGS(FunctionProperties, FunctionProperty)
Q_DECLARE_OPERATORS_FOR_FLAGS(FunctionProperties)

enum class TypeDerivationKind {
    Pointer,
    LvalueReference,
    RvalueReference,
    MemberPointer,
    Array,
    Function
};

enum class NoexceptKind {
    None,
    Always,
    Expression
};

struct TypeDerivation {
    TypeDerivationKind kind = TypeDerivationKind::Pointer;
    TypeQualifiers qualifiers = TypeQualifier::None;
    QString memberClass;
    QString text;
    QVector<ParameterInfo> parameters;
    FunctionProperties functionProperties = FunctionProperty::None;
    ReferenceQualifier refQualifier = ReferenceQualifier::None;
    NoexceptKind noexceptKind = NoexceptKind::None;
    QString noexceptExpression;
    QString trailingReturnType;
    QString requiresClause;
    QString normalizedArgs;

    static TypeDerivation pointer(TypeQualifiers qualifiers = TypeQualifier::None);
    static TypeDerivation lvalueReference();
    static TypeDerivation rvalueReference();
    static TypeDerivation memberPointer(const QString &memberClass,
                                        TypeQualifiers qualifiers = TypeQualifier::None);
    static TypeDerivation array(const QString &text);
    static TypeDerivation function(const QVector<ParameterInfo> &parameters = {});
};

struct TypeInfo {
    QString qualifiedName;
    QString qualifiedNameText;
    std::weak_ptr<AbstractStatement> declaration;
    QVector<TemplateArgument> templateArguments;
    QVector<TypeDerivation> derivations;
    TypeQualifiers qualifiers = TypeQualifier::None;
    ReferenceQualifier valueCategory = ReferenceQualifier::None;

    QString toString() const;
};

enum class ArithmeticCategory {
    None,
    Integral,
    Floating
};

ArithmeticCategory arithmeticCategory(const TypeInfo &type);
TypeInfo canonicalArithmeticType(const TypeInfo &type);
TypeInfo promotedArithmeticType(const TypeInfo &type);
TypeInfo usualArithmeticType(const TypeInfo &left, const TypeInfo &right);

enum class DeclaratorDefinitionKind {
    None,
    Defaulted,
    Deleted
};

struct DeclaratorInfo {
    QString name;
    QString qualifier;
    bool globallyQualified = false;
    bool parameterPack = false;
    SourcePosition position;
    SourcePosition namePosition;
    QVector<TypeDerivation> derivations;
    QString initializerText;
    SourcePosition initializerPosition;
    QVector<std::shared_ptr<LambdaInfo>> lambdas;
    DeclaratorDefinitionKind definitionKind = DeclaratorDefinitionKind::None;
    int structuredBindingIndex = -1;
    TypeInfo templatePattern;

    QString toString() const;
};

enum class DeclSpecifierProperty {
    None = 0x0,
    Static = 0x1,
    Extern = 0x2,
    Register = 0x4,
    ThreadLocal = 0x8,
    Mutable = 0x10,
    Inline = 0x20,
    Virtual = 0x40,
    Friend = 0x80,
    Constexpr = 0x100,
    Consteval = 0x200,
    Constinit = 0x400,
    Explicit = 0x800,
    Typedef = 0x1000
};
Q_DECLARE_FLAGS(DeclSpecifierProperties, DeclSpecifierProperty)
Q_DECLARE_OPERATORS_FOR_FLAGS(DeclSpecifierProperties)

struct DeclSpecifierInfo {
    TypeInfo type;
    DeclSpecifierProperties properties = DeclSpecifierProperty::None;
};

enum class TemplateParameterKind {
    Type,
    NonType,
    TemplateTemplate
};

struct TemplateParameterInfo {
    QString name;
    TemplateParameterKind kind = TemplateParameterKind::Type;
    bool parameterPack = false;
    PTypeInfo type;
    QString defaultArgumentText;
};

enum class TemplateDeclarationKind {
    None,
    Primary,
    PartialSpecialization,
    ExplicitSpecialization
};

enum class DeclarationKind {
    Simple,
    Class,
    Enum,
    Namespace,
    NamespaceAlias,
    Alias,
    Using,
    Block,
    Label,
    Expression
};

enum class UsingDeclarationKind {
    Namespace,
    Enum,
    Member
};

enum class ClassKey {
    None,
    Class,
    Struct,
    Union
};

enum class DeclarationAccessibility {
    Default,
    Public,
    Protected,
    Private
};

struct EnumeratorInfo {
    QString name;
    QString value;
    SourceRange range;
};

struct DeclarationInfo {
    QString fileName;
    SourceRange range;
    SourceRange bodyRange;
    DeclSpecifierInfo specifiers;
    QVector<DeclaratorInfo> declarators;
    TemplateDeclarationKind templateKind = TemplateDeclarationKind::None;
    QString templateRequiresClause;
    QVector<TemplateParameterInfo> templateParameters;
    DeclarationKind kind = DeclarationKind::Simple;
    UsingDeclarationKind usingKind = UsingDeclarationKind::Member;
    ClassKey classKey = ClassKey::None;
    DeclarationAccessibility accessibility = DeclarationAccessibility::Default;
    bool structuredBinding = false;
    bool enumScoped = false;
    QString rangeExpression;
    SourcePosition rangeExpressionPosition;
    QStringList baseClasses;
    QVector<EnumeratorInfo> enumerators;
    QVector<std::shared_ptr<LambdaInfo>> expressionLambdas;
    QVector<DeclarationInfo> children;
};

#endif // CPPTYPES_H
