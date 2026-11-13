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

EvalResult CppExpressionEvaluator::evaluate(const CppTokenizer &tokenizer,
                                            const PStatement &context) const
{
    int index = 0;
    EvalResult result = evaluateBinary(tokenizer.tokens(), index, context, 1);
    return result.isValid() && index == tokenizer.tokenCount() ? result : EvalResult();
}

EvalResult CppExpressionEvaluator::evaluateBinary(const TokenList &tokens, int &index,
                                                  const PStatement &context,
                                                  int minimumPrecedence) const
{
    EvalResult left = evaluateUnary(tokens, index, context);
    while (index < tokens.count() && left.isValid()) {
        QString operatorName = tokens[index].text;
        int precedence = binaryPrecedence(operatorName);
        if (precedence < minimumPrecedence)
            break;
        ++index;
        if (operatorName == "?") {
            EvalResult whenTrue = evaluateBinary(tokens, index, context, 1);
            if (!whenTrue.isValid() || index >= tokens.count() || tokens[index].text != ":")
                return {};
            ++index;
            EvalResult whenFalse = evaluateBinary(tokens, index, context, precedence);
            if (!whenFalse.isValid())
                return {};
            left = applyBinary(left, whenFalse, operatorName);
            if (isArithmeticType(whenTrue.type) && isArithmeticType(whenFalse.type))
                left.type = usualArithmeticType(whenTrue.type, whenFalse.type);
            continue;
        }
        EvalResult right = evaluateBinary(tokens, index, context,
                                          precedence + (isRightAssociative(operatorName) ? 0 : 1));
        if (!right.isValid())
            return {};
        left = applyBinary(left, right, operatorName);
    }
    return left;
}

EvalResult CppExpressionEvaluator::evaluateUnary(const TokenList &tokens, int &index,
                                                 const PStatement &context) const
{
    if (index >= tokens.count())
        return {};
    QString unaryOperator = tokens[index].text;
    if (unaryOperator == "const" || unaryOperator == "volatile") {
        ++index;
        EvalResult result = evaluateUnary(tokens, index, context);
        if (result.kind == EvalStatementKind::Type) {
            result.type.qualifiers.setFlag(unaryOperator == "const"
                                           ? TypeQualifier::Const : TypeQualifier::Volatile);
        }
        return result;
    }
    if (unaryOperator == "new" || unaryOperator == "delete" || unaryOperator == "sizeof") {
        ++index;
        EvalResult result = evaluateUnary(tokens, index, context);
        if (!result.isValid())
            return result;
        if (unaryOperator == "new") {
            result.type.derivations.prepend(TypeDerivation::pointer());
            result.kind = EvalStatementKind::Variable;
        } else if (unaryOperator == "delete") {
            result.type = primitiveType("void");
        } else {
            result.type = primitiveType("std::size_t");
        }
        result.kind = EvalStatementKind::Variable;
        return result;
    }
    if (unaryOperator == "(") {
        int close = tokens[index].matchIndex;
        if (close > index && close < tokens.count()) {
            int typeIndex = index + 1;
            EvalResult type = evaluateBinary(tokens, typeIndex, context, 1);
            if (type.isValid() && type.kind == EvalStatementKind::Type && typeIndex == close) {
                index = close + 1;
                EvalResult result = evaluateUnary(tokens, index, context);
                if (result.isValid()) {
                    result.type = type.type;
                    result.kind = EvalStatementKind::Variable;
                    return evaluatePostfixTail(result, tokens, index, context);
                }
            }
        }
    }
    static const QSet<QString> unaryOperators{"*", "&", "++", "--", "+", "-", "!", "~"};
    if (unaryOperators.contains(unaryOperator)) {
        ++index;
        EvalResult result = evaluateUnary(tokens, index, context);
        if (!result.isValid())
            return result;
        if (unaryOperator == "*")
            result.type = dereference(result.type);
        else if (unaryOperator == "&")
            result.type = addressOf(result.type);
        else if (unaryOperator == "++" || unaryOperator == "--")
            result.type = applyIncrement(result.type, "operator" + unaryOperator);
        else {
            TypeInfo overloaded = mResolver.invokeResult(result.type, "operator" + unaryOperator);
            if (!overloaded.qualifiedName.isEmpty())
                result.type = overloaded;
            else if (unaryOperator == "!")
                result.type = primitiveType("bool");
            else
                result.type = removeReferences(result.type);
        }
        return result;
    }
    return evaluatePostfix(tokens, index, context);
}

