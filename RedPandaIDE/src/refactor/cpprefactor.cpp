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
#include "cpprefactor.h"
#include <algorithm>
#include <QFile>
#include <qsynedit/document.h>
#include "../editor/editor.h"
#include "../settings.h"
#include "../syntaxermanager.h"
#include "../utils/file.h"

using QSynedit::CharPos;

int SymbolReferenceResults::count() const
{
    int result = 0;
    for (const SymbolReferenceFile &file : files)
        result += file.references.count();
    return result;
}

CppRefactor::CppRefactor(
        std::shared_ptr<Project> project,
        ContentProvider contentProvider,
        EditorProvider editorProvider,
        QObject *parent)
    : QObject(parent),
      mProject(std::move(project)),
      mContentProvider(std::move(contentProvider)),
      mEditorProvider(std::move(editorProvider))
{
}

static bool validNewIdentifier(Editor *editor, const QString &newWord)
{
    if (!editor || newWord.isEmpty() || isCppKeyword(newWord)
            || !editor->isIdentStartChar(newWord.front()))
        return false;
    for (const QChar &ch : newWord) {
        if (!editor->isIdentChar(ch))
            return false;
    }
    return true;
}

static QString parentName(const PStatement &statement)
{
    PStatement parent = statement ? statement->parentScope.lock() : nullptr;
    return parent ? parent->fullName : QString();
}

static QStringList completeCallExpression(
        Editor *editor,
        QStringList expression,
        const CharPos &pos)
{
    CharPos wordBegin;
    CharPos wordEnd;
    if (getWordAtPosition(editor, pos, wordBegin, wordEnd,
                          Editor::WordPurpose::wpEvaluation).isEmpty())
        return expression;
    QString lineText = editor->lineText(pos.line);
    int next = wordEnd.ch;
    while (next < lineText.size() && lineText.at(next).isSpace())
        ++next;
    if (next >= lineText.size() || lineText.at(next) != '(')
        return expression;
    CppTokenizer tokenizer;
    tokenizer.tokenize({lineText.mid(next)});
    int depth = 0;
    for (const CppTokenizer::Token &token : tokenizer.tokens()) {
        expression.append(token.text);
        if (token.text == "(") {
            ++depth;
        } else if (token.text == ")" && --depth == 0) {
            return expression;
        }
    }
    return {};
}

bool CppRefactor::findOccurrences(
        Editor *editor, const CharPos &pos, SymbolReferenceResults &results)
{
    if (!editor || !editor->parser())
        return false;
    PStatement statement;
    SymbolReferenceScope scope;
    {
        if (!editor->parser()->freeze())
            return false;
        auto unfreeze = finally([&editor] {
            editor->parser()->unFreeze();
        });
        QStringList expression = CppExpressionExtractor::extract(
                    editor, editor->syntaxer(), pos);
        expression = completeCallExpression(editor, expression, pos);
        if (expression.isEmpty())
            return false;
        statement = editor->parser()->findStatementOf(
                    editor->filename(), expression, pos.line);
        if (!statement)
            return false;
        scope = statement->scope == StatementScope::Local
                || !editor->inProject() || !mProject
                ? SymbolReferenceScope::CurrentFile
                : SymbolReferenceScope::WholeProject;
    }
    return doFindOccurrences(editor, statement, {statement}, scope, results);
}

bool CppRefactor::findOccurrences(
        Editor *editor,
        const QString &keyword,
        const QString &scopeFullName,
        SymbolReferenceScope scope,
        SymbolReferenceResults &results)
{
    if (!editor || !editor->parser())
        return false;
    PStatement statement;
    {
        if (!editor->parser()->freeze())
            return false;
        auto unfreeze = finally([&editor] {
            editor->parser()->unFreeze();
        });
        if (!scopeFullName.isEmpty()) {
            PStatement scopeStatement = editor->parser()->findStatement(scopeFullName);
            if (!scopeStatement)
                return false;
            statement = scopeStatement->kind == StatementKind::Namespace
                    ? std::dynamic_pointer_cast<NamespaceStatement>(scopeStatement)
                        ->children.value(keyword).lock()
                    : std::dynamic_pointer_cast<Statement>(scopeStatement)
                        ->children.value(keyword).lock();
        } else {
            statement = editor->parser()->findStatement(keyword);
        }
        if (!statement)
            return false;
        if (statement->scope == StatementScope::Local)
            scope = SymbolReferenceScope::CurrentFile;
    }
    return doFindOccurrences(editor, statement, {statement}, scope, results);
}

static QString baseCommand(QString command)
{
    if (command.startsWith('~'))
        command.remove(0, 1);
    int angle = command.indexOf('<');
    if (angle >= 0)
        command.truncate(angle);
    return command;
}

static bool sameOccurrence(
        const StatementOccurrence &left,
        const StatementOccurrence &right)
{
    return left.range.start.isValid() && right.range.start.isValid()
            && left.fileName == right.fileName
            && left.range.start == right.range.start;
}

