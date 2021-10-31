#ifndef OJPROBLEMSETMODEL_H
#define OJPROBLEMSETMODEL_H

#include <QAbstractListModel>
#include <memory>

enum class ProblemCaseTestState {
    NoTested,
    Passed,
    Failed
};

struct OJProblemCase {
    QString name;
    QString input;
    QString expected;

    QString output; // no persistence
    ProblemCaseTestState testState; // no persistence
};

using POJProblemCase = std::shared_ptr<OJProblemCase>;

struct OJProblem {
    QString name;
    QVector<POJProblemCase> cases;
};

using POJProblem = std::shared_ptr<OJProblem>;

struct OJProblemSet {
    QString name;
    QVector<POJProblem> problems;
};

class OJProblemModel: public QAbstractListModel {
    Q_OBJECT
public:
    explicit OJProblemModel(QObject *parent = nullptr);
    const POJProblem &problem() const;
    void setProblem(const POJProblem &newProblem);
    void addCase(POJProblemCase problemCase);
    void removeCase(int index);
    POJProblemCase getCase(int index);
    void clear();
    int count();

private:
    POJProblem mProblem;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};

class OJProblemSetModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit OJProblemSetModel(QObject *parent = nullptr);
    void clear();
    int count();
    void create(const QString& name);
    void rename(const QString& newName);
    QString name();
    void addProblem(POJProblem problem);
    POJProblem problem(int index);
    void removeProblem(int index);
    bool problemNameUsed(const QString& name);
    void removeAllProblems();
    void saveToFile(const QString& fileName);
    void loadFromFile(const QString& fileName);
private:
    OJProblemSet mProblemSet;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};

#endif // OJPROBLEMSETMODEL_H
