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
    return containerValue(range);
}

TypeInfo TypeOperationRegistry::rangeReference(const TypeInfo &range) const
{
    return containerReference(range);
}

TypeInfo TypeOperationRegistry::subscriptValue(const TypeInfo &receiver) const
{
    return valueFromReference(subscriptReference(receiver));
}

TypeInfo TypeOperationRegistry::subscriptReference(const TypeInfo &receiver) const
{
    return mResolver.invokeResult(receiver, "operator[]");
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
