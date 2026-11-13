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
#include <QVector>
#include <memory>

struct Statement;
struct TypeInfo;
using PTypeInfo = std::shared_ptr<TypeInfo>;

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
    int line = 0;

    QString toString(bool includeName = true) const;
};

enum class ReferenceQualifier {
    None,
    Lvalue,
    Rvalue
};

enum class FunctionProperty {
    None = 0x0,
    Variadic = 0x1
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

struct TypeDerivation {
    TypeDerivationKind kind = TypeDerivationKind::Pointer;
    TypeQualifiers qualifiers = TypeQualifier::None;
    QString memberClass;
    QString text;
    QVector<ParameterInfo> parameters;
    FunctionProperties functionProperties = FunctionProperty::None;
    ReferenceQualifier refQualifier = ReferenceQualifier::None;
    QString trailingReturnType;

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
    std::weak_ptr<Statement> declaration;
    QVector<TemplateArgument> templateArguments;
    QVector<TypeDerivation> derivations;
    TypeQualifiers qualifiers = TypeQualifier::None;

    QString toString() const;
};

struct DeclaratorInfo {
    QString name;
    QString qualifier;
    bool globallyQualified = false;
    int line = 0;
    QVector<TypeDerivation> derivations;

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
    Explicit = 0x800
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
};

enum class TemplateDeclarationKind {
    None,
    Primary,
    PartialSpecialization,
    ExplicitSpecialization
};

struct DeclarationInfo {
    DeclSpecifierInfo specifiers;
    QVector<DeclaratorInfo> declarators;
    TemplateDeclarationKind templateKind = TemplateDeclarationKind::None;
    QVector<TemplateParameterInfo> templateParameters;
};

#endif // CPPTYPES_H
