#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <algorithm>
#include "test_cppparserpublicapibaseline.h"
#ifdef REDPANDA_PARSER_PUBLIC_API_BASELINE_TEST
#include "test_parser_utils.h"

namespace {
struct ApiEntry {
    QString kind;
    QString name;
};

QString resourcePath(const QString &filename)
{
    return QFileInfo("resources/stdlib/" + filename).absoluteFilePath();
}

QString normalizedStatementName(QString name)
{
    name.remove("::__cxx11");
    name.remove("::__1");
    name.remove("::__fs");
    if (name.contains("operator"))
        name.replace("filesystem::path", "path");
    QStringList result;
    const QStringList parts = splitQualifiedName(name);
    for (const QString &part : parts) {
        QString normalized = part;
        int templateStart = normalized.indexOf('<');
        if (templateStart >= 0 && normalized.endsWith('>')
                && (!part.startsWith("operator") || part.contains(' '))) {
            normalized.truncate(templateStart);
        }
        result.append(normalized);
    }
    return result.join("::");
}

bool findNormalizedStatement(const PCppParser &parser, const QString &name)
{
    if (parser->findStatement(name))
        return true;
    const QString normalized = normalizedStatementName(name);
    const StatementList statements = parser->statementList().statements();
    for (const PStatement &statement : statements) {
        if (normalizedStatementName(statement->fullName) == normalized)
            return true;
        auto normal = std::dynamic_pointer_cast<Statement>(statement);
        if (!normal || !normal->declSpecifierProperties.testFlag(DeclSpecifierProperty::Friend))
            continue;
        int expectedScopeEnd = name.lastIndexOf("::");
        int statementScopeEnd = statement->fullName.lastIndexOf("::");
        if (expectedScopeEnd < 0 || statementScopeEnd < 0
                || name.mid(expectedScopeEnd) != statement->fullName.mid(statementScopeEnd)) {
            continue;
        }
        PStatement parent = statement->parentScope.lock();
        PStatement nameSpace = parent ? parent->parentScope.lock() : PStatement();
        if (nameSpace && normalizedStatementName(nameSpace->fullName)
                == normalizedStatementName(name.left(expectedScopeEnd))) {
            return true;
        }
    }
    return false;
}

PStatement findScopeStatement(const PCppParser &parser, const QString &name)
{
    if (PStatement result = parser->findStatement(name))
        return result;
    const QString normalized = normalizedStatementName(name);
    const StatementList statements = parser->statementList().statements();
    for (const PStatement &statement : statements) {
        if (normalizedStatementName(statement->fullName) == normalized)
            return statement;
    }
    return {};
}

bool statementExists(const PCppParser &parser, const QString &name)
{
    if (findNormalizedStatement(parser, name))
        return true;
    static const QHash<QString, QStringList> conversionAliases{
        {"std::atomic::operator T", {"std::atomic::operator _Tp"}},
        {"std::reference_wrapper::operator T&",
         {"std::reference_wrapper::operator _Tp&", "std::reference_wrapper::operator type&"}},
        {"std::basic_string::operator basic_string_view",
         {"std::basic_string::operator __sv_type", "std::basic_string::operator __self_view",
          "std::basic_string::operator basic_string_view"}},
        {"std::filesystem::directory_entry::operator const path&",
         {"std::filesystem::directory_entry::operator const _Path&"}}
    };
    auto conversion = conversionAliases.constFind(name);
    if (conversion != conversionAliases.constEnd()) {
        for (const QString &alias : conversion.value()) {
            if (findNormalizedStatement(parser, alias))
                return true;
        }
    }
    int scopeEnd = name.lastIndexOf("::");
    if (scopeEnd <= 0)
        return false;
    PStatement scope = findScopeStatement(parser, name.left(scopeEnd));
    if (!scope)
        return false;
    for (int depth = 0; depth < 8; ++depth) {
        if (scope->kind != StatementKind::Typedef && scope->kind != StatementKind::Alias
                && scope->kind != StatementKind::InlineAlias) {
            break;
        }
        PStatement target = parser->findAliasedStatement(scope);
        if (!target || target == scope)
            break;
        scope = target;
    }
    if (scope->kind != StatementKind::Class && scope->kind != StatementKind::Lambda
            && scope->kind != StatementKind::EnumType
            && scope->kind != StatementKind::EnumClassType) {
        return false;
    }
    const QString memberName = name.mid(scopeEnd + 2);
    if (std::dynamic_pointer_cast<Statement>(scope)->children.value(memberName).lock())
        return true;
    const QList<PStatement> members = parser->resolveMembersOfClass(scope, {});
    for (const PStatement &member : members) {
        if (member->command == memberName)
            return true;
    }
    return false;
}

bool isImplementationGap(const QString &name, CppCompilerType compilerType)
{
    static const QSet<QString> common{
        "aligned_alloc", "std::aligned_alloc", "CMPLX", "CMPLXF", "CMPLXL",
        "MATH_ERREXCEPT", "MATH_ERRNO", "math_errhandling",
        "std::timespec", "std::timespec_get",
        "std::yocto", "std::yotta", "std::zepto", "std::zetta"
    };
    if (common.contains(name))
        return true;
    if (compilerType != CppCompilerType::Clang)
        return false;
    static const QSet<QString> clangOnly{
        "std::beta", "std::betaf", "std::betal",
        "std::declare_no_pointers", "std::declare_reachable", "std::get_pointer_safety",
        "std::pointer_safety", "std::pointer_safety::preferred", "std::pointer_safety::relaxed",
        "std::pointer_safety::strict", "std::undeclare_no_pointers", "std::undeclare_reachable",
        "std::shared_mutex::native_handle", "std::timed_mutex::native_handle",
        "std::recursive_timed_mutex::native_handle", "std::is_execution_policy"
    };
    static const QStringList specialMathPrefixes{
        "std::assoc_", "std::comp_ellint_", "std::cyl_", "std::ellint_", "std::expint",
        "std::laguerre", "std::legendre", "std::riemann_zeta", "std::sph_", "std::execution::"
    };
    if (clangOnly.contains(name))
        return true;
    for (const QString &prefix : specialMathPrefixes) {
        if (name.startsWith(prefix))
            return true;
    }
    return false;
}

bool isImplicitSpecialAssignment(const QString &name)
{
    static const QSet<QString> names{
        "std::monostate::operator=", "std::queue::operator=", "std::stack::operator=",
        "std::chrono::duration::operator=", "std::regex_iterator::operator=",
        "std::scoped_allocator_adaptor::operator=", "std::reference_wrapper::operator="
    };
    return names.contains(name);
}

void reportMissing(const QString &phase, const QStringList &missing)
{
    qWarning() << phase << "missing count" << missing.count();
    const int reportCount = missing.count();
    for (int i = 0; i < reportCount; ++i)
        qWarning().noquote() << phase << "missing" << missing[i];
}
}