EvalResult CppExpressionEvaluator::evaluatePostfix(const TokenList &tokens, int &index,
                                                   const PStatement &context) const
{
    return evaluatePostfixTail(evaluatePrimary(tokens, index, context), tokens, index, context);
}

EvalResult CppExpressionEvaluator::evaluatePostfixTail(EvalResult result, const TokenList &tokens,
                                                       int &index, const PStatement &context) const
{
    while (index < tokens.count() && result.isValid()) {
        const QString operation = tokens[index].text;
        if (operation == "<") {
            int start = index;
            QVector<TemplateArgument> arguments;
            if (readNonTypeTemplateArguments(tokens, index, arguments)) {
                TypeInfo explicitResult = mOperations.explicitTemplateResult(result, arguments);
                if (!explicitResult.qualifiedName.isEmpty()) {
                    result.type = explicitResult;
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
            result = createResult(member);
            continue;
        }
        if (operation == "." || operation == "->") {
            bool arrow = operation == "->";
            ++index;
            if (index >= tokens.count())
                return {};
            QString memberName = tokens[index++].text;
            TypeInfo receiver = memberReceiver(result.type, arrow);
            bool call = index < tokens.count() && tokens[index].text == "(";
            if (call)
                index = skipBalanced(tokens, index);
            result.type = call ? mResolver.invokeResult(receiver, memberName)
                               : mResolver.memberType(receiver, memberName);
            result.kind = EvalStatementKind::Variable;
            continue;
        }
        if (operation == "[") {
            index = skipBalanced(tokens, index);
            if (result.kind == EvalStatementKind::Type) {
                result.type.derivations.append(TypeDerivation::array("[]"));
            } else {
                result.type = removeReferences(result.type);
                if (!result.type.derivations.isEmpty()
                        && (result.type.derivations.first().kind == TypeDerivationKind::Array
                            || result.type.derivations.first().kind == TypeDerivationKind::Pointer)) {
                    result.type.derivations.removeFirst();
                    result.type.derivations.prepend(TypeDerivation::lvalueReference());
                } else {
                    result.type = mOperations.subscriptReference(result.type);
                }
                result.kind = EvalStatementKind::Variable;
            }
            continue;
        }
        if (operation == "(") {
            if (result.kind == EvalStatementKind::Type) {
                index = skipBalanced(tokens, index);
            } else if (result.kind == EvalStatementKind::Function) {
                index = skipBalanced(tokens, index);
            } else {
                QVector<TypeInfo> arguments;
                if (!evaluateArguments(tokens, index, context, arguments))
                    return {};
                result.type = mOperations.invokeResult(result.type, arguments);
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
            result.type = applyIncrement(result.type, "operator" + operation);
            continue;
        }
        break;
    }
    return result;
}

EvalResult CppExpressionEvaluator::evaluatePrimary(const TokenList &tokens, int &index,
                                                   const PStatement &context) const
{
    if (index >= tokens.count())
        return {};
    const CppTokenizer::Token &initial = tokens[index++];
    if (initial.text == "[") {
        --index;
        return evaluateLambda(tokens, index, context);
    }
    if (initial.text == "(") {
        EvalResult result = evaluateBinary(tokens, index, context, 1);
        if (index >= tokens.count() || tokens[index].text != ")")
            return {};
        ++index;
        return result;
    }
    if (initial.text == "::") {
        if (index >= tokens.count())
            return {};
        return createResult(findDirectStatement({}, tokens[index++].text));
    }
    if (initial.text == "this") {
        if (!context)
            return {};
        EvalResult result = createResult(context);
        result.type.derivations.prepend(TypeDerivation::pointer());
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
            result = createResult(findVisibleStatement(initial.text, context));
        }
        break;
    default:
        break;
    }
    return result;
}

EvalResult CppExpressionEvaluator::evaluateLambda(const TokenList &tokens, int &index,
                                                  const PStatement &context) const
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
        fileName = context->fileName;
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

    auto lambda = std::make_shared<Statement>();
    lambda->parentScope = context;
    lambda->command = name;
    lambda->fullName = context && !context->fullName.isEmpty()
            ? context->fullName + "::" + name : name;
    lambda->kind = StatementKind::Lambda;
    lambda->scope = context && (context->kind == StatementKind::Function
                                || context->kind == StatementKind::Block)
            ? StatementScope::Local : StatementScope::Global;
    lambda->accessibility = StatementAccessibility::None;
    lambda->fileName = fileName;
    lambda->declarationBoundaryAncestor = context ? context->declarationBoundaryAncestor : QString();
    lambda->declarationRange.start = startToken.position;
    lambda->properties.setFlag(StatementProperty::DummyStatement);

    auto call = std::make_shared<Statement>();
    call->parentScope = lambda;
    call->command = "operator()";
    call->fullName = lambda->fullName + "::operator()";
    call->kind = StatementKind::OverloadedOperator;
    call->scope = StatementScope::ClassLocal;
    call->accessibility = StatementAccessibility::Public;
    call->fileName = fileName;
    call->declarationBoundaryAncestor = lambda->declarationBoundaryAncestor;
    call->declarationRange.start = startToken.position;
    call->typeInfo.qualifiedName = "auto";
    call->properties.setFlag(StatementProperty::OperatorOverloading);

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
            auto parameter = std::make_shared<Statement>();
            parameter->parentScope = call;
            parameter->command = tokens[nameIndex].text;
            parameter->fullName = call->fullName + "::" + parameter->command;
            parameter->kind = StatementKind::Parameter;
            parameter->scope = StatementScope::Local;
            parameter->accessibility = StatementAccessibility::None;
            parameter->fileName = fileName;
            parameter->declarationRange = SourceRange{tokens[nameIndex].position,
                                                      tokens[nameIndex].position};
            parameter->typeInfo = parseTypeText(typeText);
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

    lambda->declarationRange.end = tokens[bodyEnd].position;
    lambda->definitionRange = lambda->declarationRange;
    lambda->definitionFileName = fileName;
    lambda->definitionBoundaryAncestor = lambda->declarationBoundaryAncestor;
    lambda->setHasDefinition(true);
    call->declarationRange = lambda->declarationRange;
    call->definitionRange = lambda->declarationRange;
    call->definitionFileName = fileName;
    call->definitionBoundaryAncestor = lambda->declarationBoundaryAncestor;
    call->setHasDefinition(true);
    mModel.add(lambda);
    mModel.add(call);

    if (!trailingReturn.qualifiedName.isEmpty()) {
        call->typeInfo = trailingReturn;
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
                EvalResult returned = evaluateBinary(tokens, returnIndex, call, 1);
                if (returned.isValid())
                    call->typeInfo = returned.type;
                break;
            }
        }
        if (!foundReturn)
            call->typeInfo = primitiveType("void");
    }
    index = bodyEnd + 1;

    EvalResult result;
    result.type.qualifiedName = lambda->fullName;
    result.type.declaration = lambda;
    result.baseStatement = lambda;
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

