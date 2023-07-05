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
#include "ojproblemsetmodel.h"

#include <QDir>
#include <QFile>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include "../utils.h"
#include "../iconsmanager.h"
#include "../systemconsts.h"
#include "../settings.h"

OJProblemSetModel::OJProblemSetModel(QObject *parent) : QAbstractListModel(parent)
{

}

void OJProblemSetModel::clear()
{
    beginResetModel();
    mProblemSet.problems.clear();
    mProblemSet.exportFilename.clear();
    endResetModel();
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

QString OJProblemSetModel::name() const
{
    return mProblemSet.name;
}

QString OJProblemSetModel::exportFilename() const
{
    return mProblemSet.exportFilename;
}

void OJProblemSetModel::addProblem(const POJProblem& problem)
{
    beginInsertRows(QModelIndex(), mProblemSet.problems.count(), mProblemSet.problems.count());
    mProblemSet.problems.append(problem);
    endInsertRows();
}

void OJProblemSetModel::addProblems(const QList<POJProblem> &problems)
{
    if (problems.isEmpty())
        return;
    beginInsertRows(QModelIndex(), mProblemSet.problems.count(), mProblemSet.problems.count()+problems.count()-1);
    foreach( const POJProblem& p, problems)
        mProblemSet.problems.append(p);
    endInsertRows();
}

const QList<POJProblem> &OJProblemSetModel::problems() const
{
    return mProblemSet.problems;
}

POJProblem OJProblemSetModel::problem(int index) const
{
    return mProblemSet.problems[index];
}

void OJProblemSetModel::removeProblem(int index)
{
    Q_ASSERT(index>=0 && index < mProblemSet.problems.count());
    beginRemoveRows(QModelIndex(),index,index);
    mProblemSet.problems.removeAt(index);
    endRemoveRows();
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

void OJProblemSetModel::saveToFile(const QString &fileName, int currentIndex)
{
    QFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QJsonObject obj;
        mProblemSet.exportFilename=fileName;
        obj["name"]=mProblemSet.name;
        QJsonArray problemsArray;
        foreach (const POJProblem& problem, mProblemSet.problems) {
            QJsonObject problemObj;
            problemObj["name"]=problem->name;
            problemObj["url"]=problem->url;
            problemObj["description"]=problem->description;
            problemObj["time_limit"]=(int)problem->timeLimit;
            problemObj["memory_limit"]=(int)problem->memoryLimit;
            problemObj["time_limit_unit"]=(int)problem->timeLimitUnit;
            problemObj["memory_limit_unit"]=(int)problem->memoryLimitUnit;
            if (fileExists(problem->answerProgram))
                problemObj["answer_program"] = problem->answerProgram;
            QJsonArray cases;
            foreach (const POJProblemCase& problemCase, problem->cases) {
                QJsonObject caseObj;
                caseObj["name"]=problemCase->name;
                caseObj["input"]=problemCase->input;
                QString path = problemCase->inputFileName;
                QString prefix = includeTrailingPathDelimiter(extractFileDir(fileName));
                if (path.startsWith(prefix, PATH_SENSITIVITY)) {
                    path = "%ProblemSetPath%/"+ path.mid(prefix.length());
                }
                caseObj["input_filename"]=path;
                path = problemCase->expectedOutputFileName;
                if (path.startsWith(prefix, PATH_SENSITIVITY)) {
                    path = "%ProblemSetPath%/"+ path.mid(prefix.length());
                }
                caseObj["expected_output_filename"]=path;
                caseObj["expected"]=problemCase->expected;
                cases.append(caseObj);
            }
            problemObj["cases"]=cases;
            problemsArray.append(problemObj);
        }
        obj["problems"]=problemsArray;
        obj["current_index"]=currentIndex;
        QJsonDocument doc;
        doc.setObject(obj);
        file.write(doc.toJson());
        file.close();
    } else {
        throw FileError(QObject::tr("Can't open file '%1' for read.")
                        .arg(fileName));
    }
}

void OJProblemSetModel::loadFromFile(const QString &fileName, int& currentIndex)
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
        currentIndex = obj["current_index"].toInt(-1);
        mProblemSet.problems.clear();
        QJsonArray problemsArray = obj["problems"].toArray();
        foreach (const QJsonValue& problemVal, problemsArray) {
            QJsonObject problemObj = problemVal.toObject();
            POJProblem problem = std::make_shared<OJProblem>();
            problem->name = problemObj["name"].toString();
            problem->url = problemObj["url"].toString();
            problem->timeLimit = problemObj["time_limit"].toInt();
            problem->memoryLimit = problemObj["memory_limit"].toInt();
            problem->timeLimitUnit = (ProblemTimeLimitUnit)problemObj["time_limit_unit"].toInt();
            problem->memoryLimitUnit = (ProblemMemoryLimitUnit)problemObj["memory_limit_unit"].toInt();

            problem->description = problemObj["description"].toString();
            problem->answerProgram = problemObj["answer_program"].toString();
            QJsonArray casesArray = problemObj["cases"].toArray();
            foreach (const QJsonValue& caseVal, casesArray) {
                QJsonObject caseObj = caseVal.toObject();
                POJProblemCase problemCase = std::make_shared<OJProblemCase>();
                problemCase->name = caseObj["name"].toString();
                problemCase->input = caseObj["input"].toString();
                problemCase->expected = caseObj["expected"].toString();
                QString path = caseObj["input_filename"].toString();
                if (path.startsWith("%ProblemSetPath%/")) {
                    path = includeTrailingPathDelimiter(extractFileDir(fileName))+
                            path.mid(QLatin1String("%ProblemSetPath%/").size());
                }
                problemCase->inputFileName=path;
                path = caseObj["expected_output_filename"].toString();
                if (path.startsWith("%ProblemSetPath%/")) {
                    path = includeTrailingPathDelimiter(extractFileDir(fileName))+
                            path.mid(QLatin1String("%ProblemSetPath%/").size());
                }
                problemCase->expectedOutputFileName=path;
                problemCase->testState = ProblemCaseTestState::NotTested;
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

void OJProblemSetModel::load(int &currentIndex)
{
    QDir dir(pSettings->dirs().config());
    QString filename=dir.filePath(DEV_PROBLEM_SET_FILE);
    if (fileExists(filename))
        loadFromFile(filename,currentIndex);
}

void OJProblemSetModel::save(int currentIndex)
{
    QDir dir(pSettings->dirs().config());
    QString filename=dir.filePath(DEV_PROBLEM_SET_FILE);
    saveToFile(filename,currentIndex);
}

void OJProblemSetModel::updateProblemAnswerFilename(const QString &oldFilename, const QString &newFilename)
{
    foreach (POJProblem problem, mProblemSet.problems) {
        if (QString::compare(problem->answerProgram,oldFilename,PATH_SENSITIVITY)==0) {
            problem->answerProgram = newFilename;
        }
    }
}

int OJProblemSetModel::rowCount(const QModelIndex &) const
{
    return mProblemSet.problems.count();
}

QVariant OJProblemSetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return mProblemSet.problems[index.row()]->name;
    } else if (role == Qt::ToolTipRole) {
        POJProblem problem = mProblemSet.problems[index.row()];

        QString s;
        s=QString("<h3>%1</h3>").arg(problem->name);
        if (!problem->description.isEmpty())
            s+=problem->description;

        return s;
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
            emit problemNameChanged(index.row());
            return true;
        }
    }
    return false;
}

