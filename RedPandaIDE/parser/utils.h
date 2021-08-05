#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H
#include <QObject>
#include <memory>
// preprocess/ macro define
struct Define {
    QString Name;
    QString args;
    QString value;
    QString filename;
    bool isMultiLine; // if true the expanded macro will span multiline
    bool hardCoded;// if true, don't free memory (points to hard defines)
    QStringList argList; // args list to format values
    QString formatValue; // format template to format values
};

using PDefine = std::shared_ptr<Define>;

enum class SkipType {
    skItself,  // skip itself
    skToSemicolon, // skip to ;
    skToColon, // skip to :
    skToRightParenthesis, // skip to )
    skToLeftBrace,// Skip to {
    skToRightBrace, // skip to }
    skNone // It's a keyword but don't process here
};

enum class StatementKind  {
  skUnknown,
  skPreprocessor,
  skEnumType,
  skEnum,
  skTypedef,
  skClass,
  skFunction,
  skConstructor,
  skDestructor,
  skVariable,
  skParameter,
  skNamespace,
  skNamespaceAlias,
  skBlock,
  skUserCodeIn,  // user code template
  skKeyword, // keywords
  skGlobalVariable,
  skLocalVariable,
  skAlias
};

#endif // PARSER_UTILS_H
