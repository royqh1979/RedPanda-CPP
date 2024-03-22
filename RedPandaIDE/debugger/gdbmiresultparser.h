/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
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
    MemoryBytes,
    CreateVar,
    ListVarChildren,
    UpdateVarValue
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
        qlonglong intValue(int defaultValue=-1) const;
        qulonglong hexValue(bool &ok) const;

        QString pathValue() const;
        QString utf8PathValue() const;
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