static bool sharesOccurrence(
        const PStatement &left,
        const PStatement &right)
{
    for (const StatementOccurrence &leftDeclaration : left->declarations) {
        for (const StatementOccurrence &rightDeclaration : right->declarations) {
            if (sameOccurrence(leftDeclaration, rightDeclaration))
                return true;
        }
    }
    if (statementHasDefinition(left) && statementHasDefinition(right)
            && sameOccurrence(left->definition, right->definition))
        return true;
    return false;
}

static bool declarationNamePosition(
        const PStatement &statement,
        const QString &fileName,
        const SourcePosition &pos)
{
    auto containsName = [&](const StatementOccurrence &occurrence) {
        return occurrence.fileName == fileName
                && occurrence.range.start.line == pos.line
                && occurrence.range.contains(pos);
    };
    for (const StatementOccurrence &declaration : statement->declarations) {
        if (containsName(declaration))
            return true;
    }
    return statementHasDefinition(statement) && containsName(statement->definition);
}

static bool matchesTarget(
        const PStatement &tokenStatement,
        const PStatement &target,
        const QString &fileName,
        const SourcePosition &pos,
        const QString &token)
{
    if (tokenStatement == target)
        return true;
    if (tokenStatement && sharesOccurrence(tokenStatement, target))
        return true;
    if (baseCommand(target->command) != token)
        return false;
    if (tokenStatement && tokenStatement->kind != target->kind
            && tokenStatement->kind != StatementKind::Class
            && tokenStatement->kind != StatementKind::Constructor
            && tokenStatement->kind != StatementKind::Destructor)
        return false;
    return declarationNamePosition(target, fileName, pos);
}

static SymbolReferenceRole referenceRole(
        const PStatement &statement,
        const QString &fileName,
        const SourcePosition &pos)
{
    for (const StatementOccurrence &declaration : statement->declarations) {
        if (declaration.fileName == fileName
                && declaration.range.start.line == pos.line
                && declaration.range.contains(pos))
            return SymbolReferenceRole::Declaration;
    }
    if (statementHasDefinition(statement)
            && statement->definition.fileName == fileName
            && statement->definition.range.start.line == pos.line
            && statement->definition.range.contains(pos))
        return SymbolReferenceRole::Definition;
    return SymbolReferenceRole::Reference;
}

bool CppRefactor::doFindOccurrences(
        Editor *editor,
        const PStatement &statement,
        const QVector<PStatement> &targets,
        SymbolReferenceScope scope,
        SymbolReferenceResults &results)
{
    PStatement parent = statement->parentScope.lock();
    results.symbol = statement->command;
    results.scopeFullName = parent ? parent->fullName : QString();
    results.scope = scope;
    results.targets = targets;
    results.files.clear();

    if (scope == SymbolReferenceScope::CurrentFile) {
        if (!editor->parser()->freeze())
            return false;
        auto unfreeze = finally([&editor] {
            editor->parser()->unFreeze();
        });
        SymbolReferenceFile file = findOccurrencesInFile(
                    editor->filename(), editor->editorEncoding(),
                    targets, editor->parser(), editor->content());
        file.documentVersion = editor->document()->contentVersion();
        if (!file.references.isEmpty())
            results.files.append(file);
    } else if (mProject) {
        findOccurrencesInProject(editor, targets, results);
    }
    return true;
}

