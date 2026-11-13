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
#include "cpptypes.h"

#include <QStringList>

namespace {
bool isSuffix(TypeDerivationKind kind)
{
    return kind == TypeDerivationKind::Array || kind == TypeDerivationKind::Function;
}

QString renderParameters(const TypeDerivation &derivation)
{
    QStringList parts;
    for (const ParameterInfo &parameter : derivation.parameters)
        parts.append(parameter.toString());
    if (derivation.functionProperties.testFlag(FunctionProperty::Variadic))
        parts.append("...");
    return parts.join(", ");
}

QString renderDeclarator(const QString &name, const QVector<TypeDerivation> &derivations,
                         bool includeTrailingReturn)
{
    QString result = name;
    for (int i = 0; i < derivations.count(); ++i) {
        const TypeDerivation &derivation = derivations[i];
        switch (derivation.kind) {
        case TypeDerivationKind::Pointer:
        case TypeDerivationKind::LvalueReference:
        case TypeDerivationKind::RvalueReference:
        case TypeDerivationKind::MemberPointer: {
            QString prefix;
            if (derivation.kind == TypeDerivationKind::Pointer)
                prefix = "*";
            else if (derivation.kind == TypeDerivationKind::LvalueReference)
                prefix = "&";
            else if (derivation.kind == TypeDerivationKind::RvalueReference)
                prefix = "&&";
            else
                prefix = derivation.memberClass + "::*";
            if (derivation.qualifiers.testFlag(TypeQualifier::Const))
                prefix += "const";
            if (derivation.qualifiers.testFlag(TypeQualifier::Volatile))
                prefix += "volatile";
            if (!result.isEmpty() && prefix.back() != '*' && prefix.back() != '&')
                prefix += ' ';
            result = prefix + result;
            if (i + 1 < derivations.count() && isSuffix(derivations[i + 1].kind))
                result = "(" + result + ")";
            break;
        }
        case TypeDerivationKind::Array:
            result += derivation.text;
            break;
        case TypeDerivationKind::Function:
            result += "(" + renderParameters(derivation) + ")";
            if (derivation.qualifiers.testFlag(TypeQualifier::Const))
                result += " const";
            if (derivation.qualifiers.testFlag(TypeQualifier::Volatile))
                result += " volatile";
            if (derivation.refQualifier == ReferenceQualifier::Lvalue)
                result += " &";
            else if (derivation.refQualifier == ReferenceQualifier::Rvalue)
                result += " &&";
            if (includeTrailingReturn && !derivation.trailingReturnType.isEmpty())
                result += " -> " + derivation.trailingReturnType;
            break;
        }
    }
    return result;
}

QString renderType(const TypeInfo &type, const QString &name = QString())
{
    QString result;
    if (type.qualifiers.testFlag(TypeQualifier::Const))
        result += "const ";
    if (type.qualifiers.testFlag(TypeQualifier::Volatile))
        result += "volatile ";
    result += type.qualifiedName;
    if (!type.templateArguments.isEmpty()) {
        QStringList arguments;
        for (const TemplateArgument &argument : type.templateArguments)
            arguments.append(argument.toString());
        result += "<" + arguments.join(",") + ">";
    }
    QString declarator = renderDeclarator(name, type.derivations, true);
    if (!declarator.isEmpty()) {
        if (!result.isEmpty())
            result += ' ';
        result += declarator;
    }
    return result.trimmed();
}
}

TemplateArgument TemplateArgument::fromType(const TypeInfo &type)
{
    TemplateArgument result;
    result.kind = TemplateArgumentKind::Type;
    result.type = std::make_shared<TypeInfo>(type);
    return result;
}

TemplateArgument TemplateArgument::fromConstant(const QString &expression)
{
    TemplateArgument result;
    result.constantExpression = expression;
    return result;
}

QString TemplateArgument::toString() const
{
    if (kind == TemplateArgumentKind::Type && type)
        return type->toString();
    return constantExpression;
}

QString ParameterInfo::toString(bool includeName) const
{
    if (!type)
        return includeName ? name : QString();
    return renderType(*type, includeName ? name : QString());
}

TypeDerivation TypeDerivation::pointer(TypeQualifiers qualifiers)
{
    TypeDerivation result;
    result.kind = TypeDerivationKind::Pointer;
    result.qualifiers = qualifiers;
    return result;
}

TypeDerivation TypeDerivation::lvalueReference()
{
    TypeDerivation result;
    result.kind = TypeDerivationKind::LvalueReference;
    return result;
}

TypeDerivation TypeDerivation::rvalueReference()
{
    TypeDerivation result;
    result.kind = TypeDerivationKind::RvalueReference;
    return result;
}

TypeDerivation TypeDerivation::memberPointer(const QString &memberClass,
                                              TypeQualifiers qualifiers)
{
    TypeDerivation result = pointer(qualifiers);
    result.kind = TypeDerivationKind::MemberPointer;
    result.memberClass = memberClass;
    return result;
}

TypeDerivation TypeDerivation::array(const QString &text)
{
    TypeDerivation result;
    result.kind = TypeDerivationKind::Array;
    result.text = text;
    return result;
}

TypeDerivation TypeDerivation::function(const QVector<ParameterInfo> &parameters)
{
    TypeDerivation result;
    result.kind = TypeDerivationKind::Function;
    result.parameters = parameters;
    return result;
}

QString TypeInfo::toString() const
{
    return renderType(*this);
}

QString DeclaratorInfo::toString() const
{
    QString fullName;
    if (globallyQualified)
        fullName = "::";
    if (!qualifier.isEmpty()) {
        fullName += qualifier;
        if (!name.isEmpty())
            fullName += "::";
    }
    fullName += name;
    return renderDeclarator(fullName, derivations, true);
}