Qt::ItemFlags OJProblemSetModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::NoItemFlags;
    if (index.isValid()) {
        flags = Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
    } else if (index.row() == -1) {
        // -1 means it's a drop target?
        flags = Qt::ItemIsDropEnabled;
    }
    return flags ;
}

Qt::DropActions OJProblemSetModel::supportedDropActions() const
{
    return Qt::DropAction::MoveAction;
}

bool OJProblemSetModel::moveRows(const QModelIndex &/*sourceParent*/, int sourceRow, int count, const QModelIndex &/*destinationParent*/, int destinationChild)
{
    if (sourceRow < 0
        || sourceRow + count - 1 >= mProblemSet.problems.count()
        || destinationChild < 0
        || destinationChild > mProblemSet.problems.count()
        || sourceRow == destinationChild
        || count <= 0) {
        return false;
    }
    if (!beginMoveRows(QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), destinationChild))
        return false;

    int fromRow = sourceRow;
    if (destinationChild < sourceRow)
        fromRow += count - 1;
    else
        destinationChild--;
    while (count--)
        mProblemSet.problems.move(fromRow, destinationChild);
    endMoveRows();
    return true;
}

OJProblemModel::OJProblemModel(QObject *parent): QAbstractTableModel(parent)
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

void OJProblemModel::removeCases()
{
    beginRemoveRows(QModelIndex(),0,mProblem->cases.count());
    mProblem->cases.clear();
    endRemoveRows();
}

POJProblemCase OJProblemModel::getCase(int index)
{
    if (mProblem==nullptr)
        return POJProblemCase();
    return mProblem->cases[index];
}

POJProblemCase OJProblemModel::getCaseById(const QString& id)
{
    if (mProblem==nullptr)
        return POJProblemCase();
    foreach (const POJProblemCase& problemCase, mProblem->cases){
        if (problemCase->getId() == id)
            return problemCase;
    }
    return POJProblemCase();
}

int OJProblemModel::getCaseIndexById(const QString &id)
{
    if (mProblem==nullptr)
        return -1;
    for (int i=0;i<mProblem->cases.size();i++) {
        const POJProblemCase& problemCase = mProblem->cases[i];
        if (problemCase->getId() == id)
            return i;
    }
    return -1;
}

void OJProblemModel::clear()
{
    if (mProblem==nullptr)
        return;
    beginResetModel();
    mProblem->cases.clear();
    endResetModel();
}

