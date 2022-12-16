#ifndef FREEPROJECTSETFORMAT_H
#define FREEPROJECTSETFORMAT_H

#include "ojproblemset.h"

QList<POJProblem> importFreeProblemSet(const QString& filename);
void exportFreeProblemSet(const QList<POJProblem>& problems, const QString& filename);

#endif // FREEPROJECTSETFORMAT_H