SymbolReferenceFile CppRefactor::findOccurrencesInFile(
        const QString &fileName,
        const QByteArray &fileEncoding,
        const QVector<PStatement> &targets,
        const PCppParser &parser,
        const QStringList &openContent,
        const QString &contextFile)
{
    SymbolReferenceFile result;
    result.fileName = fileName;
    QStringList buffer;
    Editor editor(nullptr);
    editor.setEditorSettings(&pSettings->editor());
    FileType fileType = getFileType(fileName);
    if (!isC_CPPSourceFile(fileType))
        fileType = FileType::CCppHeader;
    editor.setFileType(fileType);
    editor.setFilename(fileName);
    if (!openContent.isEmpty()) {
        editor.loadContent(openContent);
    } else if (mContentProvider && mContentProvider(fileName, buffer)) {
        editor.loadContent(buffer);
    } else if (!fileExists(fileName)) {
        return result;
    } else {
        QByteArray encoding;
        try {
            editor.loadFromFile(fileName, fileEncoding, encoding);
        } catch (const FileError &) {
            return result;
        }
    }
    result.encoding = editor.editorEncoding();
    QSynedit::PSyntaxer syntaxer = SyntaxerManager::getSyntaxer(
                QSynedit::ProgrammingLanguage::CPP);
    editor.setSyntaxer(syntaxer);

    for (int line = 0; line < editor.lineCount(); ++line) {
        QString lineText = editor.lineText(line);
        if (lineText.isEmpty())
            continue;
        editor.startParseLine(syntaxer.get(), line);
        while (!syntaxer->eol()) {
            int start = syntaxer->getTokenPos();
            QString token = syntaxer->getToken();
            QSynedit::PTokenAttribute attr = syntaxer->getTokenAttribute();
            bool candidateName = false;
            for (const PStatement &target : targets) {
                if (baseCommand(target->command) == token) {
                    candidateName = true;
                    break;
                }
            }
            if (attr && syntaxer->getTokenType() == QSynedit::TokenType::Identifier
                    && candidateName) {
                CharPos pos{start, line};
                QStringList expression = CppExpressionExtractor::extract(
                            &editor, editor.syntaxer(), pos);
                int next = start + token.length();
                while (next < lineText.size() && lineText.at(next).isSpace())
                    ++next;
                if (next < lineText.size() && lineText.at(next) == '(')
                    expression = completeCallExpression(&editor, expression, pos);
                PStatement tokenStatement;
                if (contextFile.isEmpty() || contextFile == fileName) {
                    tokenStatement = parser->findStatementOf(fileName, expression, line);
                } else {
                    PStatement scope = parser->findScopeStatement(fileName, line);
                    tokenStatement = parser->findStatementOf(
                                contextFile, expression, scope);
                }
                if (!tokenStatement && expression.size() == 1
                        && (next == lineText.size() || lineText.at(next) != '('))
                    tokenStatement = parser->findStatement(expression.constFirst());
                for (const PStatement &target : targets) {
                    if (!matchesTarget(tokenStatement, target, fileName,
                                       {line, start}, token))
                        continue;
                    result.references.append(
                                {fileName, line, start, token.length(), lineText,
                                 referenceRole(target, fileName, {line, start})});
                    break;
                }
            }
            syntaxer->next();
        }
    }
    return result;
}

QVector<PStatement> CppRefactor::overloadsOf(
        Editor *editor, const CharPos &pos)
{
    if (!editor || !editor->parser())
        return {};
    if (!editor->parser()->freeze())
        return {};
    auto unfreeze = finally([&editor] {
        editor->parser()->unFreeze();
    });
    QStringList expression = CppExpressionExtractor::extract(
                editor, editor->syntaxer(), pos);
    expression = completeCallExpression(editor, expression, pos);
    if (expression.isEmpty())
        return {};
    return overloadsOf(editor->parser(),
                       editor->parser()->findStatementOf(
                           editor->filename(), expression, pos.line));
}

QVector<PStatement> CppRefactor::overloadsOf(
        const PCppParser &parser,
        const PStatement &statement) const
{
    QVector<PStatement> result;
    if (!statement)
        return result;
    PStatement parent = statement->parentScope.lock();
    const StatementMap &siblings = parser->statementList().childrenStatements(parent);
    for (const std::weak_ptr<AbstractStatement> &weak : siblings.values(statement->command)) {
        PStatement sibling = weak.lock();
        if (sibling && (sibling->kind == StatementKind::Function
                        || sibling->kind == StatementKind::OverloadedOperator
                        || sibling->kind == StatementKind::LiteralOperator))
            result.append(sibling);
    }
    return result;
}

static bool isFunction(const PStatement &statement)
{
    return statement && (statement->kind == StatementKind::Function
                         || statement->kind == StatementKind::OverloadedOperator
                         || statement->kind == StatementKind::LiteralOperator);
}

static bool sameFunctionSignature(
        const PStatement &left,
        const PStatement &right)
{
    if (!isFunction(left) || !isFunction(right))
        return false;
    auto normalLeft = std::dynamic_pointer_cast<Statement>(left);
    auto normalRight = std::dynamic_pointer_cast<Statement>(right);
    return normalLeft->normalizedArgs == normalRight->normalizedArgs
            && left->properties.testFlag(StatementProperty::Const)
                == right->properties.testFlag(StatementProperty::Const)
            && left->properties.testFlag(StatementProperty::LvalueReference)
                == right->properties.testFlag(StatementProperty::LvalueReference)
            && left->properties.testFlag(StatementProperty::RvalueReference)
                == right->properties.testFlag(StatementProperty::RvalueReference);
}

static bool sameSignature(
        const PStatement &left,
        const PStatement &right)
{
    return left && right && left->command == right->command
            && sameFunctionSignature(left, right);
}

static bool hasCallableSignature(const PStatement &statement)
{
    return isFunction(statement)
            || (statement && (statement->kind == StatementKind::Constructor
                              || statement->kind == StatementKind::Destructor));
}

static bool hasAnonymousNamespaceScope(const PStatement &statement)
{
    PStatement parent = statement ? statement->parentScope.lock() : nullptr;
    while (parent) {
        if (parent->kind == StatementKind::Namespace && parent->command.isEmpty())
            return true;
        parent = parent->parentScope.lock();
    }
    return false;
}

