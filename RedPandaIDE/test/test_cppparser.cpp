#include <QTest>
#include "src/parser/cppparser.h"
#include "test_cppparser.h"

TestCppParser::TestCppParser(QObject *parent):
    QObject{parent}
{
    init_parser();
}

void TestCppParser::init_parser()
{
    mParser = std::make_shared<CppParser>();
}

void TestCppParser::test_parse_var()
{
    mParser->setOnGetFileStream([](const QString& filename, QStringList& buffer){
        buffer=QStringList({
                               "int xxx=10;"
                           });
           return true;
                                });
    CppParser::parseFileBlocking(mParser,"text.cpp",false,"");
    QCOMPARE(mParser->statementList().count(),1);
    PStatement statement = mParser->findStatement("xxx");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"xxx");
    QCOMPARE(statement->kind,StatementKind::Variable);

}

void TestCppParser::test_parse_vars()
{
    mParser->setOnGetFileStream(nullptr);
    QFileInfo file("resources/vars.cpp");
    QVERIFY(file.exists());
    CppParser::parseFileBlocking(mParser,file.absoluteFilePath(),false,"");
    PStatement statement;
    statement = mParser->findStatement("a1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"const int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"extern const int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int");
    QCOMPARE(statement->args,"[4]");

    statement = mParser->findStatement("a6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int *");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int &");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int **");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int **");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a9");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a9");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a10");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a10");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int *");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a11");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a11");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int &");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"unsigned int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"const unsigned int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const unsigned int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"extern const unsigned int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static unsigned int");
    QCOMPARE(statement->args,"[4]");

    statement = mParser->findStatement("b6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static unsigned int *");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static unsigned int *const");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static unsigned int **");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"std::string");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"const std::string");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const std::string");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const std::string");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const std::string");
    QCOMPARE(statement->args,"[4]");

    statement = mParser->findStatement("s6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const std::string *");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const std::string &");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("f1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"f1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const struct FILE *");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("f2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"f2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const struct FILE");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("ff1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statement->type,"int");
    QCOMPARE(statement->args,"( )");

    statement = mParser->findStatement("ff2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statement->type,"static const int");
    QCOMPARE(statement->args,"( )");

    statement = mParser->findStatement("ff3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statement->type,"static const std::string");
    QCOMPARE(statement->args,"( )");

    //TODO:: ff4 ff5

    statement = mParser->findStatement("ff6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statement->type,"int *");
    QCOMPARE(statement->args,"( )");

    statement = mParser->findStatement("aa1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"aa1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(!(statement->properties & StatementProperty::FunctionPointer));
    QCOMPARE(statement->type,"int (*)");
    QCOMPARE(statement->args,"[5]");

    statement = mParser->findStatement("aa2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"aa2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(!(statement->properties & StatementProperty::FunctionPointer));
    QCOMPARE(statement->type,"static const std::string (*)");
    QCOMPARE(statement->args,"[5]");

}

void TestCppParser::test_struct()
{
    mParser->setOnGetFileStream(nullptr);
    QFileInfo file("resources/struct.cpp");
    QVERIFY(file.exists());
    CppParser::parseFileBlocking(mParser,file.absoluteFilePath(),false,"");
    PStatement statement;
    statement = mParser->findStatement("TTT");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->command, "TTT");
    QCOMPARE(statement->kind, StatementKind::Class);
    QCOMPARE(statement->publicProperties.count(), 3);
    PStatement child;
    child = statement->publicProperties[0];
    QCOMPARE(child->command,"xxx");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::xxx");
    QCOMPARE(child->type, "int");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);
    child = statement->publicProperties[1];
    QCOMPARE(child->command,"yyy");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::yyy");
    QCOMPARE(child->type, "short");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);
    child = statement->publicProperties[2];
    QCOMPARE(child->command,"zzz");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::zzz");
    QCOMPARE(child->type, "double");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);

    QCOMPARE(statement->children.count(),4);
    child = statement->children.value("ppp");
    QCOMPARE(child->command,"ppp");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::ppp");
    QCOMPARE(child->type, "int");
    QCOMPARE(child->accessibility, StatementAccessibility::Private);

}

void TestCppParser::test_structured_bindings()
{
    mParser->setOnGetFileStream(nullptr);
    QFileInfo file("resources/structured-bindings.cpp");
    QVERIFY(file.exists());
    CppParser::parseFileBlocking(mParser,file.absoluteFilePath(),false,"");
    PStatement statement;
    statement = mParser->findStatement("a");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"short");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("c");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"c");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"double");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("d");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"d");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("e");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"e");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"short");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("g");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"g");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"double");
    QCOMPARE(statement->args,"");
}
