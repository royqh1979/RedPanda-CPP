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
#include "cppstatementbuilder.h"

#include <limits>
#include <QDebug>
#include <QElapsedTimer>

namespace {
struct MergeDeclarationProfile {
    qint64 elapsedNs = 0;
    ~MergeDeclarationProfile() { qWarning() << "MERGE_DECLARATION_PROFILE ms" << elapsedNs / 1000000.0; }
};

MergeDeclarationProfile mergeDeclarationProfile;

struct MergeDeclarationProfileScope {
    explicit MergeDeclarationProfileScope() { timer.start(); }
    ~MergeDeclarationProfileScope() { mergeDeclarationProfile.elapsedNs += timer.nsecsElapsed(); }
    QElapsedTimer timer;
};

struct StatementBuilderProfile {
    qint64 createStatementNs = 0;
    qint64 autoTypeNs = 0;
    ~StatementBuilderProfile()
    {
        qWarning() << "STATEMENT_BUILDER_PROFILE createStatementMs" << createStatementNs / 1000000.0
                   << "autoTypeMs" << autoTypeNs / 1000000.0;
    }
};

StatementBuilderProfile statementBuilderProfile;

struct StatementBuilderProfileScope {
    explicit StatementBuilderProfileScope(qint64 &target) : target(target) { timer.start(); }
    ~StatementBuilderProfileScope() { target += timer.nsecsElapsed(); }
    qint64 &target;
    QElapsedTimer timer;
};

bool isFunction(const DeclaratorInfo &declarator)
{
    return !declarator.derivations.isEmpty()
            && declarator.derivations.first().kind == TypeDerivationKind::Function;
}

bool isDeductionGuide(const DeclarationInfo &declaration, const DeclaratorInfo &declarator)
{
    return isFunction(declarator)
            && declaration.specifiers.type.qualifiedName.isEmpty()
            && !declarator.derivations.first().trailingReturnType.isEmpty();
}


QString renderParameters(const TypeDerivation &function, bool includeNames)
{
    QStringList parameters;
    for (const ParameterInfo &parameter : function.parameters)
        parameters.append(parameter.toString(includeNames));
    if (function.functionProperties.testFlag(FunctionProperty::Variadic))
        parameters.append("...");
    return "(" + parameters.join(",") + ")";
}

QString withoutTemplateArguments(QString name)
{
    int angle = name.indexOf('<');
    if (angle >= 0)
        name.truncate(angle);
    return name;
}

QString className(const DeclaratorInfo &declarator, const StatementBuildContext &context)
{
    PStatement parent = context.parentScope;
    if (parent && parent->kind == StatementKind::Class)
        return withoutTemplateArguments(parent->command);
    QString qualifier = withoutTemplateArguments(declarator.qualifier);
    int separator = qualifier.lastIndexOf("::");
    if (separator >= 0)
        qualifier = qualifier.mid(separator + 2);
    return qualifier;
}

class IntegerConstantExpression
{
public:
    IntegerConstantExpression(const QVector<CppTokenizer::Token> &tokens,
                              const QHash<QString, qlonglong> &values)
        : mTokens(tokens), mValues(values) { }

    qlonglong evaluate(bool &ok)
    {
        mIndex = 0;
        ok = true;
        qlonglong result = bitOr(ok);
        return ok && mIndex == mTokens.count() ? result : 0;
    }

private:
    qlonglong primary(bool &ok)
    {
        if (mIndex >= mTokens.count()) {
            ok = false;
            return 0;
        }
        const CppTokenizer::Token &token = mTokens[mIndex++];
        if (token.text == "(") {
            qlonglong result = bitOr(ok);
            if (mIndex >= mTokens.count() || mTokens[mIndex++].text != ")")
                ok = false;
            return result;
        }
        if (token.kind == CppTokenizer::TokenKind::IntegerLiteral) {
            bool converted = false;
            qlonglong result = token.text.toLongLong(&converted, 0);
            ok = converted;
            return result;
        }
        if (token.kind == CppTokenizer::TokenKind::CharacterLiteral) {
            QString text = token.text;
            int quote = text.indexOf('\'');
            if (quote >= 0 && quote + 1 < text.count()) {
                QChar ch = text[quote + 1];
                if (ch == '\\' && quote + 2 < text.count())
                    ch = text[quote + 2];
                return ch.unicode();
            }
        }
        if (token.kind == CppTokenizer::TokenKind::Identifier && mValues.contains(token.text))
            return mValues.value(token.text);
        ok = false;
        return 0;
    }

