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
#ifndef CPPEXPRESSIONEVALUATOR_H
#define CPPEXPRESSIONEVALUATOR_H

#include "cpptokenizer.h"
#include "typeoperationregistry.h"

class CppExpressionEvaluator
{
public:
    CppExpressionEvaluator(StatementModel &model, const CppTypeResolver &resolver,
                           const TypeOperationRegistry &operations,
                           const PTemplateBindingFrame &bindings = {});

    EvalResult evaluate(const CppTokenizer &tokenizer, const PStatement &context = {},
                        CppTypeResolver::EvaluationContext *evaluation = nullptr,
                        ExpressionEvalTrace *trace = nullptr) const;

private:
    friend class TestCppExpressionEvaluator;

    using TokenList = QVector<CppTokenizer::Token>;

    EvalResult evaluateBinary(const TokenList &tokens, int &index, const PStatement &context,
                              int minimumPrecedence,
                              CppTypeResolver::EvaluationContext *evaluation) const;
    EvalResult evaluateUnary(const TokenList &tokens, int &index, const PStatement &context,
                             CppTypeResolver::EvaluationContext *evaluation) const;
    EvalResult evaluatePostfix(const TokenList &tokens, int &index, const PStatement &context,
                               CppTypeResolver::EvaluationContext *evaluation) const;
    EvalResult evaluatePostfixTail(EvalResult result, const TokenList &tokens, int &index,
                                   const PStatement &context,
                                   CppTypeResolver::EvaluationContext *evaluation) const;
    EvalResult evaluatePrimary(const TokenList &tokens, int &index, const PStatement &context,
                               CppTypeResolver::EvaluationContext *evaluation) const;
    EvalResult evaluateLambda(const TokenList &tokens, int &index, const PStatement &context,
                              CppTypeResolver::EvaluationContext *evaluation) const;
    TypeInfo parseTypeText(const QString &text) const;
    EvalResult createResult(const PStatement &statement,
                            CppTypeResolver::EvaluationContext *evaluation) const;
    bool evaluateArguments(const TokenList &tokens, int &index, const PStatement &context,
                           QVector<TypeInfo> &arguments,
                           CppTypeResolver::EvaluationContext *evaluation) const;
    bool readTemplateArguments(const TokenList &tokens, int &index,
                               QVector<TemplateArgument> &arguments) const;
    EvalResult applyBinary(const EvalResult &left, const EvalResult &right,
                           const QString &operatorName,
                           CppTypeResolver::EvaluationContext *evaluation) const;
    PStatement findVisibleStatement(const QString &name, const PStatement &context) const;
    PStatement findVisibleFunction(const QString &name, const PStatement &context) const;
    PStatement findDirectStatement(const PStatement &scope, const QString &name) const;
    PStatement findDirectFunction(const PStatement &scope, const QString &name) const;
    ClassInstance memberReceiver(const EvalResult &result, bool arrow,
                                 CppTypeResolver::EvaluationContext *evaluation) const;
    EvalResult dereference(EvalResult result, CppTypeResolver::EvaluationContext *evaluation) const;
    TypeInfo addressOf(TypeInfo type, CppTypeResolver::EvaluationContext *evaluation) const;
    EvalResult applyIncrement(EvalResult result, const QString &operatorName,
                              CppTypeResolver::EvaluationContext *evaluation) const;
    TypeInfo removeReferences(TypeInfo type) const;
    int binaryPrecedence(const QString &operatorName) const;
    bool isRightAssociative(const QString &operatorName) const;
    int skipBalanced(const TokenList &tokens, int index) const;

    StatementModel &mModel;
    const CppTypeResolver &mResolver;
    const TypeOperationRegistry &mOperations;
    PTemplateBindingFrame mBindings;
};

#endif // CPPEXPRESSIONEVALUATOR_H
