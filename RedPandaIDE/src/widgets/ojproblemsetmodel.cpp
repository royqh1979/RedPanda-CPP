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
#include "../iconsmanager.h"
#include "../systemconsts.h"
#include "../settings.h"

OJProblemSetModel::OJProblemSetModel(QObject *parent) : QAbstractListModel(parent)
{
    connect(&mProblemSet, &OJProblemSet::modifiedChanged, this, &OJProblemSetModel::onProblemSetModified);
    connect(&mProblemSet, &OJProblemSet::problemModified, this, &OJProblemSetModel::onProblemModified);
}

void OJProblemSetModel::clear()
{
    beginResetModel();
    mProblemSet.clearProblems();
    mProblemSet.setExportFilename("");
    endResetModel();
}

int OJProblemSetModel::count()
{
    return mProblemSet.problems().count();
}

void OJProblemSetModel::create(const QString& name)
{
    clear();
    mProblemSet.setName(name);
}

void OJProblemSetModel::rename(const QString &newName)
{
    mProblemSet.setName(newName);
}

QString OJProblemSetModel::name() const
{
    return mProblemSet.name();
}

QString OJProblemSetModel::exportFilename() const
{
    return mProblemSet.exportFilename();
}

void OJProblemSetModel::addProblem(const POJProblem& problem)
{
    beginInsertRows(QModelIndex(), mProblemSet.problems().count(), mProblemSet.problems().count());
    mProblemSet.addProblem(problem);
    endInsertRows();
}

void OJProblemSetModel::addProblems(const QList<POJProblem> &problems)
{
    if (problems.isEmpty())
        return;
    beginInsertRows(QModelIndex(), mProblemSet.problems().count(), mProblemSet.problems().count()+problems.count()-1);
    foreach( const POJProblem& p, problems)
        mProblemSet.addProblem(p);
    endInsertRows();
}

const QList<POJProblem> &OJProblemSetModel::problems() const
{
    return mProblemSet.problems();
}

POJProblem OJProblemSetModel::problem(int index) const
{
    return mProblemSet.problems()[index];
}

void OJProblemSetModel::removeProblem(int index)
{
    Q_ASSERT(index>=0 && index < mProblemSet.problems().count());
    beginRemoveRows(QModelIndex(),index,index);
    mProblemSet.removeProblem(index);
    endRemoveRows();
}

bool OJProblemSetModel::problemNameUsed(const QString &name)
{
    foreach (const POJProblem& problem, mProblemSet.problems()) {
        if (name == problem->name())
            return true;
    }
    return false;
}

void OJProblemSetModel::removeAllProblems()
{
    clear();
}

void OJProblemSetModel::saveToFile(const QString &filePath,bool keepFilePath,int currentIndex)
{
    if (keepFilePath) {
        mFilePath = filePath;
        emit problemSetNameChanged();
    }
    QFile file(filePath);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QJsonObject obj;
        mProblemSet.setExportFilename(filePath);
        obj["name"]=mProblemSet.name();
        QJsonArray problemsArray;
        foreach (const POJProblem& problem, mProblemSet.problems()) {
            QJsonObject problemObj;
            problemObj["name"]=problem->name();
            problemObj["url"]=problem->url();
            problemObj["description"]=problem->description();
            problemObj["time_limit"]=(int)problem->timeLimit();
            problemObj["memory_limit"]=(int)problem->memoryLimit();
            problemObj["time_limit_unit"]=(int)problem->timeLimitUnit();
            problemObj["memory_limit_unit"]=(int)problem->memoryLimitUnit();
            if (fileExists(problem->answerProgram()))
                problemObj["answer_program"] = problem->answerProgram();
            QJsonArray cases;
            foreach (const POJProblemCase& problemCase, problem->cases()) {
                QJsonObject caseObj;
                caseObj["name"]=problemCase->name();
                caseObj["input"]=problemCase->input();
                QString path = problemCase->inputFileName();
                QString prefix = includeTrailingPathDelimiter(extractFileDir(filePath));
                if (path.startsWith(prefix, PATH_SENSITIVITY)) {
                    path = "%ProblemSetPath%/"+ path.mid(prefix.length());
                }
                caseObj["input_filename"]=path;
                path = problemCase->expectedOutputFileName();
                if (path.startsWith(prefix, PATH_SENSITIVITY)) {
                    path = "%ProblemSetPath%/"+ path.mid(prefix.length());
                }
                caseObj["expected_output_filename"]=path;
                caseObj["expected"]=problemCase->expected();
                cases.append(caseObj);
                problemCase->setModified(false);
            }
            problemObj["cases"]=cases;
            problemsArray.append(problemObj);
            problem->setModified(false);
        }
        obj["problems"]=problemsArray;
        obj["current_index"]=currentIndex;
        QJsonDocument doc;
        doc.setObject(obj);
        mProblemSet.setModified(false);
        file.write(doc.toJson());
        file.close();
    } else {
        throw FileError(QObject::tr("Can't open file '%1' for write.")
                        .arg(filePath));
    }
}