    qlonglong unary(bool &ok)
    {
        if (mIndex < mTokens.count() && (mTokens[mIndex].text == "+"
                                         || mTokens[mIndex].text == "-"
                                         || mTokens[mIndex].text == "~")) {
            QString operation = mTokens[mIndex++].text;
            qlonglong result = unary(ok);
            if (operation == "-")
                result = -result;
            else if (operation == "~")
                result = ~result;
            return result;
        }
        return primary(ok);
    }

    qlonglong multiply(bool &ok)
    {
        qlonglong result = unary(ok);
        while (ok && mIndex < mTokens.count()) {
            QString operation = mTokens[mIndex].text;
            if (operation != "*" && operation != "/" && operation != "%")
                break;
            ++mIndex;
            qlonglong right = unary(ok);
            if (!ok)
                return 0;
            if (operation != "*" && (right == 0
                    || (result == std::numeric_limits<qlonglong>::min() && right == -1))) {
                ok = false;
                return 0;
            }
            if (operation == "*")
                result *= right;
            else if (operation == "/")
                result /= right;
            else
                result %= right;
        }
        return result;
    }

    qlonglong add(bool &ok)
    {
        qlonglong result = multiply(ok);
        while (ok && mIndex < mTokens.count()) {
            QString operation = mTokens[mIndex].text;
            if (operation != "+" && operation != "-")
                break;
            ++mIndex;
            qlonglong right = multiply(ok);
            result = operation == "+" ? result + right : result - right;
        }
        return result;
    }

    qlonglong shift(bool &ok)
    {
        qlonglong result = add(ok);
        while (ok && mIndex < mTokens.count()) {
            QString operation = mTokens[mIndex].text;
            if (operation != "<<" && operation != ">>")
                break;
            ++mIndex;
            qlonglong right = add(ok);
            result = operation == "<<" ? result << right : result >> right;
        }
        return result;
    }

    qlonglong bitAnd(bool &ok)
    {
        qlonglong result = shift(ok);
        while (ok && mIndex < mTokens.count() && mTokens[mIndex].text == "&") {
            ++mIndex;
            result &= shift(ok);
        }
        return result;
    }

    qlonglong bitXor(bool &ok)
    {
        qlonglong result = bitAnd(ok);
        while (ok && mIndex < mTokens.count() && mTokens[mIndex].text == "^") {
            ++mIndex;
            result ^= bitAnd(ok);
        }
        return result;
    }

    qlonglong bitOr(bool &ok)
    {
        qlonglong result = bitXor(ok);
        while (ok && mIndex < mTokens.count() && mTokens[mIndex].text == "|") {
            ++mIndex;
            result |= bitXor(ok);
        }
        return result;
    }

    const QVector<CppTokenizer::Token> &mTokens;
    const QHash<QString, qlonglong> &mValues;
    int mIndex = 0;
};

qlonglong evaluateIntegerConstant(const QString &text, const QHash<QString, qlonglong> &values,
                                  bool &ok)
{
    CppTokenizer tokenizer;
    tokenizer.tokenize({text});
    IntegerConstantExpression parser(tokenizer.tokens(), values);
    return parser.evaluate(ok);
}
}

CppStatementBuilder::CppStatementBuilder(StatementModel &model)
    : mModel(model), mTypeResolver(model), mTypeOperations(mTypeResolver),
      mExpressionEvaluator(model, mTypeResolver, mTypeOperations)
{
}

