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

    TypeInfo containerValue(const TypeInfo &container,
                            CppTypeResolver::EvaluationContext *evaluation = nullptr) const;
    TypeInfo containerReference(const TypeInfo &container,
                                CppTypeResolver::EvaluationContext *evaluation = nullptr) const;
    TypeInfo iteratorValue(const TypeInfo &iterator,
                           CppTypeResolver::EvaluationContext *evaluation = nullptr) const;
    TypeInfo iteratorReference(const TypeInfo &iterator,
                               CppTypeResolver::EvaluationContext *evaluation = nullptr) const;
    TypeInfo rangeValue(const TypeInfo &range,
                        CppTypeResolver::EvaluationContext *evaluation = nullptr) const;
    TypeInfo rangeReference(const TypeInfo &range,
                            CppTypeResolver::EvaluationContext *evaluation = nullptr) const;
    TypeInfo subscriptValue(const TypeInfo &receiver,
                            CppTypeResolver::EvaluationContext *evaluation = nullptr) const;
    TypeInfo subscriptReference(const TypeInfo &receiver,
                                CppTypeResolver::EvaluationContext *evaluation = nullptr) const;
    TypeInfo invokeResult(const TypeInfo &callable, const QVector<TypeInfo> &arguments,
                          CppTypeResolver::EvaluationContext *evaluation = nullptr,
                          CallResolution *invocation = nullptr) const;
    TypeInfo explicitTemplateResult(const EvalResult &target,
                                    const QVector<TemplateArgument> &arguments,
                                    CppTypeResolver::EvaluationContext *evaluation = nullptr) const;
    TypeInfo pipeResult(const TypeInfo &input, const TypeInfo &adaptor,
                        CppTypeResolver::EvaluationContext *evaluation = nullptr) const;

private:
    friend class TestTypeOperationRegistry;

    TypeInfo makeView(const QString &name, const QVector<TemplateArgument> &arguments,
                      CppTypeResolver::EvaluationContext *evaluation) const;
    TypeInfo rangeElement(const TypeInfo &range, bool reference,
                          CppTypeResolver::EvaluationContext *evaluation) const;
    TypeInfo callableResult(const TypeInfo &callable,
                            CppTypeResolver::EvaluationContext *evaluation) const;
    TypeInfo tupleElement(const TypeInfo &tuple, int index,
                          CppTypeResolver::EvaluationContext *evaluation) const;
    TypeInfo valueFromReference(TypeInfo type) const;

    const CppTypeResolver &mResolver;
};

#endif // TYPEOPERATIONREGISTRY_H
