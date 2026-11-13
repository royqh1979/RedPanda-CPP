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
#include "cppexpressionevaluator.h"

namespace {
PStatement findInMap(const StatementMap &statements, const QString &name)
{
    const auto values = statements.values(name);
    for (const std::weak_ptr<Statement> &statement : values) {
        if (PStatement result = statement.lock())
            return result;
    }
    return nullptr;
}

bool isFunction(const PStatement &statement)
{
    if (!statement)
        return false;
    switch (statement->kind) {
    case StatementKind::Function:
    case StatementKind::OverloadedOperator:
    case StatementKind::LiteralOperator:
        return true;
    default:
        return false;
    }
}
}

CppExpressionEvaluator::CppExpressionEvaluator(const StatementModel &model,
                                               const CppTypeResolver &resolver,
                                               const TypeOperationRegistry &operations)
    : mModel(model), mResolver(resolver), mOperations(operations)
{
}

EvalResult CppExpressionEvaluator::evaluate(const CppTokenizer &tokenizer,
                                            const PStatement &context) const
{
    const QVector<CppTokenizer::Token> &tokens = tokenizer.tokens();
    if (tokens.isEmpty())
        return {};

    int index = 0;
    bool prefixDereference = false;
    while (index < tokens.count() && tokens[index].text == "*") {
        prefixDereference = true;
        ++index;
    }
    if (index >= tokens.count())
        return {};

    EvalResult result;
    const CppTokenizer::Token &initial = tokens[index++];
    switch (initial.kind) {
    case CppTokenizer::TokenKind::IntegerLiteral:
        result.type.qualifiedName = "int";
        result.kind = EvalStatementKind::Literal;
        break;
    case CppTokenizer::TokenKind::FloatingLiteral:
        result.type.qualifiedName = "double";
        result.kind = EvalStatementKind::Literal;
        break;
    case CppTokenizer::TokenKind::CharacterLiteral:
        result.type.qualifiedName = "char";
        result.kind = EvalStatementKind::Literal;
        break;
    case CppTokenizer::TokenKind::Identifier: {
        if (initial.text == "true" || initial.text == "false") {
            result.type.qualifiedName = "bool";
            result.kind = EvalStatementKind::Literal;
            break;
        }
        PStatement statement = findVisibleStatement(initial.text, context);
        if (!statement)
            return {};
        result.baseStatement = statement;
        result.kind = isFunction(statement) ? EvalStatementKind::Function
                                            : EvalStatementKind::Variable;
        result.type = mResolver.resolve(statement->typeInfo, {}, statement->parentScope.lock());
        break;
    }
    default:
        return {};
    }

    while (index < tokens.count() && result.isValid()) {
        const QString &operation = tokens[index].text;
        if (operation == "." || operation == "->") {
            bool call = false;
            bool arrow = operation == "->";
            ++index;
            if (index >= tokens.count())
                return {};
            QString memberName = tokens[index++].text;
            TypeInfo receiver = memberReceiver(result.type, arrow);
            if (index < tokens.count() && tokens[index].text == "(") {
                call = true;
                index = skipBalanced(tokens, index);
            }
            result.type = call ? mResolver.invokeResult(receiver, memberName)
                               : mResolver.memberType(receiver, memberName);
            result.kind = EvalStatementKind::Variable;
            continue;
        }
        if (operation == "[") {
            result.type = removeReferences(result.type);
            if (!result.type.derivations.isEmpty()
                    && result.type.derivations.first().kind == TypeDerivationKind::Array) {
                result.type.derivations.removeFirst();
                result.type.derivations.prepend(TypeDerivation::lvalueReference());
            } else {
                result.type = mOperations.subscriptReference(result.type);
            }
            index = skipBalanced(tokens, index);
            result.kind = EvalStatementKind::Variable;
            continue;
        }
        if (operation == "(") {
            index = skipBalanced(tokens, index);
            if (result.kind != EvalStatementKind::Function)
                result.type = mResolver.invokeResult(result.type, "operator()");
            result.kind = EvalStatementKind::Variable;
            continue;
        }
        break;
    }

    if (prefixDereference && result.isValid())
        result.type = dereference(result.type);
    return result;
}

PStatement CppExpressionEvaluator::findVisibleStatement(const QString &name,
                                                        const PStatement &context) const
{
    PStatement scope = context;
    while (scope) {
        if (PStatement result = findInMap(scope->children, name))
            return result;
        scope = scope->parentScope.lock();
    }
    return findInMap(mModel.childrenStatements(), name);
}

TypeInfo CppExpressionEvaluator::memberReceiver(TypeInfo type, bool arrow) const
{
    type = removeReferences(type);
    if (!arrow)
        return type;
    if (!type.derivations.isEmpty()
            && type.derivations.first().kind == TypeDerivationKind::Pointer) {
        type.derivations.removeFirst();
        return type;
    }
    type = mResolver.invokeResult(type, "operator->");
    type = removeReferences(type);
    if (!type.derivations.isEmpty()
            && type.derivations.first().kind == TypeDerivationKind::Pointer)
        type.derivations.removeFirst();
    return type;
}

TypeInfo CppExpressionEvaluator::dereference(TypeInfo type) const
{
    type = removeReferences(type);
    if (!type.derivations.isEmpty()
            && type.derivations.first().kind == TypeDerivationKind::Pointer) {
        type.derivations.removeFirst();
        type.derivations.prepend(TypeDerivation::lvalueReference());
        return type;
    }
    return mOperations.iteratorReference(type);
}

TypeInfo CppExpressionEvaluator::removeReferences(TypeInfo type) const
{
    while (!type.derivations.isEmpty()
            && (type.derivations.first().kind == TypeDerivationKind::LvalueReference
                || type.derivations.first().kind == TypeDerivationKind::RvalueReference))
        type.derivations.removeFirst();
    return type;
}

int CppExpressionEvaluator::skipBalanced(const QVector<CppTokenizer::Token> &tokens, int index) const
{
    if (index < 0 || index >= tokens.count())
        return tokens.count();
    int target = tokens[index].matchIndex;
    return target > index && target < tokens.count() ? target + 1 : index + 1;
}
