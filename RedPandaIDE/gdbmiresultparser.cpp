#include "gdbmiresultparser.h"

#include <QFileInfo>
#include <QList>

GDBMIResultParser::GDBMIResultParser()
{
    mResultTypes.insert("bkpt",GDBMIResultType::Breakpoint);
    mResultTypes.insert("BreakpointTable",GDBMIResultType::BreakpointTable);
    mResultTypes.insert("stack",GDBMIResultType::FrameStack);
    mResultTypes.insert("variables", GDBMIResultType::LocalVariables);
    mResultTypes.insert("frame",GDBMIResultType::Frame);
    mResultTypes.insert("asm_insns",GDBMIResultType::Disassembly);
    mResultTypes.insert("value",GDBMIResultType::Evaluation);
    mResultTypes.insert("register-names",GDBMIResultType::RegisterNames);
    mResultTypes.insert("register-values",GDBMIResultType::RegisterValues);
    mResultTypes.insert("memory",GDBMIResultType::Memory);
}

bool GDBMIResultParser::parse(const QByteArray &record, GDBMIResultType &type, ParseValue& value)
{
    const char* p = record.data();
    QByteArray name;
    bool result = parseNameAndValue(p,name,value);
    if (!result)
        return false;
//    if (*p!=0)
//        return false;
    if (!mResultTypes.contains(name))
        return false;
    type = mResultTypes[name];
    return true;
}

bool GDBMIResultParser::parseAsyncResult(const QByteArray &record, QByteArray &result, ParseObject &multiValue)
{
    const char* p =record.data();
    if (*p!='*')
        return false;
    p++;
    const char* start;
    while (*p && *p!=',')
        p++;
    result = QByteArray(start,p-start);
    if (*p==0)
        return true;
    return parseMultiValues(p,multiValue);
}

bool GDBMIResultParser::parseMultiValues(const char* p, ParseObject &multiValue)
{
    while (*p) {
        QByteArray propName;
        ParseValue propValue;
        bool result = parseNameAndValue(p,propName,propValue);
        if (result) {
            multiValue[propName]=propValue;
        } else {
            return false;
        }
        skipSpaces(p);
        if (*p!=',')
            return false;
        p++; //skip ','
        skipSpaces(p);
        p++;
    }
    return true;
}

bool GDBMIResultParser::parseNameAndValue(const char *&p, QByteArray &name, ParseValue &value)
{
    skipSpaces(p);
    const char* nameStart =p;
    while (*p!=0 && isNameChar(*p)) {
        p++;
    }
    if (*p==0)
        return false;
    name = QByteArray(nameStart,p-nameStart);
    skipSpaces(p);
    if (*p!='=')
        return false;
    return parseValue(p,value);
}

bool GDBMIResultParser::parseValue(const char *&p, ParseValue &value)
{
    skipSpaces(p);
    bool result;
    switch (*p) {
    case '{': {
        ParseObject obj;
        result = parseObject(p,obj);
        value = obj;
        break;
    }
    case '[': {
        QList<ParseObject> array;
        result = parseArray(p,array);
        value = array;
        break;
    }
    case '"': {
        QByteArray s;
        result = parseStringValue(p,s);
        value = s;
        break;
    }
    default:
        return false;
    }
    if (!result)
        return false;
    skipSpaces(p);
    if (*p!=0 && *p!=',')
        return false;
    return true;
}

bool GDBMIResultParser::parseStringValue(const char *&p, QByteArray& stringValue)
{
    if (*p!='"')
        return false;
    p++;
    const char* valueStart = p;
    while (*p!=0) {
        if (*p == '"') {
            break;
        } else if (*p=='\\' && *(p+1)!=0) {
            p+=2;
        } else {
            p++;
        }
    }
    if (*p=='"') {
        stringValue = QByteArray(valueStart,p-valueStart);
        p++; //skip '"'
        return true;
    }
    return false;
}

bool GDBMIResultParser::parseObject(const char *&p, ParseObject &obj)
{
    if (*p!='{')
        return false;
    p++;

    if (*p!='}') {
        while (*p!=0) {
            QByteArray propName;
            ParseValue propValue;
            bool result = parseNameAndValue(p,propName,propValue);
            if (result) {
                obj[propName]=propValue;
            } else {
                return false;
            }
            skipSpaces(p);
            if (*p=='}')
                break;
            if (*p!=',')
                return false;
            p++; //skip ','
            skipSpaces(p);
        }
    }
    if (*p=='}') {
        p++; //skip '}'
        return true;
    }
    return false;
}

