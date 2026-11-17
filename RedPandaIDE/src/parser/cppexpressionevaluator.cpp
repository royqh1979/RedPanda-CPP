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

#include "cppdeclarationparser.h"

namespace {
PStatement findInMap(const StatementMap &statements, const QString &name)
{
    const auto values = statements.values(name);
    for (const std::weak_ptr<AbstractStatement> &statement : values) {
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

TypeInfo primitiveType(const QString &name)
{
    TypeInfo result;
    result.qualifiedName = name;
    return result;
}

void appendExpressionToken(QString &target, const QString &text)
{
    if (!target.isEmpty() && !text.isEmpty()
            && (target.back().isLetterOrNumber() || target.back() == '_')
            && (text.front().isLetterOrNumber() || text.front() == '_'))
        target += ' ';
    target += text;
}

bool hasPointer(const TypeInfo &type)
{
    for (const TypeDerivation &derivation : type.derivations) {
        if (derivation.kind == TypeDerivationKind::Pointer)
            return true;
    }
    return false;
}

int arithmeticRank(const QString &name)
{
    if (name == "long double")
        return 120;
    if (name == "double")
        return 110;
    if (name == "float")
        return 100;
    if (name == "long long" || name == "unsigned long long")
        return 90;
    if (name == "long" || name == "unsigned long")
        return 80;
    if (name == "int" || name == "unsigned int" || name == "unsigned")
        return 70;
    return 10;
}

TypeInfo usualArithmeticType(const TypeInfo &left, const TypeInfo &right)
{
    const TypeInfo *selected = arithmeticRank(left.qualifiedName) >= arithmeticRank(right.qualifiedName)
            ? &left : &right;
    QString name = selected->qualifiedName;
    if (arithmeticRank(left.qualifiedName) == arithmeticRank(right.qualifiedName)) {
        if (left.qualifiedName.startsWith("unsigned"))
            name = left.qualifiedName;
        else if (right.qualifiedName.startsWith("unsigned"))
            name = right.qualifiedName;
    }
    return primitiveType(name == "unsigned" ? "unsigned int" : name);
}

bool isArithmeticType(const TypeInfo &type)
{
    static const QSet<QString> names{
        "bool", "char", "wchar_t", "char8_t", "char16_t", "char32_t", "short",
        "unsigned short", "int", "unsigned int", "unsigned", "long", "unsigned long",
        "long long", "unsigned long long", "float", "double", "long double"
    };
    return type.derivations.isEmpty() && names.contains(type.qualifiedName);
}

TypeInfo integerLiteralType(const QString &literal)
{
    QString suffix;
    for (int i = literal.count() - 1; i >= 0 && literal[i].isLetter(); --i)
        suffix.prepend(literal[i].toLower());
    if (suffix == "ull" || suffix == "llu")
        return primitiveType("unsigned long long");
    if (suffix == "ll")
        return primitiveType("long long");
    if (suffix == "ul" || suffix == "lu")
        return primitiveType("unsigned long");
    if (suffix == "l")
        return primitiveType("long");
    if (suffix == "u")
        return primitiveType("unsigned int");
    return primitiveType("int");
}

TypeInfo floatingLiteralType(const QString &literal)
{
    if (literal.endsWith('f') || literal.endsWith('F'))
        return primitiveType("float");
    if (literal.endsWith('l') || literal.endsWith('L'))
        return primitiveType("long double");
    return primitiveType("double");
}
}

CppExpressionEvaluator::CppExpressionEvaluator(StatementModel &model,
                                               const CppTypeResolver &resolver,
                                               const TypeOperationRegistry &operations)
    : mModel(model), mResolver(resolver), mOperations(operations)
{
}

EvalResult CppExpressionEvaluator::evaluate(
        const CppTokenizer &tokenizer, const PStatement &context,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    CppTypeResolver::EvaluationContext localEvaluation;
    if (!evaluation)
        evaluation = &localEvaluation;
    int index = 0;
    EvalResult result = evaluateBinary(tokenizer.tokens(), index, context, 1, evaluation);
    return result.isValid() && index == tokenizer.tokenCount() ? result : EvalResult();
}

EvalResult CppExpressionEvaluator::evaluateBinary(
        const TokenList &tokens, int &index, const PStatement &context,
        int minimumPrecedence, CppTypeResolver::EvaluationContext *evaluation) const
{
    EvalResult left = evaluateUnary(tokens, index, context, evaluation);
    while (index < tokens.count() && left.isValid()) {
        QString operatorName = tokens[index].text;
        int precedence = binaryPrecedence(operatorName);
        if (precedence < minimumPrecedence)
            break;
        ++index;
        if (operatorName == "?") {
            EvalResult whenTrue = evaluateBinary(tokens, index, context, 1, evaluation);
            if (!whenTrue.isValid() || index >= tokens.count() || tokens[index].text != ":")
                return {};
            ++index;
            EvalResult whenFalse = evaluateBinary(tokens, index, context, precedence, evaluation);
            if (!whenFalse.isValid())
                return {};
            left = applyBinary(left, whenFalse, operatorName, evaluation);
            if (isArithmeticType(whenTrue.type) && isArithmeticType(whenFalse.type)) {
                left.type = usualArithmeticType(whenTrue.type, whenFalse.type);
                left.instance = {};
            }
            continue;
        }
        EvalResult right = evaluateBinary(tokens, index, context,
                                          precedence + (isRightAssociative(operatorName) ? 0 : 1),
                                          evaluation);
        if (!right.isValid())
            return {};
        left = applyBinary(left, right, operatorName, evaluation);
    }
    return left;
}

EvalResult CppExpressionEvaluator::evaluateUnary(
        const TokenList &tokens, int &index, const PStatement &context,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    if (index >= tokens.count())
        return {};
    QString unaryOperator = tokens[index].text;
    if (unaryOperator == "const" || unaryOperator == "volatile") {
        ++index;
        EvalResult result = evaluateUnary(tokens, index, context, evaluation);
        if (result.kind == EvalStatementKind::Type) {
            result.type.qualifiers.setFlag(unaryOperator == "const"
                                           ? TypeQualifier::Const : TypeQualifier::Volatile);
            result.instance.type = result.type;
        }
        return result;
    }
    if (unaryOperator == "new" || unaryOperator == "delete" || unaryOperator == "sizeof") {
        ++index;
        EvalResult result = evaluateUnary(tokens, index, context, evaluation);
        if (!result.isValid())
            return result;
        if (unaryOperator == "new") {
            result.type.derivations.prepend(TypeDerivation::pointer());
            result.instance.type = result.type;
            result.kind = EvalStatementKind::Variable;
        } else if (unaryOperator == "delete") {
            result.type = primitiveType("void");
            result.instance = {};
        } else {
            result.type = primitiveType("std::size_t");
            result.instance = {};
        }
        result.kind = EvalStatementKind::Variable;
        return result;
    }
    if (unaryOperator == "(") {
        int close = tokens[index].matchIndex;
        if (close > index && close < tokens.count()) {
            int typeIndex = index + 1;
            EvalResult type = evaluateBinary(tokens, typeIndex, context, 1, evaluation);
            if (type.isValid() && type.kind == EvalStatementKind::Type && typeIndex == close) {
                index = close + 1;
                EvalResult result = evaluateUnary(tokens, index, context, evaluation);
                if (result.isValid()) {
                    result.type = type.type;
                    result.instance = mResolver.resolveInstance(result.type, {}, {}, evaluation);
                    result.kind = EvalStatementKind::Variable;
                    return evaluatePostfixTail(result, tokens, index, context, evaluation);
                }
            }
        }
    }
    static const QSet<QString> unaryOperators{"*", "&", "++", "--", "+", "-", "!", "~"};
    if (unaryOperators.contains(unaryOperator)) {
        ++index;
        EvalResult result = evaluateUnary(tokens, index, context, evaluation);
        if (!result.isValid())
            return result;
        if (unaryOperator == "*") {
            result = dereference(result, evaluation);
        } else if (unaryOperator == "&") {
            result.type = addressOf(result.type, evaluation);
            result.instance.type = result.type;
        } else if (unaryOperator == "++" || unaryOperator == "--") {
            result = applyIncrement(result, "operator" + unaryOperator, evaluation);
        } else {
            MemberEvalResult overloaded = mResolver.invokeResult(
                        memberReceiver(result, false, evaluation), "operator" + unaryOperator,
                        evaluation);
            if (!overloaded.type.qualifiedName.isEmpty()) {
                result.statement = overloaded.statement;
                result.type = overloaded.type;
                result.instance = overloaded.instance;
            } else if (unaryOperator == "!") {
                result.type = primitiveType("bool");
                result.instance = {};
            } else {
                result.type = removeReferences(result.type);
                result.instance.type = result.type;
            }
        }
        return result;
    }
    return evaluatePostfix(tokens, index, context, evaluation);
}

EvalResult CppExpressionEvaluator::evaluatePostfix(
        const TokenList &tokens, int &index, const PStatement &context,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    return evaluatePostfixTail(evaluatePrimary(tokens, index, context, evaluation),
                               tokens, index, context, evaluation);
}

EvalResult CppExpressionEvaluator::evaluatePostfixTail(
        EvalResult result, const TokenList &tokens, int &index, const PStatement &context,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    while (index < tokens.count() && result.isValid()) {
        const QString operation = tokens[index].text;
        if (operation == "<") {
            int start = index;
            QVector<TemplateArgument> arguments;
            if (readTemplateArguments(tokens, index, arguments)) {
                if (result.kind == EvalStatementKind::Function) {
                    result.explicitTemplateArguments = arguments;
                    continue;
                }
                TypeInfo explicitResult = mOperations.explicitTemplateResult(result, arguments,
                                                                             evaluation);
                if (!explicitResult.qualifiedName.isEmpty()) {
                    result.type = explicitResult;
                    result.instance = mResolver.resolveInstance(explicitResult, {}, {}, evaluation);
                    result.kind = EvalStatementKind::Variable;
                    continue;
                }
            }
            index = start;
        }
        if (result.kind == EvalStatementKind::Type
                && (operation == "const" || operation == "volatile")) {
            result.type.qualifiers.setFlag(operation == "const"
                                           ? TypeQualifier::Const : TypeQualifier::Volatile);
            result.instance.type = result.type;
            ++index;
            continue;
        }
        if (result.kind == EvalStatementKind::Type
                && (operation == "*" || operation == "&" || operation == "&&")) {
            if (operation == "*")
                result.type.derivations.append(TypeDerivation::pointer());
            else if (operation == "&")
                result.type.derivations.append(TypeDerivation::lvalueReference());
            else
                result.type.derivations.append(TypeDerivation::rvalueReference());
            result.instance.type = result.type;
            ++index;
            continue;
        }
        if (operation == "::") {
            if (result.kind != EvalStatementKind::Type && result.kind != EvalStatementKind::Namespace)
                return {};
            ++index;
            if (index >= tokens.count())
                return {};
            PStatement scope = result.type.declaration.lock();
            if (!scope)
                scope = mResolver.findStatement(result.type.qualifiedName);
            PStatement member = findDirectStatement(scope, tokens[index++].text);
            if (!member)
                return {};
            result = createResult(member, evaluation);
            continue;
        }
        if (operation == "." || operation == "->") {
            bool arrow = operation == "->";
            ++index;
            ClassInstance receiver = memberReceiver(result, arrow, evaluation);
            if (index >= tokens.count()) {
                if (receiver.type.qualifiedName.isEmpty())
                    return {};
                result.type = receiver.type;
                result.instance = receiver;
                result.statement.reset();
                result.kind = EvalStatementKind::Variable;
                continue;
            }
            QString memberName = tokens[index++].text;
            bool call = index < tokens.count() && tokens[index].text == "(";
            QVector<TypeInfo> arguments;
            if (call && !evaluateArguments(tokens, index, context, arguments, evaluation))
                return {};
            MemberEvalResult member = mResolver.resolveMember(receiver, memberName, call, arguments,
                                                              evaluation);
            result.statement = member.statement;
            result.type = member.type;
            result.instance = member.instance;
            result.kind = !call && isFunction(result.statement)
                    ? EvalStatementKind::Function : EvalStatementKind::Variable;
            if (result.kind == EvalStatementKind::Function)
                result.baseStatement = result.statement;
            continue;
        }
        if (operation == "[") {
            index = skipBalanced(tokens, index);
            if (result.kind == EvalStatementKind::Type) {
                result.type.derivations.append(TypeDerivation::array("[]"));
                result.instance.type = result.type;
            } else {
                result.type = removeReferences(result.type);
                result.instance.type = removeReferences(result.instance.type);
                if (!result.type.derivations.isEmpty()
                        && (result.type.derivations.first().kind == TypeDerivationKind::Array
                            || result.type.derivations.first().kind == TypeDerivationKind::Pointer)) {
                    result.type.derivations.removeFirst();
                    result.type.derivations.prepend(TypeDerivation::lvalueReference());
                    result.instance.type = result.type;
                } else {
                    MemberEvalResult element = mResolver.invokeResult(
                                memberReceiver(result, false, evaluation), "operator[]", evaluation);
                    result.statement = element.statement;
                    result.type = element.type;
                    result.instance = element.instance;
                }
                result.kind = EvalStatementKind::Variable;
            }
            continue;
        }
        if (operation == "(") {
            if (result.kind == EvalStatementKind::Type) {
                index = skipBalanced(tokens, index);
            } else if (result.kind == EvalStatementKind::Function) {
                QVector<TypeInfo> arguments;
                if (!evaluateArguments(tokens, index, context, arguments, evaluation))
                    return {};
                ClassInstance called;
                result.type = mResolver.invokeFunction(result.baseStatement, arguments,
                                                       result.explicitTemplateArguments, evaluation,
                                                       &called);
                result.instance = called;
            } else {
                QVector<TypeInfo> arguments;
                if (!evaluateArguments(tokens, index, context, arguments, evaluation))
                    return {};
                result.type = mOperations.invokeResult(result.type, arguments, evaluation);
                result.instance = mResolver.resolveInstance(result.type, {}, {}, evaluation);
            }
            result.kind = EvalStatementKind::Variable;
            continue;
        }
        if (operation == "{") {
            if (result.kind != EvalStatementKind::Type)
                return {};
            index = skipBalanced(tokens, index);
            result.kind = EvalStatementKind::Variable;
            continue;
        }
        if (operation == "++" || operation == "--") {
            ++index;
            result = applyIncrement(result, "operator" + operation, evaluation);
            continue;
        }
        break;
    }
    return result;
}

EvalResult CppExpressionEvaluator::evaluatePrimary(
        const TokenList &tokens, int &index, const PStatement &context,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    if (index >= tokens.count())
        return {};
    const CppTokenizer::Token &initial = tokens[index++];
    if (initial.text == "[") {
        --index;
        return evaluateLambda(tokens, index, context, evaluation);
    }
    if (initial.text == "(") {
        EvalResult result = evaluateBinary(tokens, index, context, 1, evaluation);
        if (index >= tokens.count() || tokens[index].text != ")")
            return {};
        ++index;
        return result;
    }
    if (initial.text == "::") {
        if (index >= tokens.count())
            return {};
        return createResult(findDirectStatement({}, tokens[index++].text), evaluation);
    }
    if (initial.text == "this") {
        if (!context)
            return {};
        EvalResult result = createResult(context, evaluation);
        result.type.derivations.prepend(TypeDerivation::pointer());
        result.instance.type = result.type;
        result.kind = EvalStatementKind::Variable;
        return result;
    }

    static const QSet<QString> builtinTypes{
        "auto", "void", "bool", "char", "wchar_t", "char8_t", "char16_t", "char32_t",
        "short", "int", "long", "signed", "unsigned", "float", "double"
    };
    if (builtinTypes.contains(initial.text)) {
        EvalResult result;
        result.type = primitiveType(initial.text);
        result.kind = EvalStatementKind::Type;
        return result;
    }

    EvalResult result;
    switch (initial.kind) {
    case CppTokenizer::TokenKind::IntegerLiteral:
        result.type = integerLiteralType(initial.text);
        result.kind = EvalStatementKind::Literal;
        break;
    case CppTokenizer::TokenKind::FloatingLiteral:
        result.type = floatingLiteralType(initial.text);
        result.kind = EvalStatementKind::Literal;
        break;
    case CppTokenizer::TokenKind::StringLiteral:
        result.type = primitiveType("char");
        result.type.derivations.append(TypeDerivation::pointer());
        result.kind = EvalStatementKind::Literal;
        break;
    case CppTokenizer::TokenKind::CharacterLiteral:
        result.type = primitiveType("char");
        result.kind = EvalStatementKind::Literal;
        break;
    case CppTokenizer::TokenKind::Identifier:
        if (initial.text == "true" || initial.text == "false") {
            result.type = primitiveType("bool");
            result.kind = EvalStatementKind::Literal;
        } else if (initial.text == "nullptr") {
            result.type = primitiveType("std::nullptr_t");
            result.kind = EvalStatementKind::Literal;
        } else {
            result = createResult(findVisibleStatement(initial.text, context), evaluation);
        }
        break;
    default:
        break;
    }
    return result;
}

EvalResult CppExpressionEvaluator::evaluateLambda(
        const TokenList &tokens, int &index, const PStatement &context,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    if (index >= tokens.count() || tokens[index].text != "[")
        return {};
    const CppTokenizer::Token &startToken = tokens[index];
    int captureEnd = startToken.matchIndex;
    if (captureEnd <= index || captureEnd >= tokens.count())
        return {};
    index = captureEnd + 1;

    QString fileName = startToken.fileName;
    if (fileName.isEmpty() && context)
        fileName = statementDeclarationForNavigation(context, QString()).fileName;
    QString name = QString("__lambda_%1_%2_%3")
            .arg(qHash(fileName), 0, 16).arg(startToken.position.line).arg(startToken.position.ch);
    if (PStatement existing = findDirectStatement(context, name)) {
        while (index < tokens.count() && tokens[index].text != "{")
            ++index;
        if (index < tokens.count()) {
            int bodyEnd = tokens[index].matchIndex;
            index = bodyEnd > index && bodyEnd < tokens.count() ? bodyEnd + 1 : tokens.count();
        }
        EvalResult cached;
        cached.type.qualifiedName = existing->fullName;
        cached.type.declaration = existing;
        cached.baseStatement = existing;
        cached.kind = EvalStatementKind::Variable;
        return cached;
    }
    if (evaluation && !evaluation->allowModelChanges)
        return {};

    auto lambda = StatementFactory::createLambda(
                context, name, context && !context->fullName.isEmpty()
                ? context->fullName + "::" + name : name);
    lambda->scope = context && (context->kind == StatementKind::Function
                                || context->kind == StatementKind::Block)
            ? StatementScope::Local : StatementScope::Global;
    QString boundaryAncestor = context
            ? statementDeclarationForNavigation(context, fileName).boundaryAncestor : QString();
    lambda->properties.setFlag(StatementProperty::DummyStatement);

    auto call = StatementFactory::createOverloadedOperator(
                lambda, "operator()", lambda->fullName + "::operator()");
    call->scope = StatementScope::ClassLocal;
    call->accessibility = StatementAccessibility::Public;
    std::dynamic_pointer_cast<Statement>(call)->typeInfo.qualifiedName = "auto";
    call->properties.setFlag(StatementProperty::OperatorOverloading);
    call->properties.setFlag(StatementProperty::Const);
    std::dynamic_pointer_cast<Statement>(call)->args = "() const";
    std::dynamic_pointer_cast<Statement>(call)->normalizedArgs = "() const";

    if (index < tokens.count() && tokens[index].text == "(") {
        int parametersEnd = tokens[index].matchIndex;
        if (parametersEnd <= index || parametersEnd >= tokens.count())
            return {};
        int parameterStart = index + 1;
        int segmentStart = parameterStart;
        int nested = 0;
        auto addParameter = [&](int first, int last) {
            if (first >= last)
                return;
            int nameIndex = -1;
            for (int i = last - 1; i >= first; --i) {
                if (tokens[i].kind == CppTokenizer::TokenKind::Identifier) {
                    nameIndex = i;
                    break;
                }
            }
            if (nameIndex < 0)
                return;
            QString typeText;
            for (int i = first; i < nameIndex; ++i)
                appendExpressionToken(typeText, tokens[i].text);
            auto parameter = StatementFactory::createParameter(
                        call, tokens[nameIndex].text, call->fullName + "::" + tokens[nameIndex].text);
            parameter->scope = StatementScope::Local;
            mModel.addDeclaration(parameter, fileName,
                                  SourceRange{tokens[nameIndex].position, tokens[nameIndex].position},
                                  boundaryAncestor, {});
            std::dynamic_pointer_cast<Statement>(parameter)->typeInfo = parseTypeText(typeText);
            mModel.add(parameter);
        };
        for (int i = parameterStart; i <= parametersEnd; ++i) {
            QString text = tokens[i].text;
            if (text == "(" || text == "[" || text == "{")
                ++nested;
            else if (text == ")" || text == "]" || text == "}")
                --nested;
            if (i == parametersEnd || (text == "," && nested == 0)) {
                addParameter(segmentStart, i);
                segmentStart = i + 1;
            }
        }
        index = parametersEnd + 1;
    }
    if (index < tokens.count() && tokens[index].text == "mutable") {
        call->properties.setFlag(StatementProperty::Const, false);
        std::dynamic_pointer_cast<Statement>(call)->args = "()";
        std::dynamic_pointer_cast<Statement>(call)->normalizedArgs = "()";
        ++index;
    }

    TypeInfo trailingReturn;
    if (index < tokens.count() && tokens[index].text == "->") {
        ++index;
        QString returnText;
        while (index < tokens.count() && tokens[index].text != "{") {
            appendExpressionToken(returnText, tokens[index].text);
            ++index;
        }
        trailingReturn = parseTypeText(returnText);
    }
    if (index >= tokens.count() || tokens[index].text != "{")
        return {};
    int bodyStart = index;
    int bodyEnd = tokens[index].matchIndex;
    if (bodyEnd <= bodyStart || bodyEnd >= tokens.count())
        bodyEnd = tokens.count() - 1;

    SourceRange lambdaRange{startToken.position, tokens[bodyEnd].position};
    lambda->definition = StatementOccurrence{fileName, lambdaRange, boundaryAncestor, {}};
    setStatementHasDefinition(lambda, true);
    call->definition = StatementOccurrence{fileName, lambdaRange, boundaryAncestor, {}};
    setStatementHasDefinition(call, true);
    mModel.add(lambda);
    mModel.add(call);

    if (!trailingReturn.qualifiedName.isEmpty()) {
        std::dynamic_pointer_cast<Statement>(call)->typeInfo = trailingReturn;
    } else {
        bool foundReturn = false;
        int nested = 0;
        for (int i = bodyStart + 1; i < bodyEnd; ++i) {
            const QString &text = tokens[i].text;
            if (text == "(" || text == "[" || text == "{")
                ++nested;
            else if (text == ")" || text == "]" || text == "}")
                --nested;
            if (text == "return" && nested == 0) {
                foundReturn = true;
                int returnIndex = i + 1;
                EvalResult returned = evaluateBinary(tokens, returnIndex, call, 1, evaluation);
                if (returned.isValid())
                    std::dynamic_pointer_cast<Statement>(call)->typeInfo = returned.type;
                break;
            }
        }
        if (!foundReturn)
            std::dynamic_pointer_cast<Statement>(call)->typeInfo = primitiveType("void");
    }
    index = bodyEnd + 1;

    EvalResult result;
    result.type.qualifiedName = lambda->fullName;
    result.type.declaration = lambda;
    result.baseStatement = lambda;
    result.statement = lambda;
    result.kind = EvalStatementKind::Variable;
    return result;
}

TypeInfo CppExpressionEvaluator::parseTypeText(const QString &text) const
{
    CppTokenizer tokenizer;
    tokenizer.tokenize({text});
    CppTokenCursor cursor(tokenizer);
    CppDeclarationParser parser(cursor);
    DeclSpecifierInfo specifiers;
    if (!parser.parseDeclSpecifierSeq(specifiers))
        return {};
    DeclaratorInfo declarator;
    if (!parser.parseDeclarator(declarator))
        return {};
    TypeInfo result = specifiers.type;
    result.derivations += declarator.derivations;
    return result;
}

EvalResult CppExpressionEvaluator::createResult(
        const PStatement &statement, CppTypeResolver::EvaluationContext *evaluation) const
{
    if (!statement)
        return {};
    EvalResult result;
    result.baseStatement = statement;
    result.statement = statement;
    switch (statement->kind) {
    case StatementKind::Namespace:
        result.type.qualifiedName = statement->fullName;
        result.type.declaration = statement;
        result.kind = EvalStatementKind::Namespace;
        break;
    case StatementKind::NamespaceAlias:
        result.type = std::dynamic_pointer_cast<Statement>(statement)->typeInfo;
        result.type.declaration = mResolver.findStatement(result.type.qualifiedName);
        result.kind = EvalStatementKind::Namespace;
        break;
    case StatementKind::Class:
    case StatementKind::EnumType:
    case StatementKind::EnumClassType:
        result.type.qualifiedName = statement->fullName;
        result.type.declaration = statement;
        result.kind = EvalStatementKind::Type;
        break;
    case StatementKind::Typedef:
    case StatementKind::Alias:
        result.type.qualifiedName = statement->fullName;
        result.type.declaration = statement;
        result.instance = mResolver.resolveInstance(result.type, {}, statement->parentScope.lock(),
                                                    evaluation);
        result.type = result.instance.type;
        result.kind = EvalStatementKind::Type;
        break;
    case StatementKind::Constructor: {
        PStatement parent = statement->parentScope.lock();
        if (!parent)
            return {};
        EvalResult parentResult = createResult(parent, evaluation);
        parentResult.baseStatement = statement;
        parentResult.statement = statement;
        return parentResult;
    }
    case StatementKind::Function:
    case StatementKind::OverloadedOperator:
    case StatementKind::LiteralOperator:
        result.type = std::dynamic_pointer_cast<Statement>(statement)->typeInfo;
        result.kind = EvalStatementKind::Function;
        break;
    case StatementKind::Variable:
    case StatementKind::GlobalVariable:
    case StatementKind::LocalVariable:
    case StatementKind::Parameter:
    case StatementKind::Enum:
        result.instance = mResolver.resolveInstance(std::dynamic_pointer_cast<Statement>(statement)->typeInfo, {},
                                                    statement->parentScope.lock(), evaluation);
        result.type = result.instance.type;
        result.kind = EvalStatementKind::Variable;
        break;
    case StatementKind::Label:
        result.kind = EvalStatementKind::Label;
        break;
    case StatementKind::Preprocessor:
        result.kind = EvalStatementKind::Preprocessor;
        break;
    default:
        return {};
    }
    return result;
}

bool CppExpressionEvaluator::readTemplateArguments(
        const TokenList &tokens, int &index, QVector<TemplateArgument> &arguments) const
{
    if (index >= tokens.count() || tokens[index].text != "<")
        return false;
    int currentIndex = index + 1;
    int level = 1;
    QString expression;
    auto finishArgument = [&]() {
        QString text = expression.trimmed();
        if (text.isEmpty())
            return false;
        TypeInfo type = parseTypeText(text);
        if (!type.qualifiedName.isEmpty()) {
            TemplateArgument argument = TemplateArgument::fromType(type);
            argument.constantExpression = text;
            arguments.append(argument);
        } else {
            arguments.append(TemplateArgument::fromConstant(text));
        }
        expression.clear();
        return true;
    };
    while (currentIndex < tokens.count()) {
        const QString &text = tokens[currentIndex].text;
        if (tokens[currentIndex].matchIndex > currentIndex && text != "<") {
            int close = tokens[currentIndex].matchIndex;
            for (; currentIndex <= close && currentIndex < tokens.count(); ++currentIndex)
                appendExpressionToken(expression, tokens[currentIndex].text);
            continue;
        }
        if (text == "<") {
            ++level;
            appendExpressionToken(expression, text);
            ++currentIndex;
            continue;
        }
        if (text == ">" || text == ">>") {
            int closingCount = text.length();
            ++currentIndex;
            while (closingCount-- > 0) {
                --level;
                if (level == 0) {
                    if (!finishArgument())
                        return false;
                    index = currentIndex;
                    return true;
                }
                appendExpressionToken(expression, ">");
            }
            continue;
        }
        if (text == "," && level == 1) {
            if (!finishArgument())
                return false;
            ++currentIndex;
            continue;
        }
        appendExpressionToken(expression, text);
        ++currentIndex;
    }
    return false;
}

bool CppExpressionEvaluator::evaluateArguments(
        const TokenList &tokens, int &index, const PStatement &context,
        QVector<TypeInfo> &arguments, CppTypeResolver::EvaluationContext *evaluation) const
{
    if (index >= tokens.count() || tokens[index].text != "(")
        return false;
    ++index;
    if (index < tokens.count() && tokens[index].text == ")") {
        ++index;
        return true;
    }
    while (index < tokens.count()) {
        EvalResult argument = evaluateBinary(tokens, index, context, 2, evaluation);
        if (!argument.isValid())
            return false;
        arguments.append(argument.type);
        if (index < tokens.count() && tokens[index].text == ",") {
            ++index;
            continue;
        }
        if (index < tokens.count() && tokens[index].text == ")") {
            ++index;
            return true;
        }
        return false;
    }
    return false;
}

EvalResult CppExpressionEvaluator::applyBinary(
        const EvalResult &left, const EvalResult &right, const QString &operatorName,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    EvalResult result = left;
    MemberEvalResult overloaded = mResolver.invokeResult(
                memberReceiver(left, false, evaluation), "operator" + operatorName, evaluation);
    if (!overloaded.type.qualifiedName.isEmpty()) {
        result.statement = overloaded.statement;
        result.type = overloaded.type;
        result.instance = overloaded.instance;
        result.kind = EvalStatementKind::Variable;
        return result;
    }
    if (operatorName == "|") {
        TypeInfo piped = mOperations.pipeResult(left.type, right.type, evaluation);
        if (!piped.qualifiedName.isEmpty()) {
            result.type = piped;
            result.instance = mResolver.resolveInstance(piped, {}, {}, evaluation);
            result.kind = EvalStatementKind::Variable;
            return result;
        }
    }
    if (operatorName == "," || operatorName == "?") {
        result = right;
        result.baseStatement = left.baseStatement;
        return result;
    }
    if (operatorName == ".*" || operatorName == "->*") {
        result.type = removeReferences(right.type);
        if (!result.type.derivations.isEmpty()
                && result.type.derivations.first().kind == TypeDerivationKind::MemberPointer)
            result.type.derivations.removeFirst();
        else
            return {};
        result.instance = mResolver.resolveInstance(result.type, {}, {}, evaluation);
        result.kind = EvalStatementKind::Variable;
        return result;
    }
    static const QSet<QString> booleanOperators{
        "==", "!=", "<", "<=", ">", ">=", "<=>", "&&", "||"
    };
    if (booleanOperators.contains(operatorName)) {
        result.type = primitiveType("bool");
        result.instance = {};
        result.kind = EvalStatementKind::Literal;
        return result;
    }
    static const QSet<QString> arithmeticOperators{"+", "-", "*", "/", "%"};
    if (arithmeticOperators.contains(operatorName) && !hasPointer(left.type) && !hasPointer(right.type)) {
        result.type = usualArithmeticType(removeReferences(left.type), removeReferences(right.type));
        result.instance = {};
    } else if ((operatorName == "+" || operatorName == "-")
               && !hasPointer(left.type) && hasPointer(right.type)) {
        result.type = right.type;
        result.instance = right.instance;
    } else {
        result.type = removeReferences(left.type);
        result.instance.type = result.type;
    }
    result.kind = EvalStatementKind::Variable;
    return result;
}

PStatement CppExpressionEvaluator::findVisibleStatement(const QString &name,
                                                        const PStatement &context) const
{
    PStatement scope = context;
    while (scope) {
        if (PStatement result = findDirectStatement(scope, name))
            return result;
        const QSet<QString> *usingList = scope->kind == StatementKind::Namespace
                ? &std::dynamic_pointer_cast<NamespaceStatement>(scope)->usingList
                : &std::dynamic_pointer_cast<Statement>(scope)->usingList;
        for (const QString &usingName : *usingList) {
            PStatement target = mResolver.findStatement(usingName, scope);
            if (target && (target->kind == StatementKind::Namespace
                           || target->kind == StatementKind::EnumType
                           || target->kind == StatementKind::EnumClassType)) {
                if (PStatement result = findDirectStatement(target, name))
                    return result;
            }
        }
        scope = scope->parentScope.lock();
    }
    return findDirectStatement({}, name);
}

PStatement CppExpressionEvaluator::findDirectStatement(const PStatement &scope,
                                                       const QString &name) const
{
    return mResolver.findStatement(name, scope);
}

ClassInstance CppExpressionEvaluator::memberReceiver(
        const EvalResult &result, bool arrow, CppTypeResolver::EvaluationContext *evaluation) const
{
    ClassInstance current = result.instance;
    current.type = removeReferences(result.type);
    if (!arrow) {
        if (!current.declaration)
            return mResolver.resolveInstance(current.type, {}, {}, evaluation);
        return current;
    }
    for (int depth = 0; depth < 8; ++depth) {
        if (!current.type.derivations.isEmpty()
                && current.type.derivations.first().kind == TypeDerivationKind::Pointer) {
            current.type.derivations.removeFirst();
            return mResolver.resolveInstance(current.type, {}, {}, evaluation);
        }
        MemberEvalResult member = mResolver.resolveMember(current, "operator->", true, {}, evaluation);
        if (member.type.qualifiedName.isEmpty())
            return {};
        current = member.instance;
        current.type = removeReferences(member.type);
    }
    return {};
}

EvalResult CppExpressionEvaluator::dereference(
        EvalResult result, CppTypeResolver::EvaluationContext *evaluation) const
{
    result.type = removeReferences(result.type);
    result.instance.type = removeReferences(result.instance.type);
    if (!result.type.derivations.isEmpty()
            && result.type.derivations.first().kind == TypeDerivationKind::Pointer) {
        result.type.derivations.removeFirst();
        result.type.derivations.prepend(TypeDerivation::lvalueReference());
        result.instance.type = result.type;
        return result;
    }
    MemberEvalResult overloaded = mResolver.invokeResult(
                memberReceiver(result, false, evaluation), "operator*", evaluation);
    result.statement = overloaded.statement;
    result.type = overloaded.type;
    result.instance = overloaded.instance;
    return result;
}

TypeInfo CppExpressionEvaluator::addressOf(
        TypeInfo type, CppTypeResolver::EvaluationContext *evaluation) const
{
    Q_UNUSED(evaluation);
    type = removeReferences(type);
    type.derivations.prepend(TypeDerivation::pointer());
    return type;
}

EvalResult CppExpressionEvaluator::applyIncrement(
        EvalResult result, const QString &operatorName,
        CppTypeResolver::EvaluationContext *evaluation) const
{
    MemberEvalResult overloaded = mResolver.invokeResult(
                memberReceiver(result, false, evaluation), operatorName, evaluation);
    if (!overloaded.type.qualifiedName.isEmpty()) {
        result.statement = overloaded.statement;
        result.type = overloaded.type;
        result.instance = overloaded.instance;
    }
    return result;
}

TypeInfo CppExpressionEvaluator::removeReferences(TypeInfo type) const
{
    while (!type.derivations.isEmpty()
            && (type.derivations.first().kind == TypeDerivationKind::LvalueReference
                || type.derivations.first().kind == TypeDerivationKind::RvalueReference))
        type.derivations.removeFirst();
    return type;
}

int CppExpressionEvaluator::binaryPrecedence(const QString &operatorName) const
{
    if (operatorName == ",")
        return 1;
    static const QSet<QString> assignments{
        "=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|="
    };
    if (assignments.contains(operatorName))
        return 2;
    if (operatorName == "?")
        return 3;
    if (operatorName == "||")
        return 4;
    if (operatorName == "&&")
        return 5;
    if (operatorName == "|")
        return 6;
    if (operatorName == "^")
        return 7;
    if (operatorName == "&")
        return 8;
    if (operatorName == "==" || operatorName == "!=")
        return 9;
    if (operatorName == "<" || operatorName == "<=" || operatorName == ">"
            || operatorName == ">=" || operatorName == "<=>")
        return 10;
    if (operatorName == "<<" || operatorName == ">>")
        return 11;
    if (operatorName == "+" || operatorName == "-")
        return 12;
    if (operatorName == "*" || operatorName == "/" || operatorName == "%")
        return 13;
    if (operatorName == ".*" || operatorName == "->*")
        return 14;
    return 0;
}

bool CppExpressionEvaluator::isRightAssociative(const QString &operatorName) const
{
    return binaryPrecedence(operatorName) == 2;
}

int CppExpressionEvaluator::skipBalanced(const TokenList &tokens, int index) const
{
    if (index < 0 || index >= tokens.count())
        return tokens.count();
    int target = tokens[index].matchIndex;
    return target > index && target < tokens.count() ? target + 1 : index + 1;
}
