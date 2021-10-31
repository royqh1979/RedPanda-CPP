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
    ProblemCaseTestState testState;
};

using POJProblemCase = std::shared_ptr<OJProblemCase>;

struct OJProblem {
    QString name;
    QVector<POJProblemCase> cases;
};

using POJProbelm = std::shared_ptr<OJProblem>;

class OJProblemCaseModel: public QAbstractListModel {
    Q_OBJECT
public:
    explicit OJProblemCaseModel(QObject *parent = nullptr);
    const POJProbelm &problem() const;
    void setProblem(const POJProbelm &newProblem);

private:
    POJProbelm mProblem;

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
};

#endif // OJPROBLEMSETMODEL_H
