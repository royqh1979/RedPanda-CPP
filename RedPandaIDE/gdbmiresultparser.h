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
    Memory
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
        const ParseValue operator[](const QByteArray& name) const;
        ParseValue& operator[](const QByteArray& name);
        ParseObject& operator=(const ParseObject& object);
    private:
        QHash<QByteArray, ParseValue> mProps;
    };

    class ParseValue {
    public:
        explicit ParseValue();
        explicit ParseValue(const QString& value);
        explicit ParseValue(const ParseObject &object);
        explicit ParseValue(const QList<ParseObject>& array);
        ParseValue(const ParseValue&) = delete;
        const QString &value() const;
        const QList<ParseObject> &array() const;
        const ParseObject &object() const;
        ParseValueType type() const;
        ParseValue& operator=(const QString& value);
        ParseValue& operator=(const ParseObject& object);
        ParseValue& operator=(const QList<ParseObject>& array);
        ParseValue& operator=(const ParseValue& value);
    private:
        QString mValue;
        QList<ParseObject> mArray;
        ParseObject mObject;
        ParseValueType mType;
    };

    using PParseValue = std::shared_ptr<ParseValue>;

public:
    GDBMIResultParser();
    bool parse(const QByteArray& record, GDBMIResultType& type, ParseValue& value);
private:
    bool parseNameAndValue(const char *&p,QByteArray& name, ParseValue& value);
    bool parseValue(const char* &p, ParseValue& value);
    bool parseStringValue(const char*&p, QByteArray& stringValue);
    bool parseObject(const char*&p, ParseObject& obj);
    bool parseArray(const char*&p, QList<ParseObject>& array);
    void skipSpaces(const char* &p);
    bool isNameChar(char ch);
    bool isSpaceChar(char ch);
private:
    QHash<QByteArray, GDBMIResultType> mResultTypes;
};

#endif // GDBMIRESULTPARSER_H