EvalResult CppExpressionEvaluator::createResult(const PStatement &statement) const
{
    if (!statement)
        return {};
    EvalResult result;
    result.baseStatement = statement;
    switch (statement->kind) {
    case StatementKind::Namespace:
        result.type.qualifiedName = statement->fullName;
        result.type.declaration = statement;
        result.kind = EvalStatementKind::Namespace;
        break;
    case StatementKind::NamespaceAlias:
        result.type = statement->typeInfo;
        result.type.declaration = mResolver.findStatement(result.type.qualifiedName);
        result.kind = EvalStatementKind::Namespace;
        break;
    case StatementKind::Class:
    case StatementKind::EnumType:
    case StatementKind::EnumClassType:
    case StatementKind::Typedef:
    case StatementKind::Alias:
        result.type.qualifiedName = statement->fullName;
        result.type.declaration = statement;
        result.type = mResolver.resolve(result.type, {}, statement->parentScope.lock());
        result.kind = EvalStatementKind::Type;
        break;
    case StatementKind::Constructor: {
        PStatement parent = statement->parentScope.lock();
        if (!parent)
            return {};
        return createResult(parent);
    }
    case StatementKind::Function:
    case StatementKind::OverloadedOperator:
    case StatementKind::LiteralOperator:
        result.type = mResolver.resolve(statement->typeInfo, {}, statement->parentScope.lock());
        result.kind = EvalStatementKind::Function;
        break;
    case StatementKind::Variable:
    case StatementKind::GlobalVariable:
    case StatementKind::LocalVariable:
    case StatementKind::Parameter:
    case StatementKind::Enum:
        result.type = mResolver.resolve(statement->typeInfo, {}, statement->parentScope.lock());
        result.kind = EvalStatementKind::Variable;
        break;
    default:
        return {};
    }
    return result;
}

