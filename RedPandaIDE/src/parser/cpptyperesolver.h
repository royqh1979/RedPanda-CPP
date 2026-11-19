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

struct TemplateBindingFrame;
using PTemplateBindingFrame = std::shared_ptr<TemplateBindingFrame>;

enum class ConstraintSatisfaction {
    Satisfied,
    Unsatisfied,
    Unknown
};

struct ClassInstance {
    TypeInfo type;
    PStatement declaration;
    PTemplateBindingFrame bindings;
};

struct TypeNameSegment {
    QString name;
    QVector<TemplateArgument> arguments;
    bool hasTemplateArguments = false;
    bool templateKeyword = false;
};

struct ParsedTypeName {
    QVector<TypeNameSegment> segments;
    bool globallyQualified = false;
};

struct LookupKey {
    const AbstractStatement *scope = nullptr;
    QString name;
    bool typesOnly = false;

    bool operator==(const LookupKey &other) const
    {
        return scope == other.scope && name == other.name && typesOnly == other.typesOnly;
    }
};

inline uint qHash(const LookupKey &key, uint seed = 0) noexcept
{
    return qHash(key.name, seed) ^ qHash(reinterpret_cast<quintptr>(key.scope), seed)
            ^ (key.typesOnly ? 1 : 0);
}

struct TemplateBindingFrame {
    const AbstractStatement *owner = nullptr;
    bool complete = true;
    TemplateBindings values;
    QHash<QString, QVector<TemplateArgument>> parameterPacks;
    QHash<QString, ClassInstance> resolvedValues;
    std::shared_ptr<TemplateBindingFrame> parent;
    quint64 revision = 0;
};

inline bool sameBindingFrames(PTemplateBindingFrame left, PTemplateBindingFrame right)
{
    while (left && right) {
        if (left != right || left->revision != right->revision)
            return false;
        left = left->parent;
        right = right->parent;
    }
    return !left && !right;
}

struct ResolvedTypeKey {
    QString type;
    const AbstractStatement *declaration = nullptr;
    const AbstractStatement *context = nullptr;
    PTemplateBindingFrame bindings;

    bool operator==(const ResolvedTypeKey &other) const
    {
        return type == other.type && declaration == other.declaration
                && context == other.context && sameBindingFrames(bindings, other.bindings);
    }
};

inline uint qHash(const ResolvedTypeKey &key, uint seed = 0) noexcept
{
    uint result = qHash(key.type, seed) ^ qHash(reinterpret_cast<quintptr>(key.declaration), seed)
            ^ qHash(reinterpret_cast<quintptr>(key.context), seed);
    for (PTemplateBindingFrame frame = key.bindings; frame; frame = frame->parent) {
        result ^= qHash(reinterpret_cast<quintptr>(frame.get()), seed)
                ^ qHash(frame->revision, seed);
    }
    return result;
}

struct EvalResult {
    TypeInfo type;
    ClassInstance instance;
    EvalStatementKind kind = EvalStatementKind::Literal;
    PStatement baseStatement;
    PStatement statement;
    QVector<TemplateArgument> explicitTemplateArguments;

    bool isValid() const {
        return !type.qualifiedName.isEmpty() || kind == EvalStatementKind::Function
                || kind == EvalStatementKind::Label || kind == EvalStatementKind::Preprocessor;
    }
};

struct ExpressionEvalTraceEntry {
    int tokenIndex;
    PStatement statement;
};
using ExpressionEvalTrace = QVector<ExpressionEvalTraceEntry>;

struct MemberEvalResult {
    PStatement statement;
    TypeInfo type;
    ClassInstance instance;
};

enum class CacheLevel {
    None = 0,
    ParsedTypeText = 0x1,
    ParsedTypeName = 0x2,
    ParsedTypePattern = 0x4,
    BindingFrame = 0x8,
    ResolvedValue = 0x10,
    SelectedSpecialization = 0x20,
    Declaration = 0x40,
    Member = 0x80,
    ResolvedType = 0x100,
    All = 0x1ff
};
Q_DECLARE_FLAGS(CacheLevels, CacheLevel)
Q_DECLARE_OPERATORS_FOR_FLAGS(CacheLevels)