void OJProblemSetModel::loadFromFile(const QString &filePath,bool keepFilePath,int& currentIndex)
{
    if (keepFilePath) {
        mFilePath = filePath;
        emit problemSetNameChanged();
    }
    QFile file(filePath);
    if (file.open(QFile::ReadOnly)) {
        QByteArray content = file.readAll().trimmed();
        if (content.isEmpty())
            return;
        QJsonParseError error;
        QJsonDocument doc(QJsonDocument::fromJson(content,&error));
        if (error.error!=QJsonParseError::NoError) {
            throw FileError(QObject::tr("Can't parse problem set file '%1':%2")
                            .arg(filePath,
                                 error.errorString()));
        }
        beginResetModel();
        QJsonObject obj = doc.object();
        mProblemSet.setName(obj["name"].toString());
        currentIndex = obj["current_index"].toInt(-1);
        mProblemSet.clearProblems();
        QJsonArray problemsArray = obj["problems"].toArray();
        for (const QJsonValue& problemVal:problemsArray) {
            QJsonObject problemObj = problemVal.toObject();
            POJProblem problem = std::make_shared<OJProblem>();
            problem->setName(problemObj["name"].toString());
            problem->setUrl(problemObj["url"].toString());
            problem->setTimeLimit(problemObj["time_limit"].toInt());
            problem->setMemoryLimit(problemObj["memory_limit"].toInt());
            problem->setTimeLimitUnit((ProblemTimeLimitUnit)problemObj["time_limit_unit"].toInt());
            problem->setMemoryLimitUnit((ProblemMemoryLimitUnit)problemObj["memory_limit_unit"].toInt());

            problem->setDescription(problemObj["description"].toString());
            problem->setAnswerProgram(problemObj["answer_program"].toString());
            QJsonArray casesArray = problemObj["cases"].toArray();
            for(const QJsonValue& caseVal:casesArray) {
                QJsonObject caseObj = caseVal.toObject();
                POJProblemCase problemCase = std::make_shared<OJProblemCase>();
                problemCase->setName( caseObj["name"].toString());
                problemCase->setInput( caseObj["input"].toString());
                problemCase->setExpected( caseObj["expected"].toString());
                QString path = caseObj["input_filename"].toString();
                if (path.startsWith("%ProblemSetPath%/")) {
                    path = includeTrailingPathDelimiter(extractFileDir(filePath))+
                            path.mid(QLatin1String("%ProblemSetPath%/").size());
                }
                problemCase->setInputFileName(path);
                path = caseObj["expected_output_filename"].toString();
                if (path.startsWith("%ProblemSetPath%/")) {
                    path = includeTrailingPathDelimiter(extractFileDir(filePath))+
                            path.mid(QLatin1String("%ProblemSetPath%/").size());
                }
                problemCase->setExpectedOutputFileName(path);
                problemCase->testState = ProblemCaseTestState::NotTested;
                problemCase->setModified(false);
                problem->addCase(problemCase);
            }
            problem->setModified(false);
            mProblemSet.addProblem(problem);
        }
        mProblemSet.setModified(false);
        endResetModel();
    } else {
        throw FileError(QObject::tr("Can't open file '%1' for read.")
                        .arg(filePath));
    }
}