bool CppExpressionEvaluator::readNonTypeTemplateArguments(
        const TokenList &tokens, int &index, QVector<TemplateArgument> &arguments) const
{
    if (index >= tokens.count() || tokens[index].text != "<")
        return false;
    int currentIndex = index + 1;
    int nested = 0;
    QString expression;
    auto finishArgument = [&]() {
        if (expression.trimmed().isEmpty())
            return false;
        arguments.append(TemplateArgument::fromConstant(expression.trimmed()));
        expression.clear();
        return true;
    };
    while (currentIndex < tokens.count()) {
        const QString &text = tokens[currentIndex].text;
        if (nested == 0 && text == ">") {
            if (!finishArgument())
                return false;
            index = currentIndex + 1;
            return true;
        }
        if (nested == 0 && text == ",") {
            if (!finishArgument())
                return false;
            ++currentIndex;
            continue;
        }
        if (text == "(" || text == "[" || text == "{")
            ++nested;
        else if (text == ")" || text == "]" || text == "}")
            --nested;
        if (nested < 0)
            return false;
        expression += text;
        ++currentIndex;
    }
    return false;
}

bool CppExpressionEvaluator::evaluateArguments(const TokenList &tokens, int &index,
                                               const PStatement &context,
                                               QVector<TypeInfo> &arguments) const
{
    if (index >= tokens.count() || tokens[index].text != "(")
        return false;
    ++index;
    if (index < tokens.count() && tokens[index].text == ")") {
        ++index;
        return true;
    }
    while (index < tokens.count()) {
        EvalResult argument = evaluateBinary(tokens, index, context, 2);
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

EvalResult CppExpressionEvaluator::applyBinary(const EvalResult &left, const EvalResult &right,
                                               const QString &operatorName) const
{
    EvalResult result = left;
    TypeInfo overloaded = mResolver.invokeResult(left.type, "operator" + operatorName);
    if (!overloaded.qualifiedName.isEmpty()) {
        result.type = overloaded;
        result.kind = EvalStatementKind::Variable;
        return result;
    }
    if (operatorName == "|") {
        TypeInfo piped = mOperations.pipeResult(left.type, right.type);
        if (!piped.qualifiedName.isEmpty()) {
            result.type = piped;
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
        result.kind = EvalStatementKind::Variable;
        return result;
    }
    static const QSet<QString> booleanOperators{
        "==", "!=", "<", "<=", ">", ">=", "<=>", "&&", "||"
    };
    if (booleanOperators.contains(operatorName)) {
        result.type = primitiveType("bool");
        result.kind = EvalStatementKind::Literal;
        return result;
    }
    static const QSet<QString> arithmeticOperators{"+", "-", "*", "/", "%"};
    if (arithmeticOperators.contains(operatorName) && !hasPointer(left.type) && !hasPointer(right.type)) {
        result.type = usualArithmeticType(removeReferences(left.type), removeReferences(right.type));
    } else if ((operatorName == "+" || operatorName == "-")
               && !hasPointer(left.type) && hasPointer(right.type)) {
        result.type = right.type;
    } else {
        result.type = removeReferences(left.type);
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
        for (const QString &usingName : scope->usingList) {
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
    return scope ? findInMap(scope->children, name) : findInMap(mModel.childrenStatements(), name);
}

TypeInfo CppExpressionEvaluator::memberReceiver(TypeInfo type, bool arrow) const
{
    type = removeReferences(type);
    if (!arrow)
        return type;
    for (int depth = 0; depth < 8; ++depth) {
        if (!type.derivations.isEmpty()
                && type.derivations.first().kind == TypeDerivationKind::Pointer) {
            type.derivations.removeFirst();
            return type;
        }
        type = removeReferences(mResolver.invokeResult(type, "operator->"));
        if (type.qualifiedName.isEmpty())
            return {};
    }
    return {};
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

TypeInfo CppExpressionEvaluator::addressOf(TypeInfo type) const
{
    type = removeReferences(type);
    type.derivations.prepend(TypeDerivation::pointer());
    return type;
}

TypeInfo CppExpressionEvaluator::applyIncrement(TypeInfo type, const QString &operatorName) const
{
    TypeInfo overloaded = mResolver.invokeResult(type, operatorName);
    return overloaded.qualifiedName.isEmpty() ? type : overloaded;
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
