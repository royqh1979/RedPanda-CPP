#ifndef GDBMIRESULTPARSER_H
#define GDBMIRESULTPARSER_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <memory>


enum class GDBMIResultType {
    Breakpoint,
    BreakpointTable,
    FrameStack,
    LocalVariables,
    Locals,
    Frame,
    Disassembly,
    Evaluation,
    RegisterNames,
    RegisterValues,
    Memory,
    VariableInfo,

};


class GDBMIResultParser
{
public:
    enum class ParseValueType {
        Value,
        Object,
        Array,
        NotAssigned
    };

    class ParseValue;

    class ParseObject {
    public:
        explicit ParseObject();
        ParseObject(const ParseObject& object);
        ParseValue operator[](const QByteArray& name) const;
        ParseValue& operator[](const QByteArray& name);
        ParseObject& operator=(const ParseObject& object);
    private:
        QHash<QByteArray, ParseValue> mProps;
    };

    class ParseValue {
    public:
        explicit ParseValue();
        explicit ParseValue(const QByteArray& value);
        explicit ParseValue(const ParseObject &object);
        explicit ParseValue(const QList<ParseValue>& array);
        ParseValue(const ParseValue& value);
        const QByteArray &value() const;
        const QList<ParseValue> &array() const;
        const ParseObject &object() const;
        int intValue(int defaultValue=-1) const;
        int hexValue(int defaultValue=-1) const;

        QString pathValue() const;
        ParseValueType type() const;
        bool isValid() const;
        ParseValue& operator=(const QByteArray& value);
        ParseValue& operator=(const ParseObject& object);
        ParseValue& operator=(const QList<ParseValue>& array);
        ParseValue& operator=(const ParseValue& value);
    private:
        QByteArray mValue;
        QList<ParseValue> mArray;
        ParseObject mObject;
        ParseValueType mType;
    };

    using PParseValue = std::shared_ptr<ParseValue>;

public:
    GDBMIResultParser();
    bool parse(const QByteArray& record, const QString& command, GDBMIResultType& type, ParseObject& multiValues);
    bool parseAsyncResult(const QByteArray& record, QByteArray& result, ParseObject& multiValue);
private:
    bool parseMultiValues(const char*p, ParseObject& multiValue);
    bool parseNameAndValue(const char *&p,QByteArray& name, ParseValue& value);
    bool parseValue(const char* &p, ParseValue& value);
    bool parseStringValue(const char*&p, QByteArray& stringValue);
    bool parseObject(const char*&p, ParseObject& obj);
    bool parseArray(const char*&p, QList<ParseValue>& array);
    void skipSpaces(const char* &p);
    bool isNameChar(char ch);
    bool isSpaceChar(char ch);
private:
    QHash<QString, GDBMIResultType> mResultTypes;
};

#endif // GDBMIRESULTPARSER_H
