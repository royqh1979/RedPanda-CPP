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
#ifndef CPPSTATEMENTBUILDER_H
#define CPPSTATEMENTBUILDER_H

#include "cppexpressionevaluator.h"

struct StatementBuildContext {
    QString fileName;
    QString boundaryAncestor;
    ParsedFileInfo *fileInfo = nullptr;
    PStatement parentScope;
    StatementScope scope = StatementScope::Global;
    StatementAccessibility accessibility = StatementAccessibility::None;
    bool inProject = false;
    bool inSystemHeader = false;
};

class CppStatementBuilder
{
public:
    explicit CppStatementBuilder(StatementModel &model);

    QList<std::weak_ptr<AbstractStatement>> build(const DeclarationInfo &declaration,
                                          const StatementBuildContext &context);

private:
    QList<std::weak_ptr<AbstractStatement>> buildNamespace(const DeclarationInfo &declaration,
                                                   const StatementBuildContext &context);
    QList<std::weak_ptr<AbstractStatement>> buildUsing(const DeclarationInfo &declaration,
                                               const StatementBuildContext &context);
    PStatement createBlock(const DeclarationInfo &declaration,
                           const StatementBuildContext &context);
    PStatement createStatement(const DeclarationInfo &declaration,
                               const DeclaratorInfo &declarator,
                               const StatementBuildContext &context,
                               StatementKind kind);
    PStatement createParameter(const ParameterInfo &parameter,
                               const StatementBuildContext &context,
                               const PStatement &function);
    QList<PStatement> doFindStatements(const QString &qualifiedName) const;
    QList<PStatement> doFindDeclarationRemainder(const PStatement &statement,
                                                 const QStringList &parts) const;
    PStatement doFindDeclarationToMerge(const PStatement &candidate,
                                        const StatementBuildContext &context) const;
    PStatement mergeDeclaration(const PStatement &candidate,
                                const StatementBuildContext &context) const;
    StatementKind declaratorKind(const DeclarationInfo &declaration,
                                 const DeclaratorInfo &declarator,
                                 const StatementBuildContext &context) const;
    StatementAccessibility accessibility(const DeclarationInfo &declaration,
                                         const StatementBuildContext &context) const;
    QString fullName(const DeclaratorInfo &declarator,
                     const StatementBuildContext &context) const;
    TypeInfo deduceAutoType(TypeInfo initializerType, const DeclaratorInfo &declarator,
                            const DeclSpecifierInfo &specifiers) const;
    void buildChildren(const DeclarationInfo &declaration,
                       const StatementBuildContext &context,
                       const PStatement &parent);
    void createInlineAliases(const PStatement &statement,
                             const StatementBuildContext &context);
    void createInlineAliasesForChildren(const PStatement &nameSpace,
                                        const StatementBuildContext &context);

    StatementModel &mModel;
    CppTypeResolver mTypeResolver;
    TypeOperationRegistry mTypeOperations;
    CppExpressionEvaluator mExpressionEvaluator;
};

#endif // CPPSTATEMENTBUILDER_H