int OJProblemModel::count()
{
    if (mProblem == nullptr)
        return 0;
    return mProblem->cases.count();
}

void OJProblemModel::update(int row)
{
    emit dataChanged(index(row,0),index(row,0));
}

QString OJProblemModel::getTitle()
{
    if (!mProblem)
        return "";
    int total = mProblem->cases.count();
    int passed = 0;
    foreach (const POJProblemCase& problemCase, mProblem->cases) {
        if (problemCase->testState == ProblemCaseTestState::Passed)
            passed ++ ;
    }
    QString title = QString("%1 (%2/%3)").arg(mProblem->name)
            .arg(passed).arg(total);
    if (!mProblem->url.isEmpty()) {
        title = QString("<a href=\"%1\">%2</a>").arg(mProblem->url,title);
    }
    return title;
}

QString OJProblemModel::getTooltip()
{
    if (!mProblem)
        return "";
    QString s;
    s=QString("<h3>%1</h3>").arg(mProblem->name);
    if (!mProblem->description.isEmpty())
        s+=mProblem->description;
    return s;
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
    switch (index.column()) {
    case 0:
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            POJProblemCase problemCase = mProblem->cases[index.row()];
            return problemCase->name;
        } else if (role == Qt::DecorationRole) {
            switch (mProblem->cases[index.row()]->testState) {
            case ProblemCaseTestState::Failed:
                return pIconsManager->getIcon(IconsManager::ACTION_PROBLEM_FALIED);
            case ProblemCaseTestState::Passed:
                return pIconsManager->getIcon(IconsManager::ACTION_PROBLEM_PASSED);
            case ProblemCaseTestState::Testing:
                return pIconsManager->getIcon(IconsManager::ACTION_PROBLEM_TESTING);
            default:
                return QVariant();
            }
        }
        break;
    case 1:
        if (role == Qt::DisplayRole) {
             POJProblemCase problemCase = mProblem->cases[index.row()];
             if (problemCase->testState == ProblemCaseTestState::Passed
                     || problemCase->testState == ProblemCaseTestState::Failed)
                 return problemCase->runningTime;
             else
                 return "";
        }
        break;
#ifdef Q_OS_WIN
    case 2:
        if (role == Qt::DisplayRole) {
             POJProblemCase problemCase = mProblem->cases[index.row()];
             if (problemCase->testState == ProblemCaseTestState::Passed
                     || problemCase->testState == ProblemCaseTestState::Failed)
                 return problemCase->runningMemory/1024;
             else
                 return "";
        }
        break;
#endif
    }

    return QVariant();
}

bool OJProblemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (index.column()!=0)
        return false;
    if (mProblem==nullptr)
        return false;
    if (role == Qt::EditRole ) {
        QString s = value.toString();
        if (!s.isEmpty()) {
            mProblem->cases[index.row()]->name = s;
            return true;
        }
    }
    return false;
}

Qt::ItemFlags OJProblemModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags flags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (idx.column()==0)
        flags |= Qt::ItemIsEditable ;
    if (idx.isValid())
        flags |= Qt::ItemIsDragEnabled;
    flags |= Qt::ItemIsDropEnabled;
    return flags;
}

int OJProblemModel::columnCount(const QModelIndex &/*parent*/) const
{
#ifdef Q_OS_WIN
    return 3;
#else
    return 2;
#endif
}

QVariant OJProblemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return tr("Name");
        case 1:
            return tr("Time(ms)");
#ifdef Q_OS_WIN
        case 2:
            return tr("Memory(kb)");
#endif
        }
    }
    return QVariant();
}

Qt::DropActions OJProblemModel::supportedDropActions() const
{
    return Qt::DropAction::MoveAction;
}

bool OJProblemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int /* column */, const QModelIndex &parent)
{
    mMoveTargetRow=row;
    if (mMoveTargetRow==-1)
        mMoveTargetRow=mProblem->cases.length();
    return  QAbstractTableModel::dropMimeData(data,action,row,0,parent);
}

bool OJProblemModel::insertRows(int /* row */, int /*count*/, const QModelIndex &/*parent*/)
{
    return true;
}

bool OJProblemModel::removeRows(int row, int count, const QModelIndex &/*parent*/)
{
    int sourceRow = row;
    int destinationChild = mMoveTargetRow;
    mMoveTargetRow=-1;
    if (sourceRow < 0
        || sourceRow + count - 1 >= mProblem->cases.count()
        || destinationChild < 0
        || destinationChild > mProblem->cases.count()
        || sourceRow == destinationChild
        || count <= 0) {
        return false;
    }
    if (!beginMoveRows(QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), destinationChild))
        return false;

    int fromRow = sourceRow;
    if (destinationChild < sourceRow)
        fromRow += count - 1;
    else
        destinationChild--;
    while (count--)
        mProblem->cases.move(fromRow, destinationChild);
    endMoveRows();
    return true;
}

