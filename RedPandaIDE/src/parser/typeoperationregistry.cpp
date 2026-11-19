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

TypeInfo TypeOperationRegistry::containerValue(
        const TypeInfo &container, CppTypeResolver::EvaluationContext *evaluation) const
{
    TypeInfo value = mResolver.memberType(container, "value_type", evaluation);
    if (!value.qualifiedName.isEmpty())
        return value;
    return valueFromReference(containerReference(container, evaluation));
}

TypeInfo TypeOperationRegistry::containerReference(
        const TypeInfo &container, CppTypeResolver::EvaluationContext *evaluation) const
{
    TypeInfo iterator = mResolver.invokeResult(container, "begin", evaluation);
    return iteratorReference(iterator, evaluation);
}

TypeInfo TypeOperationRegistry::iteratorValue(
        const TypeInfo &iterator, CppTypeResolver::EvaluationContext *evaluation) const
{
    return valueFromReference(iteratorReference(iterator, evaluation));
}

TypeInfo TypeOperationRegistry::iteratorReference(
        const TypeInfo &iterator, CppTypeResolver::EvaluationContext *evaluation) const
{
    return mResolver.invokeResult(iterator, "operator*", evaluation);
}

TypeInfo TypeOperationRegistry::rangeValue(
        const TypeInfo &range, CppTypeResolver::EvaluationContext *evaluation) const
{
    return rangeElement(range, false, evaluation);
}

TypeInfo TypeOperationRegistry::rangeReference(
        const TypeInfo &range, CppTypeResolver::EvaluationContext *evaluation) const
{
    return rangeElement(range, true, evaluation);
}

TypeInfo TypeOperationRegistry::subscriptValue(
        const TypeInfo &receiver, CppTypeResolver::EvaluationContext *evaluation) const
{
    return valueFromReference(subscriptReference(receiver, evaluation));
}

TypeInfo TypeOperationRegistry::subscriptReference(
        const TypeInfo &receiver, CppTypeResolver::EvaluationContext *evaluation) const
{
    return mResolver.invokeResult(receiver, "operator[]", evaluation);
}

TypeInfo TypeOperationRegistry::invokeResult(
        const TypeInfo &callable, const QVector<TypeInfo> &arguments,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    if (arguments.count() == 1
            && (callable.qualifiedName == "std::ranges::filter_fn"
                || callable.qualifiedName == "std::ranges::transform_fn")) {
        QString closure = callable.qualifiedName == "std::ranges::filter_fn"
                ? "std::ranges::filter_closure" : "std::ranges::transform_closure";
        return makeView(closure, {TemplateArgument::fromType(arguments.first())}, evaluation);
    }
    if (arguments.isEmpty() && callable.qualifiedName == "std::ranges::join_fn")
        return makeView("std::ranges::join_closure", {}, evaluation);
    return mResolver.invokeResult(callable, "operator()", evaluation);
}

TypeInfo TypeOperationRegistry::explicitTemplateResult(
        const EvalResult &target, const QVector<TemplateArgument> &arguments,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    Q_UNUSED(evaluation);
    if (!target.baseStatement || target.baseStatement->fullName != "std::views::elements"
            || arguments.count() != 1 || arguments.first().kind != TemplateArgumentKind::NonType)
        return {};
    TypeInfo result = target.type;
    result.qualifiedName = "std::ranges::elements_fn";
    result.templateArguments = arguments;
    return result;
}

TypeInfo TypeOperationRegistry::pipeResult(
        const TypeInfo &input, const TypeInfo &adaptor,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    if (adaptor.qualifiedName == "std::ranges::filter_closure"
            || adaptor.qualifiedName == "std::ranges::transform_closure") {
        if (adaptor.templateArguments.count() != 1)
            return {};
        QString view = adaptor.qualifiedName == "std::ranges::filter_closure"
                ? "std::ranges::filter_view" : "std::ranges::transform_view";
        return makeView(view, {TemplateArgument::fromType(input), adaptor.templateArguments.first()},
                        evaluation);
    }
    if (adaptor.qualifiedName == "std::ranges::join_fn"
            || adaptor.qualifiedName == "std::ranges::join_closure")
        return makeView("std::ranges::join_view", {TemplateArgument::fromType(input)}, evaluation);
    if (adaptor.qualifiedName == "std::ranges::elements_fn"
            && adaptor.templateArguments.count() == 1)
        return makeView("std::ranges::elements_view",
                        {TemplateArgument::fromType(input), adaptor.templateArguments.first()},
                        evaluation);
    return {};
}

TypeInfo TypeOperationRegistry::makeView(
        const QString &name, const QVector<TemplateArgument> &arguments,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    Q_UNUSED(evaluation);
    TypeInfo result;
    result.qualifiedName = name;
    result.templateArguments = arguments;
    return result;
}

TypeInfo TypeOperationRegistry::rangeElement(
        const TypeInfo &range, bool reference,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    TypeInfo resolvedRange = mResolver.resolve(range, {}, {}, evaluation);
    const QVector<TemplateArgument> &arguments = resolvedRange.templateArguments;
    auto typeArgument = [&](int index) {
        return index < arguments.count() && arguments[index].kind == TemplateArgumentKind::Type
                && arguments[index].type ? *arguments[index].type : TypeInfo();
    };
    if (resolvedRange.qualifiedName == "std::ranges::iota_view")
        return typeArgument(0);
    if (resolvedRange.qualifiedName == "std::ranges::filter_view" && !arguments.isEmpty())
        return rangeElement(typeArgument(0), reference, evaluation);
    if (resolvedRange.qualifiedName == "std::ranges::transform_view" && arguments.count() >= 2)
        return callableResult(typeArgument(1), evaluation);
    if (resolvedRange.qualifiedName == "std::ranges::join_view" && !arguments.isEmpty()) {
        TypeInfo inner = rangeElement(typeArgument(0), reference, evaluation);
        return rangeElement(inner, reference, evaluation);
    }
    if (resolvedRange.qualifiedName == "std::ranges::elements_view" && arguments.count() >= 2
            && arguments[1].kind == TemplateArgumentKind::NonType) {
        TypeInfo tuple = rangeElement(typeArgument(0), false, evaluation);
        return tupleElement(tuple, arguments[1].constantExpression.toInt(), evaluation);
    }
    return reference ? containerReference(resolvedRange, evaluation)
                     : containerValue(resolvedRange, evaluation);
}

TypeInfo TypeOperationRegistry::callableResult(
        const TypeInfo &callable, CppTypeResolver::EvaluationContext *evaluation) const
{
    TypeInfo result = mResolver.invokeResult(callable, "operator()", evaluation);
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

TypeInfo TypeOperationRegistry::tupleElement(
        const TypeInfo &tuple, int index,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    if (index < 0)
        return {};
    TypeInfo resolved = mResolver.resolve(tuple, {}, {}, evaluation);
    if (resolved.qualifiedName == "std::array")
        index = 0;
    else if (resolved.qualifiedName != "std::tuple" && resolved.qualifiedName != "std::pair")
        return {};
    if (index >= resolved.templateArguments.count())
        return {};
    const TemplateArgument &argument = resolved.templateArguments[index];
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
