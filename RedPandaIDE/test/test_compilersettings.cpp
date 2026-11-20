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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "test_compilersettings.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QTest>

#include "src/settings/compilersetsettings.h"

void TestCompilerSettings::test_default_library_dirs_exclude_custom_dirs()
{
    CompilerSet set;
    set.libDirs().append("custom-library-dir");
    QVERIFY(!set.defaultLibDirs().contains("custom-library-dir"));
}

#ifdef REDPANDA_PARSER_STDLIB_TEST
void TestCompilerSettings::test_compiler_updates_at_startup()
{
    QString source = QStringLiteral(REDPANDA_PARSER_STDLIB_MINGW_ROOT) + "/bin/gcc.exe";
    if (!QFileInfo::exists(source))
        QSKIP("The configured GCC compiler is unavailable");
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    QVERIFY(QDir(temporaryDir.path()).mkdir("bin"));
    QVERIFY(QDir(temporaryDir.path()).mkdir("custom-include"));
    QString compilerCopy = temporaryDir.filePath("bin/gcc.exe");
    QString customInclude = temporaryDir.filePath("custom-include");
    QVERIFY(QFile::copy(source, compilerCopy));
    CompilerSet reference(temporaryDir.filePath("bin"), "gcc.exe");
    QVERIFY(!reference.version().isEmpty());

    {
        SettingsPersistor persistor(temporaryDir.filePath("compiler.ini"));
        DirSettings dirs(&persistor);
        CompilerSets sets(&persistor, &dirs);
        PCompilerSet set = sets.addSet();
        set->setCCompiler(compilerCopy);
        set->setCompilerType(CompilerType::Unknown);
        set->setVersion("outdated");
        set->setName("Custom compiler");
        set->setUseCustomCompileParams(true);
        set->setCustomCompileParams("-DCUSTOM_SETTING");
        set->binDirs().append(temporaryDir.filePath("bin"));
        set->CIncludeDirs().append(customInclude);
        sets.setDefaultIndex(0);
        sets.saveSets();
        persistor.beginGroup("CompilerSet_0");
        persistor.saveValue("CompilerSize", 0);
        persistor.saveValue("CompilerMTime", 0);
        persistor.endGroup();
    }

    {
        SettingsPersistor persistor(temporaryDir.filePath("compiler.ini"));
        DirSettings dirs(&persistor);
        CompilerSets sets(&persistor, &dirs);
        QSignalSpy changeSpy(&sets, &CompilerSets::compilerSetChanged);
        sets.loadSets();
        QCOMPARE(sets.size(), 1);
        PCompilerSet set = sets.getSet(0);
        QVERIFY(set);
        QCOMPARE(set->compilerType(), CompilerType::GCC);
        QCOMPARE(set->version(), reference.version());
        QCOMPARE(set->name(), "Custom compiler");
        QVERIFY(set->useCustomCompileParams());
        QCOMPARE(set->customCompileParams(), "-DCUSTOM_SETTING");
        QVERIFY(set->CIncludeDirs().contains(customInclude));
        QCOMPARE(changeSpy.count(), 1);

        set->setVersion("outdated-again");
        sets.refreshSet(0);
        QCOMPARE(set->version(), reference.version());
        QCOMPARE(changeSpy.count(), 2);
    }
}

void TestCompilerSettings::test_replaced_compiler_directory_is_refreshed()
{
    QString root = qEnvironmentVariable("REDPANDA_TEST_CLANG_ROOT");
    QString clangPath = root + "/bin/clang.exe";
    if (root.isEmpty() || !QFileInfo::exists(clangPath))
        QSKIP("Set REDPANDA_TEST_CLANG_ROOT to run the Clang replacement test");
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    QString binDir = root + "/bin";
    CompilerSet reference(binDir, "clang.exe");
    QVERIFY(!reference.version().isEmpty());

    SettingsPersistor persistor(temporaryDir.filePath("compiler.ini"));
    DirSettings dirs(&persistor);
    CompilerSets sets(&persistor, &dirs);
    PCompilerSet set = sets.addSet();
    set->setCCompiler(binDir + "/missing-gcc.exe");
    set->setCompilerType(CompilerType::GCC);
    set->setVersion("old-gcc-version");
    set->binDirs().append(binDir);
    sets.setDefaultIndex(0);
    sets.saveSets();

    QSignalSpy changeSpy(&sets, &CompilerSets::compilerSetChanged);
    sets.loadSets();
    QCOMPARE(sets.size(), 1);
    set = sets.getSet(0);
    QVERIFY(set);
    QString expectedCompiler = QFileInfo(binDir + "/gcc.exe").isFile()
            ? binDir + "/gcc.exe" : clangPath;
    QCOMPARE(set->CCompiler(), expectedCompiler);
    QCOMPARE(set->compilerType(), CompilerType::Clang);
    QCOMPARE(set->version(), reference.version());
    QVERIFY(set->name().contains("Clang"));
    QVERIFY(set->name().contains(reference.version()));
    QCOMPARE(changeSpy.count(), 1);
}
#endif
