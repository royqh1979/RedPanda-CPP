#ifndef OJPROBLEMSETMODEL_H
#define OJPROBLEMSETMODEL_H

#include <QAbstractListModel>
#include <memory>
#include "../problems/ojproblemset.h"

class OJProblemModel: public QAbstractListModel {
    Q_OBJECT
public:
    explicit OJProblemModel(QObject *parent = nullptr);
    const POJProblem &problem() const;
    void setProblem(const POJProblem &newProblem);
    void addCase(POJProblemCase problemCase);
    void removeCase(int index);
    POJProblemCase getCase(int index);
    POJProblemCase getCaseById(const QString& id);
    int getCaseIndexById(const QString& id);
    void clear();
    int count();
    void update(int row);
    QString getTitle();
    QString getTooltip();

private:
    POJProblem mProblem;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
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
signals:
    void problemNameChanged(int index);

private:
    OJProblemSet mProblemSet;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
};

#endif // OJPROBLEMSETMODEL_H