class CppTypeResolver
{
public:
    struct EvaluationContext {
        EvaluationContext();
        CacheLevels caches = CacheLevel::All;
        QSet<QString> aliases;
        QSet<QString> resolvingTypes;
        QSet<QString> resolvingConstraints;
        std::shared_ptr<QHash<QString, TypeInfo>> parsedTypeTexts =
                std::make_shared<QHash<QString, TypeInfo>>();
        std::shared_ptr<QHash<QString, ParsedTypeName>> parsedTypeNames =
                std::make_shared<QHash<QString, ParsedTypeName>>();
        std::shared_ptr<QHash<QString, ParsedTypeName>> parsedTypePatterns =
                std::make_shared<QHash<QString, ParsedTypeName>>();
        std::shared_ptr<QHash<QString, PTemplateBindingFrame>> bindingFrames =
                std::make_shared<QHash<QString, PTemplateBindingFrame>>();
        std::shared_ptr<QHash<QString, ClassInstance>> selectedSpecializations =
                std::make_shared<QHash<QString, ClassInstance>>();
        std::shared_ptr<QHash<ResolvedTypeKey, ClassInstance>> resolvedTypes =
                std::make_shared<QHash<ResolvedTypeKey, ClassInstance>>();
        std::shared_ptr<QHash<LookupKey, PStatement>> declarationResults =
                std::make_shared<QHash<LookupKey, PStatement>>();
        std::shared_ptr<QHash<LookupKey, PStatement>> memberResults =
                std::make_shared<QHash<LookupKey, PStatement>>();
        int operationCount = 0;
        bool resolutionFailed = false;
        bool allowModelChanges = false;
        ExpressionEvalTrace *identifierTrace = nullptr;
    };
    using ClassInstance = ::ClassInstance;
    using PTemplateBindingFrame = ::PTemplateBindingFrame;

    explicit CppTypeResolver(const StatementModel &model, EvaluationContext *evaluation = nullptr,
                             bool supportsClangTypeBuiltins = false);