struct SymbolIdentity {
    StatementKind kind;
    QString fullName;
    QString parentFullName;
    StatementScope scope;
    bool internalLinkage;
    bool externalDeclaration;
    QString normalizedArgs;
    bool isConst;
    bool isLvalueReference;
    bool isRvalueReference;
    QVector<StatementOccurrence> occurrences;
    QString translationUnit;
};

static SymbolIdentity symbolIdentity(
        const PStatement &statement,
        ParserLanguage language)
{
    SymbolIdentity result{
        statement->kind, statement->fullName, parentName(statement), statement->scope,
        (statement->scope == StatementScope::Global && statementIsStatic(statement))
                || hasAnonymousNamespaceScope(statement),
        false, QString(), false, false, false, {}, QString()
    };
    for (const StatementOccurrence &declaration : statement->declarations)
        result.occurrences.append(declaration);
    if (statementHasDefinition(statement))
        result.occurrences.append(statement->definition);
    if (auto normal = std::dynamic_pointer_cast<Statement>(statement)) {
        result.externalDeclaration = normal->declSpecifierProperties.testFlag(
                    DeclSpecifierProperty::Extern);
        if (language == ParserLanguage::CPlusPlus
                && statement->scope == StatementScope::Global
                && !result.externalDeclaration
                && (statement->kind == StatementKind::Variable
                    || statement->kind == StatementKind::GlobalVariable)
                && (normal->typeInfo.qualifiers.testFlag(TypeQualifier::Const)
                    || normal->declSpecifierProperties.testFlag(
                        DeclSpecifierProperty::Constexpr)))
            result.internalLinkage = true;
        if (hasCallableSignature(statement)) {
            result.normalizedArgs = normal->normalizedArgs;
            result.isConst = statement->properties.testFlag(StatementProperty::Const);
            result.isLvalueReference = statement->properties.testFlag(
                        StatementProperty::LvalueReference);
            result.isRvalueReference = statement->properties.testFlag(
                        StatementProperty::RvalueReference);
        }
    }
    return result;
}

static bool matchesSymbolIdentity(
        const PStatement &statement,
        const SymbolIdentity &identity,
        const QString &translationUnit,
        ParserLanguage language)
{
    if (!statement || statement->kind != identity.kind)
        return false;
    SymbolIdentity candidate = symbolIdentity(statement, language);
    if (hasCallableSignature(statement)
            && (candidate.normalizedArgs != identity.normalizedArgs
                || candidate.isConst != identity.isConst
                || candidate.isLvalueReference != identity.isLvalueReference
                || candidate.isRvalueReference != identity.isRvalueReference))
        return false;
    if (statement->command.startsWith("__RED_PANDA_IDE__")) {
        if (candidate.parentFullName != identity.parentFullName)
            return false;
        for (const StatementOccurrence &left : candidate.occurrences) {
            for (const StatementOccurrence &right : identity.occurrences) {
                if (sameOccurrence(left, right))
                    return true;
            }
        }
        return false;
    }
    if (candidate.fullName != identity.fullName)
        return false;
    if (candidate.scope == StatementScope::Local && !candidate.externalDeclaration)
        return false;
    if (candidate.internalLinkage != identity.internalLinkage)
        return false;
    return !candidate.internalLinkage || identity.translationUnit == translationUnit;
}

static QVector<PStatement> equivalentTargetsInCurrentParse(
        const PCppParser &parser,
        const QVector<SymbolIdentity> &identities,
        const QString &translationUnit)
{
    QVector<PStatement> result;
    for (const PStatement &candidate : parser->statementList().statements()) {
        for (const SymbolIdentity &identity : identities) {
            if (matchesSymbolIdentity(candidate, identity, translationUnit,
                                      parser->language())) {
                result.append(candidate);
                break;
            }
        }
    }
    return result;
}

