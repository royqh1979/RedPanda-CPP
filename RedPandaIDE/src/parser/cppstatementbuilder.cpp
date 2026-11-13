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

namespace {
bool isFunction(const DeclaratorInfo &declarator)
{
    return !declarator.derivations.isEmpty()
            && declarator.derivations.first().kind == TypeDerivationKind::Function;
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
}

CppStatementBuilder::CppStatementBuilder(StatementModel &model)
    : mModel(model)
{
}

QList<std::weak_ptr<Statement>> CppStatementBuilder::build(
        const DeclarationInfo &declaration, const StatementBuildContext &context)
{
    QList<std::weak_ptr<Statement>> result;
    if (declaration.kind == DeclarationKind::Using)
        return result;

    int firstVariable = 0;
    if (declaration.kind == DeclarationKind::Class
            || declaration.kind == DeclarationKind::Enum
            || declaration.kind == DeclarationKind::Namespace
            || declaration.kind == DeclarationKind::NamespaceAlias
            || declaration.kind == DeclarationKind::Alias)
        firstVariable = declaration.declarators.isEmpty() ? 0 : 1;

    for (int i = 0; i < declaration.declarators.count(); ++i) {
        const DeclaratorInfo &declarator = declaration.declarators[i];
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

        PStatement statement = createStatement(declaration, declarator, context, kind);
        if (i >= firstVariable && firstVariable > 0
                && (declaration.kind == DeclarationKind::Class
                    || declaration.kind == DeclarationKind::Enum)) {
            statement->typeInfo.qualifiedName = declaration.declarators.first().name;
        }
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
                enumStatement->value = enumerator.value;
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
                    PStatement parameterStatement = createParameter(parameter, context, statement);
                    mModel.add(parameterStatement);
                    if (context.fileInfo)
                        context.fileInfo->addStatement(parameterStatement);
                }
            }
        }
    }
    return result;
}

PStatement CppStatementBuilder::createStatement(const DeclarationInfo &declaration,
                                                const DeclaratorInfo &declarator,
                                                const StatementBuildContext &context,
                                                StatementKind kind)
{
    auto statement = std::make_shared<Statement>();
    statement->parentScope = context.parentScope;
    statement->command = declarator.name;
    statement->fullName = fullName(declarator, context);
    statement->kind = kind;
    statement->scope = context.scope;
    statement->accessibility = accessibility(declaration, context);
    statement->declarationRange = declaration.range;
    statement->fileName = context.fileName;
    statement->declarationBoundaryAncestor = context.boundaryAncestor;
    statement->templateKind = declaration.templateKind;
    statement->templateParameters = declaration.templateParameters;
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
        statement->args = renderParameters(function, true);
        statement->noNameArgs = renderParameters(function, false);
        if (!function.trailingReturnType.isEmpty()
                && statement->typeInfo.qualifiedName == "auto")
            statement->typeInfo.qualifiedName = function.trailingReturnType;
    } else {
        for (const TypeDerivation &derivation : declarator.derivations) {
            if (derivation.kind == TypeDerivationKind::Function) {
                statement->args += renderParameters(derivation, true);
                statement->noNameArgs += renderParameters(derivation, false);
                statement->properties.setFlag(StatementProperty::FunctionPointer);
            } else if (derivation.kind == TypeDerivationKind::Array) {
                statement->args += derivation.text;
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
    if (!isFunction(declarator)) {
        if (context.scope == StatementScope::Global)
            return StatementKind::GlobalVariable;
        if (context.scope == StatementScope::Local)
            return StatementKind::LocalVariable;
        return StatementKind::Variable;
    }
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
    if (!declarator.globallyQualified && parent && !parent->fullName.isEmpty())
        return parent->fullName + "::" + name;
    return name;
}

void CppStatementBuilder::buildChildren(const DeclarationInfo &declaration,
                                        const StatementBuildContext &context,
                                        const PStatement &parent)
{
    StatementBuildContext childContext = context;
    childContext.parentScope = parent;
    childContext.scope = parent->kind == StatementKind::Class
            ? StatementScope::ClassLocal : StatementScope::Global;
    childContext.accessibility = parent->kind == StatementKind::Class
            ? parent->accessibility : StatementAccessibility::None;
    for (const DeclarationInfo &child : declaration.children)
        build(child, childContext);
}