void OJProblemSetModel::updateProblemAnswerFilename(const QString &oldFilename, const QString &newFilename)
{
    foreach (POJProblem problem, mProblemSet.problems()) {
        if (QString::compare(problem->answerProgram(),oldFilename,PATH_SENSITIVITY)==0) {
            problem->setAnswerProgram( newFilename);
        }
    }
}

const OJProblemSet *OJProblemSetModel::problemSet() const
{
    return &mProblemSet;
}

void OJProblemSetModel::onProblemSetModified()
{
    emit problemSetNameChanged();
}

void OJProblemSetModel::onProblemModified(const QString &id)
{
    for(int i=0;i<mProblemSet.problems().count();i++) {
        if (mProblemSet.problems()[i]->id() == id) {
            emit problemNameChanged(i);
            break;
        }
    }
}

const QString &OJProblemSetModel::filePath() const
{
    return mFilePath;
}

int OJProblemSetModel::rowCount(const QModelIndex &) const
{
    return mProblemSet.problems().count();
}

QVariant OJProblemSetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::EditRole) {
        return mProblemSet.problems()[index.row()]->name();
    } else if (role == Qt::DisplayRole) {
        POJProblem problem = mProblemSet.problems()[index.row()];
        QString name = problem->name();
        if (problem->isModified())
            name += "[*]";
        return name;
    } else if (role == Qt::ToolTipRole) {
        POJProblem problem = mProblemSet.problems()[index.row()];

        QString s;
        s=QString("<h3>%1</h3>").arg(problem->name());
        if (!problem->description().isEmpty())
            s+=problem->description();

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
            mProblemSet.problems()[index.row()]->setName(s);
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
        || sourceRow + count - 1 >= mProblemSet.problems().count()
        || destinationChild < 0
        || destinationChild > mProblemSet.problems().count()
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
        mProblemSet.moveProblem(fromRow, destinationChild);
    endMoveRows();
    return true;
}

OJProblemModel::OJProblemModel(IconsManager *iconsManager, QObject *parent): QAbstractTableModel(parent)
{
    Q_ASSERT(iconsManager!=nullptr);
    mIconsManager = iconsManager;
}

const POJProblem &OJProblemModel::problem() const
{
    return mProblem;
}

void OJProblemModel::setProblem(const POJProblem &newProblem)
{
    if (newProblem!=mProblem) {
        if (mProblem) {
            disconnect(mProblem.get(), &OJProblem::problemCaseModified, this, &OJProblemModel::onProblemCaseModified);
            disconnect(mProblem.get(), &OJProblem::modifiedChanged, this, &OJProblemModel::onProblemModified);
        }
        beginResetModel();
        mProblem = newProblem;
        if (mProblem) {
            connect(mProblem.get(), &OJProblem::problemCaseModified, this, &OJProblemModel::onProblemCaseModified);
            connect(mProblem.get(), &OJProblem::modifiedChanged, this, &OJProblemModel::onProblemModified);
        }
        endResetModel();
    }
}

void OJProblemModel::addCase(POJProblemCase problemCase)
{
    if (mProblem==nullptr)
        return;
    beginInsertRows(QModelIndex(),mProblem->cases().count(),mProblem->cases().count());
    mProblem->addCase(problemCase);
    endInsertRows();
}

void OJProblemModel::removeCase(int index)
{
    if (mProblem==nullptr)
        return;
    Q_ASSERT(index >= 0 && index < mProblem->cases().count());
    beginRemoveRows(QModelIndex(),index,index);
    mProblem->removeCase(index);
    endRemoveRows();
}

void OJProblemModel::removeCases()
{
    beginRemoveRows(QModelIndex(),0,mProblem->cases().count());
    mProblem->clearCases();
    endRemoveRows();
}

POJProblemCase OJProblemModel::getCase(int index)
{
    if (mProblem==nullptr)
        return POJProblemCase();
    return mProblem->cases()[index];
}

