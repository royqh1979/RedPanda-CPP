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
        QObject *parent)
    : QObject(parent),
      mProject(std::move(project)),
      mContentProvider(std::move(contentProvider))
{
}

static bool validNewIdentifier(Editor *editor, const QString &newWord)
{
    if (!editor || newWord.isEmpty() || !editor->isIdentStartChar(newWord.front()))
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

bool CppRefactor::findOccurrences(
        Editor *editor, const CharPos &pos, SymbolReferenceResults &results)
{
    if (!editor || !editor->parser())
        return false;
    if (!editor->parser()->freeze())
        return false;
    auto unfreeze = finally([&editor] {
        editor->parser()->unFreeze();
    });
    QStringList expression = CppExpressionExtractor::extract(
                editor, editor->syntaxer(), pos);
    if (expression.isEmpty())
        return false;
    PStatement statement = editor->parser()->findStatementOf(
                editor->filename(), expression, pos.line);
    if (!statement)
        return false;
    SymbolReferenceScope scope = statement->scope == StatementScope::Local
            || !editor->inProject() || !mProject
            ? SymbolReferenceScope::CurrentFile
            : SymbolReferenceScope::WholeProject;
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
    if (!editor->parser()->freeze())
        return false;
    auto unfreeze = finally([&editor] {
        editor->parser()->unFreeze();
    });
    PStatement statement;
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

    auto addFile = [&](const QString &fileName, const QByteArray &encoding) {
        SymbolReferenceFile file = findOccurrencesInFile(
                    fileName, encoding, targets, editor->parser());
        if (!file.references.isEmpty())
            results.files.append(file);
    };
    if (scope == SymbolReferenceScope::CurrentFile) {
        SymbolReferenceFile file = findOccurrencesInFile(
                    editor->filename(), editor->editorEncoding(),
                    targets, editor->parser(), editor->content());
        file.documentVersion = editor->document()->contentVersion();
        if (!file.references.isEmpty())
            results.files.append(file);
    } else if (mProject) {
        for (const PProjectUnit &unit : mProject->unitList()) {
            QByteArray encoding = unit->encoding() == ENCODING_PROJECT
                    ? mProject->options().encoding : unit->encoding();
            addFile(unit->fileName(), encoding);
        }
    }
    return true;
}

SymbolReferenceFile CppRefactor::findOccurrencesInFile(
        const QString &fileName,
        const QByteArray &fileEncoding,
        const QVector<PStatement> &targets,
        const PCppParser &parser,
        const QStringList &openContent)
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
                if (next < lineText.size() && lineText.at(next) == '(') {
                    CppTokenizer tokenizer;
                    tokenizer.tokenize({lineText.mid(start)});
                    QStringList callExpression = expression;
                    int depth = 0;
                    bool complete = false;
                    for (int i = 1; i < tokenizer.tokens().size(); ++i) {
                        QString text = tokenizer.tokens().at(i).text;
                        callExpression.append(text);
                        if (text == "(") {
                            ++depth;
                        } else if (text == ")" && --depth == 0) {
                            complete = true;
                            break;
                        }
                    }
                    expression = complete ? callExpression : QStringList();
                }
                PStatement tokenStatement = parser->findStatementOf(
                            fileName, expression, line);
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

static bool sameSignature(
        const PStatement &left,
        const PStatement &right)
{
    if (!isFunction(left) || !isFunction(right)
            || left->command != right->command)
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

RenameResult CppRefactor::renameReferencesInEditor(
        Editor *editor,
        const SymbolReferenceFile &file,
        const QString &newWord)
{
    if (file.fileName != editor->filename()
            || file.documentVersion != editor->document()->contentVersion())
        return RenameResult::ContentChanged;
    QVector<SymbolReference> references = file.references;
    for (const SymbolReference &reference : references) {
        if (reference.line < 0 || reference.line >= editor->lineCount()
                || reference.ch < 0 || reference.length < 0)
            return RenameResult::ContentChanged;
        QString lineText = editor->lineText(reference.line);
        if (reference.ch + reference.length > lineText.size()
                || reference.lineText.mid(reference.ch, reference.length)
                    != lineText.mid(reference.ch, reference.length))
            return RenameResult::ContentChanged;
    }

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
    if (expression.isEmpty())
        return RenameResult::InvalidInput;
    PStatement oldStatement = editor->parser()->findStatementOf(
                editor->filename(), expression, pos.line);
    if (!oldStatement)
        return RenameResult::SymbolNotFound;

    QStringList newExpression = expression;
    newExpression.last() = newWord;
    PStatement newStatement = editor->parser()->findStatementOf(
                editor->filename(), newExpression, pos.line);
    if (newStatement && parentName(newStatement) == parentName(oldStatement))
        return RenameResult::NameConflict;

    QVector<PStatement> targets = collectRenameTargets(
                editor->parser(), oldStatement, selectedOverloads);
    if (targets.isEmpty())
        return RenameResult::NoSelection;
    for (const PStatement &statement : targets) {
        for (const StatementOccurrence &declaration : statement->declarations) {
            if (declaration.fileName != editor->filename())
                return RenameResult::NotInCurrentFile;
        }
        if (statementHasDefinition(statement)
                && statement->definition.fileName != editor->filename())
            return RenameResult::NotInCurrentFile;
    }

    SymbolReferenceScope scope = oldStatement->scope == StatementScope::Local
            || !editor->inProject() || !mProject
            ? SymbolReferenceScope::CurrentFile
            : SymbolReferenceScope::WholeProject;
    if (!doFindOccurrences(editor, oldStatement, targets, scope, results))
        return RenameResult::SymbolNotFound;
    bool found = false;
    for (const SymbolReferenceFile &file : results.files) {
        if (file.fileName != editor->filename())
            return RenameResult::NotInCurrentFile;
        found = true;
    }
    return found ? RenameResult::Success : RenameResult::SymbolNotFound;
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
    if (results.files.size() != 1
            || results.files.constFirst().fileName != editor->filename())
        return RenameResult::NotInCurrentFile;
    return renameReferencesInEditor(editor, results.files.constFirst(), newWord);
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
        return RenameResult::NotInCurrentFile;

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
