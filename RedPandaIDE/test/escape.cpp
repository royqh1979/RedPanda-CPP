#include <cstdlib>

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QString>

#include "utils/escape.h"

int testIndex = 0;

QByteArray content = "main(){}";

void testMake(QString name)
{
    ++testIndex;
    auto dir = QString("test-escape-%1").arg(testIndex);
    auto srcName = name + ".c";
    auto objName = name + ".o";
#ifdef Q_OS_WIN
    auto binName = name + ".exe";
#else
    auto binName = name;
#endif 
    auto includeMfName = name + ".mf";

    auto fail = [&name](const QString& msg) {
        qDebug() << "Error in test" << testIndex << name << ":" << msg;
        exit(1);
    };

    // create directory
    {
        auto cwd = QDir();
        if (cwd.exists(dir)) {
            if (!QDir(dir).removeRecursively())
                fail("cannot remove directory");
        }
        if (!cwd.mkdir(dir))
            fail("cannot create directory");
    }

    // create source file
    {
        auto srcPath = QString("%1/%2").arg(dir, srcName);
        QFile f(srcPath);
        if (!f.open(QIODevice::WriteOnly))
            fail("cannot create source file");
        if (f.write(content) != content.size())
            fail("cannot write to source file");
        f.close();
    }

    // create included makefile
    {
        auto file = QString("%1/%2").arg(dir, includeMfName);
        QFile f(file);
        if (!f.open(QIODevice::WriteOnly))
            fail("cannot create included makefile");
        f.close();
    }

    // create makefile
    {
        QStringList mf;
        mf << "BIN_DEP = " + escapeFilenameForMakefilePrerequisite(binName);
        mf << "BIN_TAR = " + escapeFilenameForMakefileTarget(binName);
        mf << "BIN_ARG = " + escapeArgumentForMakefileVariableValue(binName, false);
        mf << "OBJS_DEP = " + escapeFilenameForMakefilePrerequisite(objName);
        mf << "OBJS_ARG = " + escapeArgumentForMakefileVariableValue(objName, false);
        mf << "include " + escapeFilenameForMakefileInclude(includeMfName);
        mf << ".PHONY: all clean";
        mf << "all: $(BIN_DEP)";
        mf << "$(BIN_TAR): $(OBJS_DEP)";
        mf << "\tgcc -o $(BIN_ARG) $(OBJS_ARG)";
        mf << escapeFilenameForMakefileTarget(objName) + ": " + escapeFilenameForMakefilePrerequisite(srcName);
        mf << "\tgcc -o " + escapeArgumentForMakefileRecipe(objName, false) + " -c " + escapeArgumentForMakefileRecipe(srcName, false);
        mf << "clean:";
        mf << "\trm -f $(BIN_ARG) $(OBJS_ARG)";

        auto file = QString("%1/makefile").arg(dir);
        QFile f(file);
        if (!f.open(QIODevice::WriteOnly))
            fail("cannot create makefile");
        f.write(mf.join("\n").toUtf8());
    }

    // run make
    {
        QProcess p;
        p.setWorkingDirectory(dir);
#ifdef Q_OS_WIN
        p.setProgram("mingw32-make.exe");
#else
        p.setProgram("make");
#endif
        p.start();
        p.waitForFinished();
        if (p.exitCode() != 0) {
            qDebug() << "Error in test" << testIndex << name << ": make failed";
            exit(1);
        }
        auto binFile = QString("%1/%2").arg(dir, binName);
        if (!QFile(binFile).exists()) {
            qDebug() << "Error in test" << testIndex << name << ": executable not properly created";
            exit(1);
        }
    }
}

int main()
{
    testMake("simple");
    testMake("dollar$dollar");
    testMake("paren(paren");
    testMake("paren)paren");
    testMake("pair(of)paren");
    testMake("bracket[bracket");
    testMake("bracket]bracket");
    testMake("pair[of]brackets");
    testMake("brace{brace");
    testMake("brace}brace");
    testMake("pair{of}braces");
    testMake("hash#hash");
    testMake("percent%percent");
    testMake("ampersand&ampersand");
    testMake("space space");
    testMake("quote'quote");
    testMake("complex$(complex)complex");
    testMake("complex${complex}complex");

#ifndef Q_OS_WIN
    testMake("colon:colon");
    testMake("less<less");
    testMake("greater>greater");
    testMake("pair<of>angle");
    testMake("asterisk*asterisk");
    testMake("question?question");
    testMake("escape\033escape");
    testMake(R"(quote"quote)");
    testMake(R"(backslash\backslash)");
    testMake(R"(complex\#complex)");
    testMake(R"(complex\ complex)");
    testMake(R"(weird\)");
    testMake(R"(weird\\)");
    testMake(R"(weird\\\)");
    testMake(R"(weird\\\\)");
#endif

    // seems impossible:
    // testMake("tab\ttab");
    // testMake("newline\nnewline");

    return 0;
}
