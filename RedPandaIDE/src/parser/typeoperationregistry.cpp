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
#include "typeoperationregistry.h"

TypeOperationRegistry::TypeOperationRegistry(const CppTypeResolver &resolver)
    : mResolver(resolver)
{
}

TypeInfo TypeOperationRegistry::containerValue(const TypeInfo &container) const
{
    TypeInfo value = mResolver.memberType(container, "value_type");
    if (!value.qualifiedName.isEmpty())
        return value;
    return valueFromReference(containerReference(container));
}

TypeInfo TypeOperationRegistry::containerReference(const TypeInfo &container) const
{
    return mResolver.invokeResult(container, "front");
}

TypeInfo TypeOperationRegistry::iteratorValue(const TypeInfo &iterator) const
{
    return valueFromReference(iteratorReference(iterator));
}

TypeInfo TypeOperationRegistry::iteratorReference(const TypeInfo &iterator) const
{
    return mResolver.invokeResult(iterator, "operator*");
}

TypeInfo TypeOperationRegistry::rangeValue(const TypeInfo &range) const
{
    return rangeElement(range, false);
}

TypeInfo TypeOperationRegistry::rangeReference(const TypeInfo &range) const
{
    return rangeElement(range, true);
}

TypeInfo TypeOperationRegistry::subscriptValue(const TypeInfo &receiver) const
{
    return valueFromReference(subscriptReference(receiver));
}

TypeInfo TypeOperationRegistry::subscriptReference(const TypeInfo &receiver) const
{
    return mResolver.invokeResult(receiver, "operator[]");
}

TypeInfo TypeOperationRegistry::rangeElement(const TypeInfo &range, bool reference) const
{
    const QVector<TemplateArgument> &arguments = range.templateArguments;
    auto typeArgument = [&](int index) {
        return index < arguments.count() && arguments[index].kind == TemplateArgumentKind::Type
                && arguments[index].type ? *arguments[index].type : TypeInfo();
    };
    if (range.qualifiedName == "std::ranges::iota_view")
        return typeArgument(0);
    if (range.qualifiedName == "std::ranges::filter_view" && !arguments.isEmpty())
        return rangeElement(typeArgument(0), reference);
    if (range.qualifiedName == "std::ranges::transform_view" && arguments.count() >= 2)
        return callableResult(typeArgument(1));
    if (range.qualifiedName == "std::ranges::join_view" && !arguments.isEmpty()) {
        TypeInfo inner = rangeElement(typeArgument(0), reference);
        return rangeElement(inner, reference);
    }
    if (range.qualifiedName == "std::ranges::elements_view" && arguments.count() >= 2
            && arguments[1].kind == TemplateArgumentKind::NonType) {
        TypeInfo tuple = rangeElement(typeArgument(0), false);
        return tupleElement(tuple, arguments[1].constantExpression.toInt());
    }
    return reference ? containerReference(range) : containerValue(range);
}

TypeInfo TypeOperationRegistry::callableResult(const TypeInfo &callable) const
{
    TypeInfo result = mResolver.invokeResult(callable, "operator()");
    if (!result.qualifiedName.isEmpty())
        return result;
    for (int i = 0; i < callable.derivations.count(); ++i) {
        const TypeDerivation &function = callable.derivations[i];
        if (function.kind != TypeDerivationKind::Function)
            continue;
        result = callable;
        result.derivations.remove(0, i + 1);
        if (result.qualifiedName == "auto" && !function.trailingReturnType.isEmpty())
            result.qualifiedName = function.trailingReturnType;
        return result;
    }
    return {};
}

TypeInfo TypeOperationRegistry::tupleElement(const TypeInfo &tuple, int index) const
{
    if (index < 0)
        return {};
    if (tuple.qualifiedName == "std::array")
        index = 0;
    else if (tuple.qualifiedName != "std::tuple" && tuple.qualifiedName != "std::pair")
        return {};
    if (index >= tuple.templateArguments.count())
        return {};
    const TemplateArgument &argument = tuple.templateArguments[index];
    return argument.kind == TemplateArgumentKind::Type && argument.type
            ? *argument.type : TypeInfo();
}

TypeInfo TypeOperationRegistry::valueFromReference(TypeInfo type) const
{
    if (!type.derivations.isEmpty()
            && (type.derivations.first().kind == TypeDerivationKind::LvalueReference
                || type.derivations.first().kind == TypeDerivationKind::RvalueReference))
        type.derivations.removeFirst();
    type.qualifiers = TypeQualifier::None;
    return type;
}
