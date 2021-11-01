#include "ojproblemsetmodel.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "../utils.h"

OJProblemSetModel::OJProblemSetModel(QObject *parent) : QAbstractListModel(parent)
{

}

void OJProblemSetModel::clear()
{
    beginRemoveRows(QModelIndex(),0,mProblemSet.problems.count()-1);
    mProblemSet.problems.clear();
    endRemoveRows();
}

int OJProblemSetModel::count()
{
    return mProblemSet.problems.count();
}

void OJProblemSetModel::create(const QString& name)
{
    mProblemSet.name = name;
    clear();
}

void OJProblemSetModel::rename(const QString &newName)
{
    if (mProblemSet.name!=newName)
        mProblemSet.name = newName;
}

QString OJProblemSetModel::name()
{
    return mProblemSet.name;
}

void OJProblemSetModel::addProblem(POJProblem problem)
{
    beginInsertRows(QModelIndex(), mProblemSet.problems.count(), mProblemSet.problems.count());
    mProblemSet.problems.append(problem);
    endInsertRows();
}

POJProblem OJProblemSetModel::problem(int index)
{
    return mProblemSet.problems[index];
}

void OJProblemSetModel::removeProblem(int index)
{
    Q_ASSERT(index>=0 && index < mProblemSet.problems.count());
    mProblemSet.problems.removeAt(index);
}

bool OJProblemSetModel::problemNameUsed(const QString &name)
{
    foreach (const POJProblem& problem, mProblemSet.problems) {
        if (name == problem->name)
            return true;
    }
    return false;
}

void OJProblemSetModel::removeAllProblems()
{
    clear();
}

void OJProblemSetModel::saveToFile(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QJsonObject obj;
        obj["name"]=mProblemSet.name;
        QJsonArray problemsArray;
        foreach (const POJProblem& problem, mProblemSet.problems) {
            QJsonObject problemObj;
            problemObj["name"]=problem->name;
            QJsonArray cases;
            foreach (const POJProblemCase& problemCase, problem->cases) {
                QJsonObject caseObj;
                caseObj["name"]=problemCase->name;
                caseObj["input"]=problemCase->input;
                caseObj["expected"]=problemCase->expected;
                cases.append(caseObj);
            }
            problemObj["cases"]=cases;
            problemsArray.append(problemObj);
        }
        obj["problems"]=problemsArray;
        QJsonDocument doc;
        doc.setObject(obj);
        file.write(doc.toJson());
        file.close();
    } else {
        throw FileError(QObject::tr("Can't open file '%1' for read.")
                        .arg(fileName));
    }
}

void OJProblemSetModel::loadFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        QByteArray content = file.readAll();
        QJsonParseError error;
        QJsonDocument doc(QJsonDocument::fromJson(content,&error));
        if (error.error!=QJsonParseError::NoError) {
            throw FileError(QObject::tr("Can't parse problem set file '%1':%2")
                            .arg(fileName)
                            .arg(error.errorString()));
        }
        beginResetModel();
        QJsonObject obj = doc.object();
        mProblemSet.name = obj["name"].toString();
        mProblemSet.problems.clear();
        QJsonArray problemsArray = obj["problems"].toArray();
        foreach (const QJsonValue& problemVal, problemsArray) {
            QJsonObject problemObj = problemVal.toObject();
            POJProblem problem = std::make_shared<OJProblem>();
            problem->name = problemObj["name"].toString();
            QJsonArray casesArray = problemObj["cases"].toArray();
            foreach (const QJsonValue& caseVal, casesArray) {
                QJsonObject caseObj = caseVal.toObject();
                POJProblemCase problemCase = std::make_shared<OJProblemCase>();
                problemCase->name = caseObj["name"].toString();
                problemCase->input = caseObj["input"].toString();
                problemCase->expected = caseObj["expected"].toString();
                problemCase->testState = ProblemCaseTestState::NoTested;
                problem->cases.append(problemCase);
            }
            mProblemSet.problems.append(problem);
        }
        endResetModel();
    } else {
        throw FileError(QObject::tr("Can't open file '%1' for read.")
                        .arg(fileName));
    }
}

int OJProblemSetModel::rowCount(const QModelIndex &parent) const
{
    return mProblemSet.problems.count();
}

QVariant OJProblemSetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return mProblemSet.problems[index.row()]->name;
    }
    return QVariant();
}

bool OJProblemSetModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (role == Qt::EditRole) {
        QString s = value.toString();
        if (!s.isEmpty()) {
            mProblemSet.problems[index.row()]->name = s;
            return true;
        }
    }
    return false;
}

Qt::ItemFlags OJProblemSetModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

OJProblemModel::OJProblemModel(QObject *parent): QAbstractListModel(parent)
{

}

const POJProblem &OJProblemModel::problem() const
{
    return mProblem;
}

void OJProblemModel::setProblem(const POJProblem &newProblem)
{
    if (newProblem!=mProblem) {
        beginResetModel();
        mProblem = newProblem;
        endResetModel();
    }
}

void OJProblemModel::addCase(POJProblemCase problemCase)
{
    if (mProblem==nullptr)
        return;
    beginInsertRows(QModelIndex(),mProblem->cases.count(),mProblem->cases.count());
    mProblem->cases.append(problemCase);
    endInsertRows();
}

void OJProblemModel::removeCase(int index)
{
    if (mProblem==nullptr)
        return;
    Q_ASSERT(index >= 0 && index < mProblem->cases.count());
    beginRemoveRows(QModelIndex(),index,index);
    mProblem->cases.removeAt(index);
    endRemoveRows();
}

POJProblemCase OJProblemModel::getCase(int index)
{
    if (mProblem==nullptr)
        return POJProblemCase();
    return mProblem->cases[index];
}

void OJProblemModel::clear()
{
    if (mProblem==nullptr)
        return;
    beginRemoveRows(QModelIndex(),0,mProblem->cases.count()-1);
    mProblem->cases.clear();
    endRemoveRows();
}

int OJProblemModel::count()
{
    if (mProblem == nullptr)
        return 0;
    return mProblem->cases.count();
}

int OJProblemModel::rowCount(const QModelIndex &) const
{
    if (mProblem==nullptr)
        return 0;
    return mProblem->cases.count();
}

QVariant OJProblemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (mProblem==nullptr)
        return QVariant();
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return mProblem->cases[index.row()]->name;
    }
    return QVariant();
}

bool OJProblemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
}

Qt::ItemFlags OJProblemModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
}