void CppRefactor::findOccurrencesInProject(
        Editor *editor,
        const QVector<PStatement> &targets,
        SymbolReferenceResults &results)
{
    PCppParser parser = editor->parser();
    QVector<SymbolIdentity> identities;
    {
        if (!parser->freeze())
            return;
        auto unfreeze = finally([&parser] {
            parser->unFreeze();
        });
        for (const PStatement &target : targets) {
            SymbolIdentity identity = symbolIdentity(target, parser->language());
            identity.translationUnit = editor->contextFile().isEmpty()
                    ? editor->filename() : editor->contextFile();
            identities.append(identity);
        }
    }

    QStringList translationUnits;
    for (const PProjectUnit &unit : mProject->unitList()) {
        if (isC_CPPSourceFile(getFileType(unit->fileName())))
            translationUnits.append(unit->fileName());
    }
    if (translationUnits.isEmpty())
        translationUnits.append(editor->filename());

    auto restoreCurrentParse = finally([&parser, &editor] {
        CppParser::parseFileBlocking(parser, editor->filename(), editor->inProject(),
                                     editor->contextFile(), false, false);
    });
    for (const QString &translationUnit : translationUnits) {
        CppParser::parseFileBlocking(parser, translationUnit, true, "", false, false);
        if (!parser->freeze())
            continue;
        auto unfreeze = finally([&parser] {
            parser->unFreeze();
        });
        QVector<PStatement> currentTargets = equivalentTargetsInCurrentParse(
                    parser, identities, translationUnit);
        if (currentTargets.isEmpty())
            continue;
        QStringList files{translationUnit};
        QStringList includedFiles = parser->getIncludedFiles(translationUnit).values();
        std::sort(includedFiles.begin(), includedFiles.end());
        files.append(includedFiles);
        for (const QString &fileName : files) {
            QByteArray encoding = ENCODING_AUTO_DETECT;
            for (const PProjectUnit &unit : mProject->unitList()) {
                if (unit->fileName() == fileName) {
                    encoding = unit->encoding() == ENCODING_PROJECT
                            ? mProject->options().encoding : unit->encoding();
                    break;
                }
            }
            SymbolReferenceFile file = findOccurrencesInFile(
                        fileName, encoding, currentTargets, parser,
                        fileName == editor->filename()
                        ? editor->content() : QStringList(), translationUnit);
            if (file.references.isEmpty())
                continue;
            if (mEditorProvider) {
                Editor *openEditor = mEditorProvider(fileName);
                if (openEditor)
                    file.documentVersion = openEditor->document()->contentVersion();
            }
            if (fileName == editor->filename())
                file.documentVersion = editor->document()->contentVersion();
            auto fileIt = std::find_if(
                        results.files.begin(), results.files.end(),
                        [&fileName](const SymbolReferenceFile &existing) {
                return existing.fileName == fileName;
            });
            if (fileIt == results.files.end()) {
                results.files.append(file);
                continue;
            }
            for (const SymbolReference &reference : file.references) {
                bool duplicate = false;
                for (const SymbolReference &existing : fileIt->references) {
                    if (existing.line == reference.line && existing.ch == reference.ch
                            && existing.length == reference.length) {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate)
                    fileIt->references.append(reference);
            }
        }
    }
}

static bool hasRenameNameConflict(
        const PCppParser &parser,
        const QVector<PStatement> &targets,
        const QString &newWord)
{
    QSet<PStatement> targetSet(targets.begin(), targets.end());
    for (const PStatement &target : targets) {
        QVector<PStatement> scopes{target->parentScope.lock()};
        if (target->kind == StatementKind::Class)
            scopes.append(target);
        for (const PStatement &scope : scopes) {
            const StatementMap &siblings = parser->statementList().childrenStatements(scope);
            for (const std::weak_ptr<AbstractStatement> &weak : siblings.values(newWord)) {
                PStatement candidate = weak.lock();
                if (!candidate || targetSet.contains(candidate))
                    continue;
                if (isFunction(target) && isFunction(candidate)
                        && !sameFunctionSignature(target, candidate))
                    continue;
                return true;
            }
        }
    }
    return false;
}

static QString normalizedBaseName(QString name)
{
    name = name.trimmed();
    for (const QString &prefix : {"public ", "protected ", "private ", "virtual "}) {
        if (name.startsWith(prefix))
            name = name.mid(prefix.size()).trimmed();
    }
    if (name.startsWith("::"))
        name = name.mid(2);
    int angle = name.indexOf('<');
    if (angle >= 0)
        name.truncate(angle);
    return name.trimmed();
}

static PStatement findClass(
        const PCppParser &parser,
        const PStatement &classStatement,
        const QString &name)
{
    if (!classStatement)
        return nullptr;
    for (const PStatement &candidate : parser->statementList().statements()) {
        if (candidate->kind == StatementKind::Class
                && (candidate->fullName == name || candidate->command == name))
            return candidate;
    }
    return nullptr;
}

void CppRefactor::collectOverrides(
        const PCppParser &parser,
        const PStatement &statement,
        QSet<PStatement> &targets,
        QSet<PStatement> &visited) const
{
    if (!statement || visited.contains(statement))
        return;
    visited.insert(statement);
    targets.insert(statement);
    PStatement parent = statement->parentScope.lock();
    if (!parent || parent->kind != StatementKind::Class)
        return;
    auto normalParent = std::dynamic_pointer_cast<Statement>(parent);

    for (const QString &baseName : normalParent->baseClasses) {
        PStatement base = findClass(parser, parent, normalizedBaseName(baseName));
        if (!base)
            continue;
        for (const std::weak_ptr<AbstractStatement> &weak : std::dynamic_pointer_cast<Statement>(base)
                 ->children.values(statement->command)) {
            PStatement candidate = weak.lock();
            if (sameSignature(statement, candidate))
                collectOverrides(parser, candidate, targets, visited);
        }
    }

    for (const PStatement &derived : parser->statementList().statements()) {
        if (derived->kind != StatementKind::Class)
            continue;
        auto normalDerived = std::dynamic_pointer_cast<Statement>(derived);
        bool inherits = false;
        for (const QString &baseName : normalDerived->baseClasses) {
            QString normalized = normalizedBaseName(baseName);
            if (normalized == parent->fullName || normalized == parent->command) {
                inherits = true;
                break;
            }
        }
        if (!inherits)
            continue;
        for (const std::weak_ptr<AbstractStatement> &weak
             : normalDerived->children.values(statement->command)) {
            PStatement candidate = weak.lock();
            if (sameSignature(statement, candidate))
                collectOverrides(parser, candidate, targets, visited);
        }
    }
}

QVector<PStatement> CppRefactor::collectClassTargets(
        const PCppParser &parser,
        const PStatement &classStatement) const
{
    QSet<PStatement> targets;
    if (!classStatement)
        return {};
    for (const PStatement &candidate : parser->statementList().statements()) {
        if (candidate->kind == StatementKind::Class
                && baseCommand(candidate->command) == classStatement->command
                && parentName(candidate) == parentName(classStatement))
            targets.insert(candidate);
    }
    for (const PStatement &candidate : parser->statementList().statements()) {
        if ((candidate->kind == StatementKind::Constructor
             || candidate->kind == StatementKind::Destructor)
                && targets.contains(candidate->parentScope.lock()))
            targets.insert(candidate);
    }
    return QVector<PStatement>::fromList(targets.values());
}

QVector<PStatement> CppRefactor::collectRenameTargets(
        const PCppParser &parser,
        const PStatement &statement,
        const std::optional<QVector<PStatement>> &selectedOverloads) const
{
    if (statement->kind == StatementKind::Class)
        return collectClassTargets(parser, statement);
    if (statement->kind == StatementKind::Constructor
            || statement->kind == StatementKind::Destructor) {
        return collectClassTargets(parser, statement->parentScope.lock());
    }
    if (!isFunction(statement))
        return {statement};

    QSet<PStatement> targets;
    QSet<PStatement> visited;
    QVector<PStatement> selected = selectedOverloads
            ? *selectedOverloads : overloadsOf(parser, statement);
    for (const PStatement &overload : selected)
        collectOverrides(parser, overload, targets, visited);
    return QVector<PStatement>::fromList(targets.values());
}

static RenameResult validateReferencesInEditor(
        Editor *editor,
        const SymbolReferenceFile &file)
{
    if (file.fileName != editor->filename()
            || file.documentVersion != editor->document()->contentVersion())
        return RenameResult::ContentChanged;
    for (const SymbolReference &reference : file.references) {
        if (reference.line < 0 || reference.line >= editor->lineCount()
                || reference.ch < 0 || reference.length < 0)
            return RenameResult::ContentChanged;
        QString lineText = editor->lineText(reference.line);
        if (reference.ch + reference.length > lineText.size()
                || reference.lineText.mid(reference.ch, reference.length)
                    != lineText.mid(reference.ch, reference.length))
            return RenameResult::ContentChanged;
    }
    return RenameResult::Success;
}

RenameResult CppRefactor::renameReferencesInEditor(
        Editor *editor,
        const SymbolReferenceFile &file,
        const QString &newWord)
{
    RenameResult validation = validateReferencesInEditor(editor, file);
    if (validation != RenameResult::Success)
        return validation;
    QVector<SymbolReference> references = file.references;
    std::sort(references.begin(), references.end(),
              [](const SymbolReference &left, const SymbolReference &right) {
        if (left.line != right.line)
            return left.line > right.line;
        return left.ch > right.ch;
    });
    editor->clearSelection();
    editor->beginEditing();
    auto endEditing = finally([&editor] {
        editor->setCaretPos(editor->ensureCharPosValid(editor->caretPos()));
        editor->endEditing();
    });
    for (const SymbolReference &reference : references) {
        QSynedit::DocumentMatch match{
            {reference.ch, reference.line}, reference.length,
            editor->document()->contentVersion()};
        if (!editor->replaceMatch(match, newWord))
            return RenameResult::ContentChanged;
    }
    return RenameResult::Success;
}

RenameResult CppRefactor::prepareRename(
        Editor *editor,
        const CharPos &pos,
        const QString &newWord,
        const std::optional<QVector<PStatement>> &selectedOverloads,
        SymbolReferenceResults &results)
{
    if (!editor || !validNewIdentifier(editor, newWord))
        return RenameResult::InvalidInput;
    if (!editor->parser())
        return RenameResult::ParserUnavailable;
    PStatement oldStatement;
    QVector<PStatement> targets;
    SymbolReferenceScope scope;
    {
        if (!editor->parser()->freeze())
            return RenameResult::ParserUnavailable;
        auto unfreeze = finally([&editor] {
            editor->parser()->unFreeze();
        });
        QChar current = editor->charAt(pos);
        CharPos expressionPos = pos;
        if (!editor->isIdentStartChar(current))
            expressionPos.ch--;
        if (expressionPos.ch < 0)
            return RenameResult::InvalidInput;
        QStringList expression = CppExpressionExtractor::extract(
                    editor, editor->syntaxer(), expressionPos);
        expression = completeCallExpression(editor, expression, expressionPos);
        if (expression.isEmpty())
            return RenameResult::InvalidInput;
        oldStatement = editor->parser()->findStatementOf(
                    editor->filename(), expression, pos.line);
        if (!oldStatement)
            return RenameResult::SymbolNotFound;

        targets = collectRenameTargets(
                    editor->parser(), oldStatement, selectedOverloads);
        if (targets.isEmpty())
            return RenameResult::NoSelection;
        scope = oldStatement->scope == StatementScope::Local
                || !editor->inProject() || !mProject
                ? SymbolReferenceScope::CurrentFile
                : SymbolReferenceScope::WholeProject;
        auto allowedFile = [&](const QString &fileName) {
            return scope == SymbolReferenceScope::CurrentFile
                    ? fileName == editor->filename()
                    : mProject->inProject(fileName);
        };
        for (const PStatement &statement : targets) {
            for (const StatementOccurrence &declaration : statement->declarations) {
                if (!allowedFile(declaration.fileName))
                    return RenameResult::OutsideAllowedScope;
            }
            if (statementHasDefinition(statement)
                    && !allowedFile(statement->definition.fileName))
                return RenameResult::OutsideAllowedScope;
        }
        if (hasRenameNameConflict(editor->parser(), targets, newWord))
            return RenameResult::NameConflict;
    }
    if (!doFindOccurrences(editor, oldStatement, targets, scope, results))
        return RenameResult::SymbolNotFound;
    if (results.files.isEmpty())
        return RenameResult::SymbolNotFound;
    for (const SymbolReferenceFile &file : results.files) {
        if (scope == SymbolReferenceScope::CurrentFile
                && file.fileName != editor->filename())
            return RenameResult::OutsideAllowedScope;
        if (scope == SymbolReferenceScope::WholeProject
                && !mProject->inProject(file.fileName))
            return RenameResult::OutsideAllowedScope;
    }
    return RenameResult::Success;
}

RenameResult CppRefactor::applyRename(
        Editor *editor,
        const SymbolReferenceResults &results,
        const QString &newWord)
{
    if (!editor || !validNewIdentifier(editor, newWord))
        return RenameResult::InvalidInput;
    if (!editor->parser() || !editor->parser()->freeze())
        return RenameResult::ParserUnavailable;
    auto unfreeze = finally([&editor] {
        editor->parser()->unFreeze();
    });
    struct PreparedFile {
        SymbolReferenceFile file;
        Editor *editor = nullptr;
        std::shared_ptr<Editor> ownedEditor;
        QByteArray originalBytes;
        bool existed = false;
        bool needsSave = false;
    };
    QVector<PreparedFile> prepared;
    for (const SymbolReferenceFile &file : results.files) {
        PreparedFile item;
        item.file = file;
        if (file.fileName == editor->filename()) {
            item.editor = editor;
        } else if (mEditorProvider) {
            item.editor = mEditorProvider(file.fileName);
        }
        if (!item.editor) {
            item.ownedEditor = std::make_shared<Editor>();
            item.editor = item.ownedEditor.get();
            item.editor->setEditorSettings(&pSettings->editor());
            FileType fileType = getFileType(file.fileName);
            if (!isC_CPPSourceFile(fileType))
                fileType = FileType::CCppHeader;
            item.editor->setFileType(fileType);
            item.editor->setFilename(file.fileName);
            item.editor->setSyntaxer(SyntaxerManager::getSyntaxer(
                        QSynedit::ProgrammingLanguage::CPP));
            item.existed = fileExists(file.fileName);
            if (item.existed) {
                QFile original(file.fileName);
                if (!original.open(QIODevice::ReadOnly))
                    return RenameResult::FileAccess;
                item.originalBytes = original.readAll();
            }
            QStringList buffer;
            if (mContentProvider && mContentProvider(file.fileName, buffer)) {
                item.editor->setEditorEncoding(file.encoding);
                item.editor->loadContent(buffer);
            } else {
                if (!item.existed)
                    return RenameResult::FileAccess;
                QByteArray realEncoding;
                try {
                    item.editor->loadFromFile(file.fileName, file.encoding, realEncoding);
                } catch (const FileError &) {
                    return RenameResult::FileAccess;
                }
            }
            item.file.documentVersion = item.editor->document()->contentVersion();
            item.needsSave = true;
        }
        RenameResult validation = validateReferencesInEditor(item.editor, item.file);
        if (validation != RenameResult::Success)
            return validation;
        prepared.append(item);
    }
    if (prepared.isEmpty())
        return RenameResult::SymbolNotFound;

    QVector<PreparedFile *> applied;
    QVector<PreparedFile *> saved;
    auto rollback = [&] {
        for (auto it = applied.rbegin(); it != applied.rend(); ++it) {
            if (!(*it)->ownedEditor)
                (*it)->editor->undo();
        }
        for (auto it = saved.rbegin(); it != saved.rend(); ++it) {
            if ((*it)->existed) {
                QFile output((*it)->file.fileName);
                if (output.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    output.write((*it)->originalBytes);
            } else {
                QFile::remove((*it)->file.fileName);
            }
        }
    };
    for (PreparedFile &item : prepared) {
        RenameResult result = renameReferencesInEditor(item.editor, item.file, newWord);
        if (result != RenameResult::Success) {
            rollback();
            return result;
        }
        applied.append(&item);
        if (item.needsSave) {
            saved.append(&item);
            QFile output(item.file.fileName);
            QByteArray realEncoding;
            try {
                item.editor->saveToFile(output, item.file.encoding, realEncoding);
            } catch (const FileError &) {
                rollback();
                return RenameResult::FileAccess;
            }
            output.close();
        }
    }
    return RenameResult::Success;
}

RenameResult CppRefactor::renameSymbol(
        Editor *editor,
        const CharPos &pos,
        const QString &newWord,
        const std::optional<QVector<PStatement>> &selectedOverloads)
{
    SymbolReferenceResults results;
    RenameResult result = prepareRename(
                editor, pos, newWord, selectedOverloads, results);
    if (result != RenameResult::Success)
        return result;
    return applyRename(editor, results, newWord);
}

RenameResult CppRefactor::renameUndefinedLocalVariable(
        Editor *editor,
        const CharPos &pos,
        const QString &newWord)
{
    if (!editor || !validNewIdentifier(editor, newWord))
        return RenameResult::InvalidInput;
    if (!editor->parser())
        return RenameResult::ParserUnavailable;
    if (!editor->parser()->freeze())
        return RenameResult::ParserUnavailable;
    auto unfreeze = finally([&editor] {
        editor->parser()->unFreeze();
    });
    PStatement scope = editor->parser()->findScopeStatement(
                editor->filename(), pos.line);
    if (!scope || (scope->kind != StatementKind::Function
                   && scope->kind != StatementKind::Constructor
                   && scope->kind != StatementKind::Destructor
                   && scope->kind != StatementKind::Lambda))
        return RenameResult::NotInScope;
    StatementOccurrence scopeDefinition = statementDefinitionForNavigation(scope);
    if (scopeDefinition.fileName != editor->filename())
        return RenameResult::OutsideAllowedScope;

    CharPos expressionPos = pos;
    if (!editor->isIdentStartChar(editor->charAt(pos)))
        expressionPos.ch--;
    if (expressionPos.ch < 0)
        return RenameResult::InvalidInput;
    QStringList expression = CppExpressionExtractor::extract(
                editor, editor->syntaxer(), expressionPos);
    if (expression.size() != 1)
        return RenameResult::InvalidInput;
    QString oldVarName = expression.front();
    if (editor->parser()->findStatementOf(
                editor->filename(), expression, pos.line))
        return RenameResult::SymbolNotFound;
    if (editor->parser()->findStatementOf(
                editor->filename(), {newWord}, pos.line))
        return RenameResult::NameConflict;

    QSynedit::PSyntaxer syntaxer = SyntaxerManager::getSyntaxer(
                QSynedit::ProgrammingLanguage::CPP);
    editor->clearSelection();
    editor->beginEditing();
    for (int line = scopeDefinition.range.start.line;
         line < editor->lineCount(); ++line) {
        bool inScope = false;
        PStatement currentScope = editor->parser()->findScopeStatement(
                    editor->filename(), line);
        while (currentScope) {
            if (currentScope == scope) {
                inScope = true;
                break;
            }
            currentScope = currentScope->parentScope.lock();
        }
        if (!inScope)
            break;
        QString oldLine = editor->lineText(line);
        QString newLine;
        editor->startParseLine(syntaxer.get(), line);
        while (!syntaxer->eol()) {
            QString token = syntaxer->getToken();
            if (token == oldVarName) {
                CharPos tokenPos{syntaxer->getTokenPos(), line};
                QStringList tokenExpression = CppExpressionExtractor::extract(
                            editor, editor->syntaxer(), tokenPos);
                if (tokenExpression.size() == 1
                        && !editor->parser()->findStatementOf(
                            editor->filename(), tokenExpression, line))
                    token = newWord;
            }
            newLine += token;
            syntaxer->next();
        }
        if (newLine != oldLine)
            editor->replaceLine(line, newLine);
    }
    editor->setCaretPos(editor->ensureCharPosValid(editor->caretPos()));
    editor->endEditing();
    return RenameResult::Success;
}
