#include "ojproblemsetmodel.h"

OJProblemSetModel::OJProblemSetModel(QObject *parent) : QAbstractListModel(parent)
{

}

OJProblemCaseModel::OJProblemCaseModel(QObject *parent): QAbstractListModel(parent)
{

}

const POJProbelm &OJProblemCaseModel::problem() const
{
    return mProblem;
}

void OJProblemCaseModel::setProblem(const POJProbelm &newProblem)
{
    mProblem = newProblem;
}

int OJProblemCaseModel::rowCount(const QModelIndex &parent) const
{

}