POJProblemCase OJProblemModel::getCaseById(const QString& id)
{
    if (mProblem==nullptr)
        return POJProblemCase();
    foreach (const POJProblemCase& problemCase, mProblem->cases()){
        if (problemCase->id() == id)
            return problemCase;
    }
    return POJProblemCase();
}

int OJProblemModel::getCaseIndexById(const QString &id)
{
    if (mProblem==nullptr)
        return -1;
    for(int i=0;i<mProblem->cases().count();i++) {
        const POJProblemCase& problemCase = mProblem->cases()[i];
        if (problemCase->id() == id)
            return i;
    }
    return -1;
}

void OJProblemModel::clear()
{
    if (mProblem==nullptr)
        return;
    beginResetModel();
    mProblem->clearCases();
    endResetModel();
}

int OJProblemModel::count()
{
    if (mProblem == nullptr)
        return 0;
    return mProblem->cases().count();
}

void OJProblemModel::update(int row)
{
    emit dataChanged(index(row,0),index(row,0));
}

QString OJProblemModel::getTitle()
{
    if (!mProblem)
        return "";
    int total = mProblem->cases().count();
    int passed = 0;
    foreach (const POJProblemCase& problemCase, mProblem->cases()) {
        if (problemCase->testState == ProblemCaseTestState::Passed)
            passed ++ ;
    }
    QString name = mProblem->name();
    if (mProblem->isModified())
        name += "[*]";
    QString title = QString("%1 (%2/%3)").arg(name)
            .arg(passed).arg(total);
    if (!mProblem->url().isEmpty()) {
        title = QString("<a href=\"%1\">%2</a>").arg(mProblem->url(),title);
    }
    return title;
}

QString OJProblemModel::getTooltip()
{
    if (!mProblem)
        return "";
    QString s;
    s=QString("<h3>%1</h3>").arg(mProblem->name());
    if (!mProblem->description().isEmpty())
        s+=mProblem->description();
    return s;
}

void OJProblemModel::onProblemModified(const QString& id)
{
    Q_UNUSED(id);
    emit problemModified();
}

void OJProblemModel::onProblemCaseModified(const QString& id)
{
    for (int i=0;i<mProblem->cases().count();i++) {
        if (id == mProblem->cases()[i]->id()) {
            QModelIndex idx = index(i,0);
            emit dataChanged(idx,idx);
            break;
        }
    }
}

int OJProblemModel::rowCount(const QModelIndex &) const
{
    if (mProblem==nullptr)
        return 0;
    return mProblem->cases().count();
}

QVariant OJProblemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (mProblem==nullptr)
        return QVariant();
    switch (index.column()) {
    case 0:
        if (role == Qt::DisplayRole) {
            POJProblemCase problemCase = mProblem->cases()[index.row()];
            QString name =  problemCase->name();
            if (problemCase->isModified()) {
                name += "[*]";
            }
            return name;
        } else if (role == Qt::EditRole) {
            POJProblemCase problemCase = mProblem->cases()[index.row()];
            return problemCase->name();
        } else if (role == Qt::DecorationRole) {
            switch (mProblem->cases()[index.row()]->testState) {
            case ProblemCaseTestState::Failed:
                return mIconsManager->getIcon(IconsManager::ACTION_PROBLEM_FALIED);
            case ProblemCaseTestState::Passed:
                return mIconsManager->getIcon(IconsManager::ACTION_PROBLEM_PASSED);
            case ProblemCaseTestState::Testing:
                return mIconsManager->getIcon(IconsManager::ACTION_PROBLEM_TESTING);
            default:
                return QVariant();
            }
        }
        break;
    case 1:
        if (role == Qt::DisplayRole) {
             POJProblemCase problemCase = mProblem->cases()[index.row()];
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
             POJProblemCase problemCase = mProblem->cases()[index.row()];
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
            mProblem->cases()[index.row()]->setName(s);
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
        mMoveTargetRow=mProblem->cases().count();
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
        || sourceRow + count - 1 >= mProblem->cases().count()
        || destinationChild < 0
        || destinationChild > mProblem->cases().count()
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
        mProblem->moveCase(fromRow, destinationChild);
    endMoveRows();
    return true;
}