void TestCppParserPublicApiBaseline::verifyAndProfile(const QString &stem)
{
    QFile headerFile(resourcePath(stem + "-headers.txt"));
    QVERIFY(headerFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QStringList includes;
    const QList<QByteArray> headerLines = headerFile.readAll().split('\n');
    for (const QByteArray &line : headerLines) {
        if (!line.trimmed().isEmpty())
            includes.append("#include <" + line.trimmed() + ">");
    }
    QVERIFY(!includes.isEmpty());

    QFile entryFile(resourcePath(stem + ".tsv"));
    QVERIFY(entryFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QVector<ApiEntry> entries;
    const QList<QByteArray> entryLines = entryFile.readAll().split('\n');
    for (const QByteArray &line : entryLines) {
        if (line.trimmed().isEmpty())
            continue;
        int separator = line.indexOf('\t');
        QVERIFY(separator > 0);
        entries.append({QString::fromUtf8(line.left(separator)),
                        QString::fromUtf8(line.mid(separator + 1))});
    }
    QVERIFY(entries.count() > 100);

    bool isC = stem == "public-api-c11";
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    QString main = temporaryDir.filePath(isC ? "main.c" : "main.cpp");
    QVERIFY(writeLines(main, includes));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(
                mParser, bitsStdcpp, isC ? ParserLanguage::C : ParserLanguage::CPlusPlus));
    CppParser::parseFileBlocking(mParser, main, "");

    QStringList lookupMissing;
    QStringList implementationGaps;
    QStringList implicitSpecialMembers;
    for (const ApiEntry &entry : entries) {
        if (statementExists(mParser, entry.name))
            continue;
        if (isImplementationGap(entry.name, mParser->compilerType())) {
            implementationGaps.append(entry.kind + "\t" + entry.name);
        } else if (entry.kind == "constructor" || entry.kind == "destructor"
                   || isImplicitSpecialAssignment(entry.name)) {
            implicitSpecialMembers.append(entry.kind + "\t" + entry.name);
        } else {
            lookupMissing.append(entry.kind + "\t" + entry.name);
        }
    }
    reportMissing(stem + " implementation gap", implementationGaps);
    reportMissing(stem + " implicit special member", implicitSpecialMembers);
    reportMissing(stem + " lookup", lookupMissing);

    QStringList evalMissing;
    QVector<QPair<qint64, QString>> timings;
    timings.reserve(entries.count());
    QElapsedTimer evalTimer;
    evalTimer.start();
    for (const ApiEntry &entry : entries) {
        QStringList expression;
        const QStringList parts = splitQualifiedName(entry.name);
        for (const QString &part : parts) {
            if (!expression.isEmpty())
                expression.append("::");
            expression.append(part);
        }
        QElapsedTimer timer;
        timer.start();
        EvalResult result = mParser->evalExpression(main, expression, {});
        qint64 elapsed = timer.nsecsElapsed();
        timings.append({elapsed, entry.name});
        if (!result.isValid())
            evalMissing.append(entry.kind + "\t" + entry.name);
    }
    qint64 totalNs = evalTimer.nsecsElapsed();
    std::sort(timings.begin(), timings.end(), [](const auto &left, const auto &right) {
        return left.first > right.first;
    });
    qWarning() << "PUBLIC_API_EVAL_PROFILE" << stem
               << "entries" << entries.count()
               << "succeeded" << entries.count() - evalMissing.count()
               << "failed" << evalMissing.count()
               << "totalMs" << totalNs / 1000000.0
               << "averageUs" << totalNs / 1000.0 / entries.count()
               << "maxMs" << timings.first().first / 1000000.0;
    for (int i = 0; i < qMin(10, timings.count()); ++i)
        qWarning() << "PUBLIC_API_EVAL_SLOWEST" << stem << timings[i].second << timings[i].first;
    reportMissing(stem + " eval", evalMissing);

    QCOMPARE(lookupMissing.count(), 0);
}

void TestCppParserPublicApiBaseline::test_cpp17_public_api_validation_and_eval_profile()
{
    verifyAndProfile("public-api-cpp17");
}

void TestCppParserPublicApiBaseline::test_c11_public_api_validation_and_eval_profile()
{
    verifyAndProfile("public-api-c11");
}
#endif
