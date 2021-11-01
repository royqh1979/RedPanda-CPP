#include "ojproblemset.h"

#include <QUuid>

OJProblemCase::OJProblemCase()
{
    QUuid uid = QUuid::createUuid();
    id = uid.toString();
}

const QString &OJProblemCase::getId() const
{
    return id;
}