QList<PStatement> CppStatementBuilder::doFindDeclarationRemainder(
        const PStatement &statement, const QStringList &parts) const
{
    QList<PStatement> result;
    if (!statement || parts.isEmpty())
        return result;
    if (parts.count() == 1) {
        const auto candidates = mModel.childrenStatements(statement->parentScope).values(
                    statement->command);
        for (const std::weak_ptr<Statement> &weak : candidates) {
            if (PStatement candidate = weak.lock())
                result.append(candidate);
        }
        return result;
    }
    if (statement->kind != StatementKind::Namespace
            && statement->kind != StatementKind::Class
            && statement->kind != StatementKind::EnumType
            && statement->kind != StatementKind::EnumClassType) {
        return result;
    }

    QList<PStatement> current{statement};
    for (int i = 1; !current.isEmpty() && i < parts.count(); ++i) {
        QList<PStatement> next;
        for (const PStatement &scope : current) {
            const auto candidates = mModel.childrenStatements(scope).values(parts[i]);
            for (const std::weak_ptr<Statement> &weak : candidates) {
                if (PStatement candidate = weak.lock())
                    next.append(candidate);
            }
        }
        current = next;
    }
    return current;
}

QList<PStatement> CppStatementBuilder::doFindStatements(const QString &qualifiedName) const
{
    QString name = qualifiedName;
    if (name.startsWith("::"))
        name.remove(0, 2);
    const QStringList parts = name.split("::", Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return {};

    for (int namespacePartCount = parts.count(); namespacePartCount > 0; --namespacePartCount) {
        QString namespaceName = parts.mid(0, namespacePartCount).join("::");
        QList<PStatement> namespaces = mModel.namespaceStatements(namespaceName);
        if (namespaces.isEmpty())
            continue;
        if (namespacePartCount == parts.count())
            return namespaces;
        QList<PStatement> result;
        for (const PStatement &nameSpace : namespaces) {
            result += doFindDeclarationRemainder(
                        nameSpace, parts.mid(namespacePartCount - 1));
        }
        return result;
    }

    QList<PStatement> result;
    const auto candidates = mModel.childrenStatements().values(parts.first());
    for (const std::weak_ptr<Statement> &weak : candidates) {
        if (PStatement candidate = weak.lock())
            result += doFindDeclarationRemainder(candidate, parts);
    }
    return result;
}

PStatement CppStatementBuilder::doFindDeclarationToMerge(
        const PStatement &candidate, const StatementBuildContext &context) const
{
    QList<PStatement> candidates;
    if (context.scope == StatementScope::Local) {
        const auto directCandidates = mModel.childrenStatements(context.parentScope).values(
                    candidate->command);
        for (const std::weak_ptr<Statement> &weak : directCandidates) {
            if (PStatement existing = weak.lock())
                candidates.append(existing);
        }
    } else {
        candidates = doFindStatements(candidate->fullName);
    }

    for (const PStatement &existing : candidates) {
        if (existing->fullName != candidate->fullName || existing->kind != candidate->kind
                || existing->noNameArgs != candidate->noNameArgs) {
            continue;
        }
        bool sameAncestor = existing->declarationBoundaryAncestor == context.boundaryAncestor
                || existing->definitionBoundaryAncestor == context.boundaryAncestor;
        bool declarationPair = existing->hasDefinition() != candidate->hasDefinition();
        if (sameAncestor || declarationPair)
            return existing;
    }
    return nullptr;
}

PStatement CppStatementBuilder::mergeDeclaration(const PStatement &candidate,
                                                 const StatementBuildContext &context) const
{
    MergeDeclarationProfileScope profileScope;
    PStatement existing = doFindDeclarationToMerge(candidate, context);
    if (!existing)
        return nullptr;

    if (candidate->hasDefinition()) {
        existing->definitionRange = candidate->declarationRange;
        existing->definitionFileName = context.fileName;
        existing->definitionBoundaryAncestor = context.boundaryAncestor;
        existing->setHasDefinition(true);
    } else {
        existing->declarationRange = candidate->declarationRange;
        existing->fileName = context.fileName;
        existing->declarationBoundaryAncestor = context.boundaryAncestor;
    }
    existing->declSpecifierProperties |= candidate->declSpecifierProperties;
    mModel.updateNamespaceTransparency(existing);
    return existing;
}

QList<std::weak_ptr<Statement>> CppStatementBuilder::build(
        const DeclarationInfo &declaration, const StatementBuildContext &context)
{
    QList<std::weak_ptr<Statement>> result;
    if (declaration.kind == DeclarationKind::Label) {
        for (const DeclaratorInfo &declarator : declaration.declarators) {
            PStatement statement = createStatement(declaration, declarator, context,
                                                   StatementKind::Label);
            mModel.add(statement);
            if (context.fileInfo)
                context.fileInfo->addStatement(statement);
            result.append(statement);
        }
        return result;
    }
    if (declaration.kind == DeclarationKind::Using)
        return buildUsing(declaration, context);
    if (declaration.kind == DeclarationKind::Namespace)
        return buildNamespace(declaration, context);
    if (declaration.kind == DeclarationKind::Block) {
        PStatement block = createBlock(declaration, context);
        mModel.add(block);
        if (context.fileInfo) {
            context.fileInfo->addStatement(block);
            context.fileInfo->addScope(declaration.range, block, context.parentScope);
        }
        result.append(block);
        StatementBuildContext childContext = context;
        childContext.parentScope = block;
        childContext.scope = StatementScope::Local;
        childContext.accessibility = StatementAccessibility::None;
        if (!declaration.rangeExpression.isEmpty()) {
            for (const DeclaratorInfo &declarator : declaration.declarators) {
                PStatement variable = createStatement(declaration, declarator, childContext,
                                                      StatementKind::LocalVariable);
                mModel.add(variable);
                if (context.fileInfo)
                    context.fileInfo->addStatement(variable);
            }
        }
        for (const DeclarationInfo &child : declaration.children)
            build(child, childContext);
        return result;
    }

    int firstVariable = 0;
    if (declaration.kind == DeclarationKind::Class
            || declaration.kind == DeclarationKind::Enum
            || declaration.kind == DeclarationKind::Namespace
            || declaration.kind == DeclarationKind::NamespaceAlias
            || declaration.kind == DeclarationKind::Alias)
        firstVariable = declaration.declarators.isEmpty() ? 0 : 1;

    for (int i = 0; i < declaration.declarators.count(); ++i) {
        const DeclaratorInfo &declarator = declaration.declarators[i];
        if (isDeductionGuide(declaration, declarator))
            continue;
        StatementKind kind;
        if (i < firstVariable) {
            switch (declaration.kind) {
            case DeclarationKind::Class:
                kind = StatementKind::Class;
                break;
            case DeclarationKind::Enum:
                kind = declaration.enumScoped ? StatementKind::EnumClassType
                                              : StatementKind::EnumType;
                break;
            case DeclarationKind::Namespace:
                kind = StatementKind::Namespace;
                break;
            case DeclarationKind::NamespaceAlias:
                kind = StatementKind::NamespaceAlias;
                break;
            case DeclarationKind::Alias:
                kind = StatementKind::Alias;
                break;
            default:
                continue;
            }
        } else {
            kind = declaratorKind(declaration, declarator, context);
        }

        StatementBuildContext declarationContext = context;
        if (isFunction(declarator) && context.scope == StatementScope::Local
                && declaration.specifiers.properties.testFlag(DeclSpecifierProperty::Extern)) {
            declarationContext.scope = StatementScope::Global;
            declarationContext.accessibility = StatementAccessibility::None;
            PStatement parent = context.parentScope;
            while (parent && parent->kind != StatementKind::Namespace)
                parent = parent->parentScope.lock();
            declarationContext.parentScope = parent;
        }
        PStatement statement = createStatement(declaration, declarator, declarationContext, kind);
        if (i >= firstVariable && firstVariable > 0
                && (declaration.kind == DeclarationKind::Class
                    || declaration.kind == DeclarationKind::Enum)) {
            statement->typeInfo.qualifiedName = declaration.declarators.first().name;
        }
        PStatement merged = mergeDeclaration(statement, context);
        if (merged)
            statement = merged;
        else
            mModel.add(statement);
        if (context.fileInfo) {
            context.fileInfo->addStatement(statement);
            if (declaration.bodyRange.isValid()
                    && (kind == StatementKind::Class || kind == StatementKind::Namespace
                        || isFunction(declarator))) {
                context.fileInfo->addScope(declaration.bodyRange, statement, context.parentScope);
            }
        }
        result.append(statement);

        if (i == 0 && (kind == StatementKind::Class || kind == StatementKind::Namespace))
            buildChildren(declaration, context, statement);
        if (i == 0 && (kind == StatementKind::EnumType || kind == StatementKind::EnumClassType)) {
            PStatement enumeratorParent = declaration.enumScoped ? statement : context.parentScope;
            StatementBuildContext enumeratorContext = context;
            enumeratorContext.parentScope = enumeratorParent;
            QHash<QString, qlonglong> enumeratorValues;
            qlonglong nextEnumeratorValue = 0;
            for (const EnumeratorInfo &enumerator : declaration.enumerators) {
                auto enumStatement = std::make_shared<Statement>();
                enumStatement->parentScope = enumeratorParent;
                enumStatement->command = enumerator.name;
                enumStatement->fullName = enumeratorParent && !enumeratorParent->fullName.isEmpty()
                        ? enumeratorParent->fullName + "::" + enumerator.name : enumerator.name;
                enumStatement->kind = StatementKind::Enum;
                enumStatement->scope = context.scope;
                enumStatement->accessibility = accessibility(declaration, context);
                enumStatement->declarationRange = enumerator.range;
                enumStatement->fileName = context.fileName;
                enumStatement->declarationBoundaryAncestor = context.boundaryAncestor;
                enumStatement->typeInfo.qualifiedName = declaration.declarators.first().name;
                qlonglong numericValue = nextEnumeratorValue;
                if (!enumerator.value.isEmpty()) {
                    bool ok = false;
                    numericValue = evaluateIntegerConstant(enumerator.value, enumeratorValues, ok);
                    enumStatement->value = ok ? QString::number(numericValue) : enumerator.value;
                } else {
                    enumStatement->value = QString::number(numericValue);
                }
                enumeratorValues.insert(enumerator.name, numericValue);
                nextEnumeratorValue = numericValue + 1;
                enumStatement->properties.setFlag(StatementProperty::InProject, context.inProject);
                enumStatement->properties.setFlag(StatementProperty::InSystemHeader,
                                                  context.inSystemHeader);
                mModel.add(enumStatement);
                if (context.fileInfo)
                    context.fileInfo->addStatement(enumStatement);
            }
        }
        if (isFunction(declarator)) {
            const TypeDerivation &function = declarator.derivations.first();
            for (const ParameterInfo &parameter : function.parameters) {
                if (!parameter.name.isEmpty()) {
                    if (merged && statement->children.contains(parameter.name))
                        continue;
                    PStatement parameterStatement = createParameter(parameter, context, statement);
                    mModel.add(parameterStatement);
                    if (context.fileInfo)
                        context.fileInfo->addStatement(parameterStatement);
                }
            }
            if (declaration.bodyRange.isValid())
                buildChildren(declaration, context, statement);
        }
    }
    return result;
}

PStatement CppStatementBuilder::createStatement(const DeclarationInfo &declaration,
                                                const DeclaratorInfo &declarator,
                                                const StatementBuildContext &context,
                                                StatementKind kind)
{
    StatementBuilderProfileScope profileScope(statementBuilderProfile.createStatementNs);
    auto statement = std::make_shared<Statement>();
    statement->parentScope = context.parentScope;
    statement->command = declarator.name;
    statement->fullName = fullName(declarator, context);
    statement->kind = kind;
    statement->classKey = declaration.classKey;
    statement->scope = context.scope;
    statement->accessibility = accessibility(declaration, context);
    statement->declarationRange = declaration.range;
    statement->fileName = context.fileName;
    statement->declarationBoundaryAncestor = context.boundaryAncestor;
    statement->templateKind = declaration.templateKind;
    statement->templateParameters = declaration.templateParameters;
    statement->declSpecifierProperties = declaration.specifiers.properties;
    statement->baseClasses = declaration.baseClasses;
    statement->properties.setFlag(StatementProperty::Static,
                                  declaration.specifiers.properties.testFlag(DeclSpecifierProperty::Static));
    statement->properties.setFlag(StatementProperty::Virtual,
                                  declaration.specifiers.properties.testFlag(DeclSpecifierProperty::Virtual));
    statement->properties.setFlag(StatementProperty::Constexpr,
                                  declaration.specifiers.properties.testFlag(DeclSpecifierProperty::Constexpr));
    statement->properties.setFlag(StatementProperty::InProject, context.inProject);
    statement->properties.setFlag(StatementProperty::InSystemHeader, context.inSystemHeader);

    statement->typeInfo = declaration.specifiers.type;
    statement->typeInfo.derivations += declarator.derivations;
    if (isFunction(declarator)) {
        const TypeDerivation function = declarator.derivations.first();
        statement->typeInfo.derivations.removeFirst();
        statement->parameters = function.parameters;
        statement->args = renderParameters(function, true);
        statement->noNameArgs = renderParameters(function, false);
        statement->properties.setFlag(StatementProperty::Override,
                                      function.functionProperties.testFlag(FunctionProperty::Override));
        if (!function.trailingReturnType.isEmpty()
                && statement->typeInfo.qualifiedName == "auto")
            statement->typeInfo.qualifiedName = function.trailingReturnType;
    } else {
        for (const TypeDerivation &derivation : declarator.derivations) {
            if (derivation.kind == TypeDerivationKind::Function) {
                bool empty = derivation.parameters.isEmpty()
                        && !derivation.functionProperties.testFlag(FunctionProperty::Variadic);
                statement->args += empty ? "( )" : renderParameters(derivation, true);
                statement->noNameArgs += empty ? "( )" : renderParameters(derivation, false);
                statement->properties.setFlag(StatementProperty::FunctionPointer);
            } else if (derivation.kind == TypeDerivationKind::Array) {
                statement->args += derivation.text;
            }
        }
    }

    if (declaration.specifiers.type.qualifiedName == "auto"
            && !declaration.structuredBinding
            && (kind == StatementKind::Variable || kind == StatementKind::GlobalVariable
                || kind == StatementKind::LocalVariable)) {
        QString expression = declaration.rangeExpression.isEmpty()
                ? declarator.initializerText : declaration.rangeExpression;
        if (!expression.isEmpty()) {
            StatementBuilderProfileScope autoProfileScope(statementBuilderProfile.autoTypeNs);
            CppTokenizer tokenizer;
            tokenizer.tokenize(expression.split('\n'));
            tokenizer.relocateTokens(context.fileName, declaration.rangeExpression.isEmpty()
                                     ? declarator.initializerPosition : declaration.rangeExpressionPosition);
            PStatement evaluationContext = context.parentScope;
            if (!declaration.rangeExpression.isEmpty() && evaluationContext)
                evaluationContext = evaluationContext->parentScope.lock();
            EvalResult evaluated = mExpressionEvaluator.evaluate(tokenizer, evaluationContext);
            if (evaluated.isValid()) {
                TypeInfo initializerType = evaluated.type;
                if (!declaration.rangeExpression.isEmpty()) {
                    bool reference = !declarator.derivations.isEmpty()
                            && (declarator.derivations.first().kind == TypeDerivationKind::LvalueReference
                                || declarator.derivations.first().kind == TypeDerivationKind::RvalueReference);
                    initializerType = reference ? mTypeOperations.rangeReference(initializerType)
                                                : mTypeOperations.rangeValue(initializerType);
                }
                statement->typeInfo = deduceAutoType(initializerType, declarator,
                                                     declaration.specifiers);
            }
        }
    }

    bool hasBodyDefinition = isFunction(declarator) || kind == StatementKind::Class
            || kind == StatementKind::Namespace || kind == StatementKind::EnumType
            || kind == StatementKind::EnumClassType;
    if (hasBodyDefinition && declaration.bodyRange.isValid()) {
        statement->definitionRange = declaration.range;
        statement->definitionFileName = context.fileName;
        statement->definitionBoundaryAncestor = context.boundaryAncestor;
        statement->setHasDefinition(true);
    }
    if (kind == StatementKind::Alias || kind == StatementKind::NamespaceAlias)
        statement->value = declaration.specifiers.type.toString();
    if (kind == StatementKind::OverloadedOperator || kind == StatementKind::LiteralOperator)
        statement->properties.setFlag(StatementProperty::OperatorOverloading);
    return statement;
}

PStatement CppStatementBuilder::createParameter(const ParameterInfo &parameter,
                                                const StatementBuildContext &context,
                                                const PStatement &function)
{
    auto statement = std::make_shared<Statement>();
    statement->parentScope = function;
    statement->command = parameter.name;
    statement->fullName = function->fullName + "::" + parameter.name;
    statement->kind = StatementKind::Parameter;
    statement->scope = StatementScope::Local;
    statement->accessibility = StatementAccessibility::None;
    statement->declarationRange = SourceRange{parameter.position, parameter.position};
    statement->fileName = context.fileName;
    statement->declarationBoundaryAncestor = context.boundaryAncestor;
    if (parameter.type)
        statement->typeInfo = *parameter.type;
    statement->properties.setFlag(StatementProperty::InProject, context.inProject);
    statement->properties.setFlag(StatementProperty::InSystemHeader, context.inSystemHeader);
    return statement;
}

StatementKind CppStatementBuilder::declaratorKind(
        const DeclarationInfo &declaration, const DeclaratorInfo &declarator,
        const StatementBuildContext &context) const
{
    if (declaration.specifiers.properties.testFlag(DeclSpecifierProperty::Typedef))
        return StatementKind::Typedef;
    if (!isFunction(declarator))
        return StatementKind::Variable;
    if (declarator.name.startsWith("operator")) {
        return declarator.name.contains("\"\"")
                ? StatementKind::LiteralOperator : StatementKind::OverloadedOperator;
    }
    if (declarator.name.startsWith('~'))
        return StatementKind::Destructor;
    if (declarator.name == className(declarator, context))
        return StatementKind::Constructor;
    return StatementKind::Function;
}

StatementAccessibility CppStatementBuilder::accessibility(
        const DeclarationInfo &declaration, const StatementBuildContext &context) const
{
    switch (declaration.accessibility) {
    case DeclarationAccessibility::Public:
        return StatementAccessibility::Public;
    case DeclarationAccessibility::Protected:
        return StatementAccessibility::Protected;
    case DeclarationAccessibility::Private:
        return StatementAccessibility::Private;
    case DeclarationAccessibility::Default:
        return context.accessibility;
    }
    return StatementAccessibility::None;
}

QString CppStatementBuilder::fullName(const DeclaratorInfo &declarator,
                                      const StatementBuildContext &context) const
{
    QString name;
    if (!declarator.qualifier.isEmpty())
        name = declarator.qualifier + "::";
    name += declarator.name;
    PStatement parent = context.parentScope;
    if (!declarator.globallyQualified && parent && !parent->fullName.isEmpty()) {
        QString prefix = parent->fullName + "::";
        return name.startsWith(prefix) ? name : prefix + name;
    }
    return name;
}

void CppStatementBuilder::buildChildren(const DeclarationInfo &declaration,
                                        const StatementBuildContext &context,
                                        const PStatement &parent)
{
    StatementBuildContext childContext = context;
    childContext.parentScope = parent;
    if (parent->kind == StatementKind::Class)
        childContext.scope = StatementScope::ClassLocal;
    else if (parent->kind == StatementKind::Namespace)
        childContext.scope = StatementScope::Global;
    else
        childContext.scope = StatementScope::Local;
    childContext.accessibility = parent->kind == StatementKind::Class
            ? parent->accessibility : StatementAccessibility::None;
    for (const DeclarationInfo &child : declaration.children) {
        StatementBuildContext actualContext = childContext;
        if (!child.fileName.isEmpty())
            actualContext.fileName = child.fileName;
        if (actualContext.fileName != context.fileName)
            actualContext.fileInfo = nullptr;
        build(child, actualContext);
    }
}


QList<std::weak_ptr<Statement>> CppStatementBuilder::buildNamespace(
        const DeclarationInfo &declaration, const StatementBuildContext &context)
{
    QList<std::weak_ptr<Statement>> result;
    if (declaration.declarators.isEmpty())
        return result;
    StatementBuildContext current = context;
    QStringList parts = declaration.declarators.first().name.split(
                "::", Qt::SkipEmptyParts);
    if (parts.isEmpty())
        parts.append("");
    for (const QString &part : parts) {
        DeclaratorInfo declarator = declaration.declarators.first();
        declarator.name = part;
        declarator.qualifier.clear();
        declarator.globallyQualified = false;
        PStatement statement = createStatement(declaration, declarator, current,
                                               StatementKind::Namespace);
        PStatement merged = mergeDeclaration(statement, current);
        if (merged)
            statement = merged;
        else
            mModel.add(statement);
        if (current.fileInfo)
            current.fileInfo->addStatement(statement);
        current.parentScope = statement;
        current.scope = StatementScope::Global;
        result.clear();
        result.append(statement);
    }
    if (!result.empty()) {
        PStatement statement = result.first().lock();
        if (context.fileInfo && declaration.bodyRange.isValid())
            context.fileInfo->addScope(declaration.bodyRange, statement, current.parentScope);
        buildChildren(declaration, current, statement);
    }
    return result;
}

QList<std::weak_ptr<Statement>> CppStatementBuilder::buildUsing(
        const DeclarationInfo &declaration, const StatementBuildContext &context)
{
    QList<std::weak_ptr<Statement>> result;
    if (declaration.declarators.isEmpty())
        return result;
    const QString &target = declaration.declarators.first().name;
    if (declaration.usingKind == UsingDeclarationKind::Namespace
            || declaration.usingKind == UsingDeclarationKind::Enum) {
        if (context.parentScope)
            context.parentScope->usingList.insert(target);
        else if (context.fileInfo)
            context.fileInfo->addUsing(target);
        return result;
    }

    PStatement alias = createStatement(declaration, declaration.declarators.first(), context,
                                       StatementKind::Alias);
    int separator = target.lastIndexOf("::");
    alias->command = separator >= 0 ? target.mid(separator + 2) : target;
    alias->fullName = context.parentScope && !context.parentScope->fullName.isEmpty()
            ? context.parentScope->fullName + "::" + alias->command : alias->command;
    alias->value = target;
    PStatement targetStatement = mTypeResolver.findStatement(target, context.parentScope);
    if (targetStatement) {
        if (targetStatement->kind == StatementKind::Class
                || targetStatement->kind == StatementKind::EnumType
                || targetStatement->kind == StatementKind::EnumClassType
                || targetStatement->kind == StatementKind::Namespace) {
            alias->typeInfo.qualifiedName = targetStatement->fullName;
            alias->typeInfo.declaration = targetStatement;
        } else {
            alias->typeInfo = targetStatement->typeInfo;
        }
    } else {
        alias->typeInfo.qualifiedName = target;
    }
    mModel.add(alias);
    if (context.fileInfo)
        context.fileInfo->addStatement(alias);
    result.append(alias);
    return result;
}

PStatement CppStatementBuilder::createBlock(const DeclarationInfo &declaration,
                                            const StatementBuildContext &context)
{
    auto statement = std::make_shared<Statement>();
    statement->parentScope = context.parentScope;
    statement->command = "block";
    statement->fullName = context.parentScope && !context.parentScope->fullName.isEmpty()
            ? context.parentScope->fullName + "::block" : "block";
    statement->kind = StatementKind::Block;
    statement->scope = StatementScope::Local;
    statement->accessibility = StatementAccessibility::None;
    statement->declarationRange = declaration.range;
    statement->fileName = context.fileName;
    statement->declarationBoundaryAncestor = context.boundaryAncestor;
    statement->properties.setFlag(StatementProperty::DummyStatement);
    statement->properties.setFlag(StatementProperty::InProject, context.inProject);
    statement->properties.setFlag(StatementProperty::InSystemHeader, context.inSystemHeader);
    return statement;
}

TypeInfo CppStatementBuilder::deduceAutoType(TypeInfo initializerType,
                                             const DeclaratorInfo &declarator,
                                             const DeclSpecifierInfo &specifiers) const
{
    if (declarator.derivations.isEmpty()) {
        while (!initializerType.derivations.isEmpty()
                && (initializerType.derivations.first().kind == TypeDerivationKind::LvalueReference
                    || initializerType.derivations.first().kind == TypeDerivationKind::RvalueReference))
            initializerType.derivations.removeFirst();
        initializerType.qualifiers = TypeQualifier::None;
    }
    initializerType.derivations += declarator.derivations;
    initializerType.qualifiers |= specifiers.type.qualifiers;
    return initializerType;
}
