#ifndef GDBMIRESULTPARSER_H
#define GDBMIRESULTPARSER_H

#include <QByteArray>
#include <QVariant>


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
    struct ParseValue {
    private:
        QVariant mData;
    };

    struct ParseObject {
        QHash<QByteArray, QVariant> props;
    };

public:
    GDBMIResultParser();
    bool parse(const QByteArray& record, GDBMIResultType& type, void** pResult);
};

#endif // GDBMIRESULTPARSER_H
