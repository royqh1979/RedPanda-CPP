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
    CppExpressionEvaluator(const StatementModel &model, const CppTypeResolver &resolver,
                           const TypeOperationRegistry &operations);

    EvalResult evaluate(const CppTokenizer &tokenizer, const PStatement &context = {}) const;

private:
    PStatement findVisibleStatement(const QString &name, const PStatement &context) const;
    TypeInfo memberReceiver(TypeInfo type, bool arrow) const;
    TypeInfo dereference(TypeInfo type) const;
    TypeInfo removeReferences(TypeInfo type) const;
    int skipBalanced(const QVector<CppTokenizer::Token> &tokens, int index) const;

    const StatementModel &mModel;
    const CppTypeResolver &mResolver;
    const TypeOperationRegistry &mOperations;
};

#endif // CPPEXPRESSIONEVALUATOR_H
