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
#ifndef CPPTYPERESOLVER_H
#define CPPTYPERESOLVER_H

#include "statementmodel.h"

using TemplateBindings = QHash<QString, TemplateArgument>;

struct EvalResult {
    TypeInfo type;
    EvalStatementKind kind = EvalStatementKind::Literal;
    PStatement baseStatement;
    PStatement statement;
    QVector<TemplateArgument> explicitTemplateArguments;

    bool isValid() const {
        return !type.qualifiedName.isEmpty() || kind == EvalStatementKind::Label
                || kind == EvalStatementKind::Preprocessor;
    }
};

class CppTypeResolver
{
public:
    explicit CppTypeResolver(const StatementModel &model);

    TypeInfo resolve(const TypeInfo &type, const TemplateBindings &bindings = {},
                     const PStatement &context = {}) const;
    PStatement findStatement(const QString &qualifiedName, const PStatement &context = {}) const;
    PStatement memberStatement(const TypeInfo &receiver, const QString &memberName) const;
    TypeInfo memberType(const TypeInfo &receiver, const QString &memberName) const;
    TypeInfo invokeResult(const TypeInfo &receiver, const QString &memberName) const;
    TypeInfo invokeFunction(const PStatement &function, const QVector<TypeInfo> &arguments,
                            const QVector<TemplateArgument> &explicitArguments = {}) const;
    QList<PStatement> membersOf(const PStatement &scope) const;
    StatementAccessibility memberAccessibility(const PStatement &scope,
                                               const PStatement &member) const;

private:
    struct TemplateBindingFrame {
        const Statement *owner = nullptr;
        TemplateBindings values;
        std::shared_ptr<TemplateBindingFrame> parent;
    };
    using PTemplateBindingFrame = std::shared_ptr<TemplateBindingFrame>;
    struct EvaluationContext {
        QSet<QString> aliases;
        QSet<QString> resolvingTypes;
        int operationCount = 0;
        bool resolutionFailed = false;
    };
    struct TypeNameSegment {
        QString name;
        QVector<TemplateArgument> arguments;
        bool hasTemplateArguments = false;
        bool templateKeyword = false;
    };
    struct ClassInstance {
        TypeInfo type;
        PStatement declaration;
        PTemplateBindingFrame bindings;
    };
    struct MemberResolution {
        PStatement statement;
        ClassInstance owner;
        StatementAccessibility accessibility = StatementAccessibility::None;
    };
    enum class SpecializationMatch {
        Match,
        NoMatch,
        Unknown
    };
    struct MemberEntry {
        PStatement statement;
        StatementAccessibility accessibility;
    };
    using MemberEntryList = QList<MemberEntry>;
    void appendMembers(const PStatement &scope, bool inherited,
                       QSet<const Statement *> &visited, MemberEntryList &result) const;
    PStatement baseClass(const PStatement &scope, const QString &baseSpecifier,
                         StatementAccessibility &inheritance) const;
    StatementAccessibility applyInheritance(
            StatementAccessibility member, StatementAccessibility inheritance) const;
    void bindParameterType(const TypeInfo &parameter, const TypeInfo &argument,
                           const QSet<QString> &templateParameters, TemplateBindings &bindings) const;
    QList<PStatement> functionCandidates(const PStatement &function) const;
    TypeInfo invokeFunctionCandidate(const PStatement &function, const QVector<TypeInfo> &arguments,
                                     const QVector<TemplateArgument> &explicitArguments) const;
    bool compatibleArgument(const TypeInfo &parameter, const TypeInfo &argument,
                            const PTemplateBindingFrame &bindings, const PStatement &context,
                            EvaluationContext &evaluation) const;
    TypeInfo resolveType(const TypeInfo &type, const PTemplateBindingFrame &bindings,
                         const PStatement &context, EvaluationContext &evaluation) const;
    TypeInfo resolveSegments(const TypeInfo &reference, const QVector<TypeNameSegment> &segments,
                             bool globallyQualified, const PTemplateBindingFrame &bindings,
                             const PStatement &context, EvaluationContext &evaluation) const;
    bool parseTypeNameSegments(const QString &text, QVector<TypeNameSegment> &segments,
                               bool &globallyQualified) const;
    TypeInfo parseTypeText(const QString &text) const;
    TemplateArgument resolveArgument(const QString &text, TemplateParameterKind kind,
                                     const PTemplateBindingFrame &bindings,
                                     const PStatement &context, EvaluationContext &evaluation) const;
    QString resolveConstantExpression(const QString &text, const PTemplateBindingFrame &bindings,
                                      const PStatement &context, EvaluationContext &evaluation) const;
    TypeInfo traitMemberType(const ClassInstance &instance, const QString &memberName,
                             EvaluationContext &evaluation) const;
    const TemplateArgument *findBinding(const PTemplateBindingFrame &bindings,
                                        const QString &name) const;
    ClassInstance classInstance(const TypeInfo &type, const PTemplateBindingFrame &bindings,
                                const PStatement &context, EvaluationContext &evaluation) const;
    ClassInstance selectSpecialization(const ClassInstance &primary,
                                       EvaluationContext &evaluation) const;
    SpecializationMatch matchSpecialization(const PStatement &specialization,
                                            const QVector<TemplateArgument> &arguments,
                                            const PStatement &context,
                                            TemplateBindings &bindings) const;
    SpecializationMatch matchType(const TypeInfo &pattern, const TypeInfo &actual,
                                  const QSet<QString> &parameters, TemplateBindings &bindings,
                                  const PStatement &context) const;
    SpecializationMatch matchArgument(const TemplateArgument &pattern,
                                      const TemplateArgument &actual,
                                      const QSet<QString> &parameters, TemplateBindings &bindings,
                                      const PStatement &context) const;
    MemberResolution findMember(const ClassInstance &instance, const QString &name,
                                EvaluationContext &evaluation) const;
    MemberResolution findMember(const ClassInstance &instance, const QString &name,
                                QSet<QString> &visited, EvaluationContext &evaluation) const;
    void applyReference(TypeInfo &target, const TypeInfo &reference) const;
    PStatement findDeclaration(const QString &qualifiedName, const PStatement &context,
                               bool typesOnly = false) const;
    PStatement findInNamespaceScope(const PStatement &scope, const QStringList &parts,
                                    bool &firstPartFound, bool typesOnly) const;
    PStatement findDeclarationRemainder(const PStatement &statement,
                                        const QStringList &parts, bool typesOnly) const;
    PStatement findMember(const PStatement &scope, const QString &name, bool typesOnly = false) const;
    PStatement findMemberRecursive(const PStatement &scope, const QString &name,
                                   QSet<const Statement *> &visited, bool typesOnly) const;
    PTemplateBindingFrame declarationBindings(const PStatement &declaration,
                                              const TypeInfo &instance,
                                              const PTemplateBindingFrame &outerBindings,
                                              EvaluationContext &evaluation) const;

    const StatementModel &mModel;
};

#endif // CPPTYPERESOLVER_H
