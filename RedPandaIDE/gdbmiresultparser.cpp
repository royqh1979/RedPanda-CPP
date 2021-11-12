#include "gdbmiresultparser.h"

#include <QList>

GDBMIResultParser::GDBMIResultParser()
{

}

bool GDBMIResultParser::parse(const QByteArray &record, GDBMIResultType &type, void **pResult)
{
    QList<QByteArray> tokens;
    tokens = tokenize(record);
}