bool GDBMIResultParser::parseArray(const char *&p, QList<GDBMIResultParser::ParseValue> &array)
{
    if (*p!='[')
        return false;
    p++;
    if (*p!=']') {
        while (*p!=0) {
            skipSpaces(p);
            ParseValue val;
            bool result = parseValue(p,val);
            if (result) {
                array.append(val);
            } else {
                return false;
            }
            skipSpaces(p);
            if (*p==']')
                break;
            if (*p!=',')
                return false;
            p++; //skip ','
            skipSpaces(p);
        }
    }
    if (*p==']') {
        p++; //skip ']'
        return true;
    }
    return false;
}

bool GDBMIResultParser::isNameChar(char ch)
{
    if (ch=='-')
        return true;
    if (ch>='a' && ch<='z')
        return true;
    if (ch>='A' && ch<='Z')
        return true;
    return false;
}

bool GDBMIResultParser::isSpaceChar(char ch)
{
    switch(ch) {
    case ' ':
    case '\t':
        return true;
    }
    return false;
}

void GDBMIResultParser::skipSpaces(const char *&p)
{
    while (*p!=0 && isSpaceChar(*p))
        p++;
}

const QByteArray &GDBMIResultParser::ParseValue::value() const
{
    Q_ASSERT(mType == ParseValueType::Value);
    return mValue;
}

const QList<::GDBMIResultParser::ParseValue> &GDBMIResultParser::ParseValue::array() const
{
    Q_ASSERT(mType == ParseValueType::Array);
    return mArray;
}

const GDBMIResultParser::ParseObject &GDBMIResultParser::ParseValue::object() const
{
    Q_ASSERT(mType == ParseValueType::Object);
    return mObject;
}

int GDBMIResultParser::ParseValue::intValue(int defaultValue) const
{
    Q_ASSERT(mType == ParseValueType::Value);
    bool ok;
    int value = QString(mValue).toInt(&ok);
    if (ok)
        return value;
    else
        return defaultValue;
}

QString GDBMIResultParser::ParseValue::pathValue() const
{
    Q_ASSERT(mType == ParseValueType::Value);
    return QFileInfo(QString::fromLocal8Bit(mValue)).absoluteFilePath();
}

GDBMIResultParser::ParseValueType GDBMIResultParser::ParseValue::type() const
{
    return mType;
}

GDBMIResultParser::ParseValue::ParseValue():
    mType(ParseValueType::NotAssigned) {

}

GDBMIResultParser::ParseValue::ParseValue(const QByteArray &value):
    mValue(value),
    mType(ParseValueType::Value)
{
}

GDBMIResultParser::ParseValue::ParseValue(const ParseObject &object):
    mObject(object),
    mType(ParseValueType::Object)
{
}

GDBMIResultParser::ParseValue::ParseValue(const QList<ParseValue> &array):
    mArray(array),
    mType(ParseValueType::Array)
{
}

GDBMIResultParser::ParseValue &GDBMIResultParser::ParseValue::operator=(const QByteArray &value)
{
    Q_ASSERT(mType == ParseValueType::NotAssigned);
    mType = ParseValueType::Value;
    mValue = value;
}

GDBMIResultParser::ParseValue &GDBMIResultParser::ParseValue::operator=(const ParseObject& object)
{
    Q_ASSERT(mType == ParseValueType::NotAssigned);
    mType = ParseValueType::Object;
    mObject = object;
}

GDBMIResultParser::ParseValue &GDBMIResultParser::ParseValue::operator=(const QList<ParseValue>& array)
{
    Q_ASSERT(mType == ParseValueType::NotAssigned);
    mType = ParseValueType::Array;
    mArray = array;
}


const GDBMIResultParser::ParseValue GDBMIResultParser::ParseObject::operator[](const QByteArray &name) const
{
    if (mProps.contains(name))
        return mProps[name];
    return ParseValue();
}

GDBMIResultParser::ParseObject &GDBMIResultParser::ParseObject::operator=(const ParseObject &object)
{
    mProps = object.mProps;
}

GDBMIResultParser::ParseValue &GDBMIResultParser::ParseObject::operator[](const QByteArray &name) {
    if (!mProps.contains(name))
        mProps[name]=ParseValue();
    return mProps[name];
}

