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

#include <QHash>
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
    if (name.isEmpty() && type.qualifiers == TypeQualifiers()
            && type.derivations.isEmpty() && type.templateArguments.isEmpty()) {
        return type.qualifiedNameText.contains('<')
                ? type.qualifiedNameText.trimmed() : type.qualifiedName.trimmed();
    }
    QString result;
    if (type.qualifiers.testFlag(TypeQualifier::Const))
        result += "const ";
    if (type.qualifiers.testFlag(TypeQualifier::Volatile))
        result += "volatile ";
    bool useQualifiedNameText = type.templateArguments.isEmpty()
            && type.qualifiedNameText.contains('<');
    result += useQualifiedNameText ? type.qualifiedNameText : type.qualifiedName;
    if (!useQualifiedNameText && !type.templateArguments.isEmpty()) {
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

namespace {
struct ArithmeticInfo {
    ArithmeticCategory category = ArithmeticCategory::None;
    QString name;
    int rank = 0;
    int bits = 0;
    bool isUnsigned = false;
};

QString canonicalArithmeticName(const QString &name)
{
    static const QHash<QString, QString> aliases{
        {"signed", "int"}, {"signed int", "int"}, {"unsigned", "unsigned int"},
        {"short int", "short"}, {"signed short", "short"}, {"signed short int", "short"},
        {"unsigned short int", "unsigned short"}, {"short unsigned", "unsigned short"},
        {"short unsigned int", "unsigned short"},
        {"long int", "long"}, {"signed long", "long"}, {"signed long int", "long"},
        {"unsigned long int", "unsigned long"}, {"long unsigned", "unsigned long"},
        {"long unsigned int", "unsigned long"},
        {"long long int", "long long"}, {"signed long long", "long long"},
        {"signed long long int", "long long"},
        {"unsigned long long int", "unsigned long long"},
        {"long long unsigned", "unsigned long long"},
        {"long long unsigned int", "unsigned long long"}
    };
    const QString simplified = name.simplified();
    return aliases.value(simplified, simplified);
}

ArithmeticInfo arithmeticInfo(const TypeInfo &type)
{
    if (!type.derivations.isEmpty())
        return {};
    ArithmeticInfo result;
    result.name = canonicalArithmeticName(type.qualifiedName);
    result.isUnsigned = result.name.startsWith("unsigned ") || result.name == "char8_t"
            || result.name == "char16_t" || result.name == "char32_t";
    if (result.name == "long double" || result.name == "double" || result.name == "float") {
        result.category = ArithmeticCategory::Floating;
        result.rank = result.name == "long double" ? 90 : result.name == "double" ? 80 : 70;
        return result;
    }
    static const QHash<QString, int> ranks{
        {"bool", 10}, {"char", 20}, {"signed char", 20}, {"unsigned char", 20},
        {"wchar_t", 20}, {"char8_t", 20}, {"char16_t", 20}, {"char32_t", 20},
        {"short", 30}, {"unsigned short", 30}, {"int", 40}, {"unsigned int", 40},
        {"long", 50}, {"unsigned long", 50}, {"long long", 60}, {"unsigned long long", 60}
    };
    if (!ranks.contains(result.name))
        return {};
    result.category = ArithmeticCategory::Integral;
    result.rank = ranks[result.name];
    if (result.rank <= 20)
        result.bits = result.name == "char32_t" ? 32 : result.name == "wchar_t" ? 16 : 8;
    else if (result.rank == 30)
        result.bits = 16;
    else if (result.rank == 40)
        result.bits = 32;
    else if (result.rank == 50) {
#ifdef Q_OS_WIN
        result.bits = 32;
#else
        result.bits = sizeof(long) * 8;
#endif
    } else {
        result.bits = 64;
    }
    return result;
}

TypeInfo arithmeticType(const QString &name)
{
    TypeInfo result;
    result.qualifiedName = name;
    return result;
}

QString unsignedArithmeticName(const QString &name)
{
    if (name.startsWith("unsigned "))
        return name;
    return "unsigned " + name;
}
}

ArithmeticCategory arithmeticCategory(const TypeInfo &type)
{
    return arithmeticInfo(type).category;
}

TypeInfo canonicalArithmeticType(const TypeInfo &type)
{
    ArithmeticInfo info = arithmeticInfo(type);
    return info.category == ArithmeticCategory::None ? TypeInfo() : arithmeticType(info.name);
}

TypeInfo promotedArithmeticType(const TypeInfo &type)
{
    ArithmeticInfo info = arithmeticInfo(type);
    if (info.category == ArithmeticCategory::None)
        return {};
    if (info.category == ArithmeticCategory::Floating)
        return arithmeticType(info.name == "float" ? "double" : info.name);
    if (info.name == "char32_t")
        return arithmeticType("unsigned int");
    return arithmeticType(info.rank < 40 ? "int" : info.name);
}

TypeInfo usualArithmeticType(const TypeInfo &left, const TypeInfo &right)
{
    ArithmeticInfo leftInfo = arithmeticInfo(left);
    ArithmeticInfo rightInfo = arithmeticInfo(right);
    if (leftInfo.category == ArithmeticCategory::None || rightInfo.category == ArithmeticCategory::None)
        return {};
    if (leftInfo.category == ArithmeticCategory::Floating
            || rightInfo.category == ArithmeticCategory::Floating) {
        const ArithmeticInfo &selected = leftInfo.rank >= rightInfo.rank ? leftInfo : rightInfo;
        return arithmeticType(selected.name);
    }
    TypeInfo leftPromoted = promotedArithmeticType(left);
    TypeInfo rightPromoted = promotedArithmeticType(right);
    leftInfo = arithmeticInfo(leftPromoted);
    rightInfo = arithmeticInfo(rightPromoted);
    if (leftInfo.name == rightInfo.name)
        return arithmeticType(leftInfo.name);
    if (leftInfo.rank == rightInfo.rank)
        return arithmeticType(leftInfo.isUnsigned ? leftInfo.name : rightInfo.name);
    const ArithmeticInfo &higher = leftInfo.rank > rightInfo.rank ? leftInfo : rightInfo;
    const ArithmeticInfo &lower = leftInfo.rank > rightInfo.rank ? rightInfo : leftInfo;
    if (higher.isUnsigned)
        return arithmeticType(higher.name);
    if (lower.isUnsigned && higher.bits <= lower.bits)
        return arithmeticType(unsignedArithmeticName(higher.name));
    return arithmeticType(higher.name);
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