    TypeInfo resolve(const TypeInfo &type, const TemplateBindings &bindings = {},
                     const PStatement &context = {}, EvaluationContext *evaluation = nullptr) const;
    ClassInstance resolveInstance(const TypeInfo &type, const TemplateBindings &bindings = {},
                                  const PStatement &context = {},
                                  EvaluationContext *evaluation = nullptr) const;
    ClassInstance resolveInContext(const TypeInfo &type, const PTemplateBindingFrame &bindings,
                                   const PStatement &context,
                                   EvaluationContext *evaluation = nullptr) const;
    PStatement findStatement(const QString &qualifiedName, const PStatement &context = {}) const;
    PStatement memberStatement(const TypeInfo &receiver, const QString &memberName,
                               EvaluationContext *evaluation = nullptr) const;
    TypeInfo memberType(const TypeInfo &receiver, const QString &memberName,
                        EvaluationContext *evaluation = nullptr) const;
    TypeInfo invokeResult(const TypeInfo &receiver, const QString &memberName,
                          EvaluationContext *evaluation = nullptr) const;
    MemberEvalResult invokeResult(const ClassInstance &receiver, const QString &memberName,
                                  EvaluationContext *evaluation = nullptr) const;
    MemberEvalResult memberAccess(const TypeInfo &receiver, const QString &memberName,
                                  EvaluationContext *evaluation = nullptr) const;
    MemberEvalResult memberAccess(const ClassInstance &receiver, const QString &memberName,
                                  EvaluationContext *evaluation = nullptr) const;
    MemberEvalResult invokeMember(const TypeInfo &receiver, const QString &memberName,
                                  const QVector<TypeInfo> &arguments = {},
                                  EvaluationContext *evaluation = nullptr) const;
    MemberEvalResult invokeMember(const ClassInstance &receiver, const QString &memberName,
                                  const QVector<TypeInfo> &arguments = {},
                                  EvaluationContext *evaluation = nullptr) const;
    TypeInfo invokeFunction(const PStatement &function, const QVector<TypeInfo> &arguments,
                            const QVector<TemplateArgument> &explicitArguments = {},
                            EvaluationContext *evaluation = nullptr,
                            ClassInstance *resultInstance = nullptr) const;
    QList<PStatement> membersOf(const PStatement &scope) const;
    QList<PStatement> resolveMembersOfClass(const PStatement &classStatement,
                                            const PStatement &accessScope) const;

private:
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
                       QSet<const AbstractStatement *> &visited, MemberEntryList &result) const;
    PStatement baseClass(const PStatement &scope, const QString &baseSpecifier,
                         StatementAccessibility &inheritance) const;
    StatementAccessibility applyInheritance(
            StatementAccessibility member, StatementAccessibility inheritance) const;
    void bindParameterType(const TypeInfo &parameter, const TypeInfo &argument,
                           const QSet<QString> &templateParameters, TemplateBindings &bindings,
                           EvaluationContext &evaluation) const;
    void materializeTemplateArguments(TypeInfo &type, EvaluationContext &evaluation) const;
    void materializeTemplateArgumentPatterns(TypeInfo &type, EvaluationContext &evaluation) const;
    QList<PStatement> functionCandidates(const PStatement &function) const;
    PTemplateBindingFrame matchFunctionCandidate(
            const PStatement &function, const QVector<TypeInfo> &arguments,
            const QVector<TemplateArgument> &explicitArguments,
            EvaluationContext &evaluation, const PTemplateBindingFrame &outerBindings,
            int *matchScore) const;
    QList<MemberResolution> memberCandidates(const ClassInstance &instance, const QString &name,
                                             QSet<QString> &visited,
                                             EvaluationContext &evaluation,
                                             bool stopAtDirectMember = false) const;
    MemberEvalResult resolveMember(const ClassInstance &receiver, const QString &memberName,
                                   bool call, const QVector<TypeInfo> &arguments,
                                   EvaluationContext &evaluation) const;
    bool compatibleArgument(const TypeInfo &parameter, const TypeInfo &argument,
                            const PTemplateBindingFrame &bindings, const PStatement &context,
                            EvaluationContext &evaluation, TypeInfo &resolvedParameter) const;
    TypeInfo resolveType(const TypeInfo &type, const PTemplateBindingFrame &bindings,
                         const PStatement &context, EvaluationContext &evaluation) const;
    ClassInstance resolveTypeInstance(const TypeInfo &type, const PTemplateBindingFrame &bindings,
                                      const PStatement &context, EvaluationContext &evaluation) const;
    ClassInstance resolveSegments(const TypeInfo &reference, const QVector<TypeNameSegment> &segments,
                                  bool globallyQualified, const PTemplateBindingFrame &bindings,
                                  const PStatement &context, EvaluationContext &evaluation) const;
    bool parseTypeNameSegments(const QString &text, QVector<TypeNameSegment> &segments,
                               bool &globallyQualified, EvaluationContext &evaluation,
                               bool materializeArguments = true) const;
    TypeInfo parseTypeText(const QString &text, EvaluationContext &evaluation,
                           bool materializeArguments = true) const;
    TemplateArgument resolveArgument(const QString &text, TemplateParameterKind kind,
                                     const PTemplateBindingFrame &bindings,
                                     const PStatement &context, EvaluationContext &evaluation) const;
    QString resolveConstantExpression(const QString &text, const PTemplateBindingFrame &bindings,
                                      const PStatement &context, EvaluationContext &evaluation) const;
    ConstraintSatisfaction constraintsSatisfied(const PStatement &statement,
                                                const PTemplateBindingFrame &bindings,
                                                const PStatement &context,
                                                EvaluationContext &evaluation) const;
    ConstraintSatisfaction evaluateConstraint(const QString &text,
                                              const PTemplateBindingFrame &bindings,
                                              const PStatement &context,
                                              EvaluationContext &evaluation) const;
    ConstraintSatisfaction evaluateConstraintAtom(const QString &text,
                                                  const PTemplateBindingFrame &bindings,
                                                  const PStatement &context,
                                                  EvaluationContext &evaluation) const;
    QString evaluateConstantValue(const QString &text, const PTemplateBindingFrame &bindings,
                                  const PStatement &context, EvaluationContext &evaluation) const;
    TypeInfo traitMemberType(const ClassInstance &instance, const QString &memberName,
                             EvaluationContext &evaluation) const;
    const TemplateArgument *findBinding(const PTemplateBindingFrame &bindings,
                                        const QString &name) const;
    const QVector<TemplateArgument> *findParameterPack(const PTemplateBindingFrame &bindings,
                                                       const QString &name) const;
    ClassInstance classInstanceFromResolvedType(const TypeInfo &type,
                                                const PTemplateBindingFrame &bindings,
                                                const PStatement &context,
                                                EvaluationContext &evaluation) const;
    ClassInstance selectSpecialization(const ClassInstance &primary,
                                       EvaluationContext &evaluation) const;
    SpecializationMatch matchSpecialization(const PStatement &specialization,
                                            const QVector<TemplateArgument> &arguments,
                                            const PStatement &context,
                                            TemplateBindings &bindings,
                                            EvaluationContext &evaluation) const;
    SpecializationMatch matchType(const TypeInfo &pattern, const TypeInfo &actual,
                                  const QSet<QString> &parameters, TemplateBindings &bindings,
                                  const PStatement &context, EvaluationContext &evaluation) const;
    SpecializationMatch matchArgument(const TemplateArgument &pattern,
                                      const TemplateArgument &actual,
                                      const QSet<QString> &parameters, TemplateBindings &bindings,
                                      const PStatement &context, EvaluationContext &evaluation) const;
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
    PStatement findMemberDeclaration(const PStatement &scope, const QString &name,
                                     bool typesOnly = false) const;
    PTemplateBindingFrame declarationBindings(const PStatement &declaration,
                                              const TypeInfo &instance,
                                              const PTemplateBindingFrame &outerBindings,
                                              EvaluationContext &evaluation) const;

    const StatementModel &mModel;
    EvaluationContext *mEvaluation;
    bool mSupportsClangTypeBuiltins;
};

#endif // CPPTYPERESOLVER_H
