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
#ifndef TYPEOPERATIONREGISTRY_H
#define TYPEOPERATIONREGISTRY_H

#include "cpptyperesolver.h"

class TypeOperationRegistry
{
public:
    explicit TypeOperationRegistry(const CppTypeResolver &resolver);

    TypeInfo containerValue(const TypeInfo &container) const;
    TypeInfo containerReference(const TypeInfo &container) const;
    TypeInfo iteratorValue(const TypeInfo &iterator) const;
    TypeInfo iteratorReference(const TypeInfo &iterator) const;
    TypeInfo rangeValue(const TypeInfo &range) const;
    TypeInfo rangeReference(const TypeInfo &range) const;
    TypeInfo subscriptValue(const TypeInfo &receiver) const;
    TypeInfo subscriptReference(const TypeInfo &receiver) const;

private:
    TypeInfo valueFromReference(TypeInfo type) const;

    const CppTypeResolver &mResolver;
};

#endif // TYPEOPERATIONREGISTRY_H
