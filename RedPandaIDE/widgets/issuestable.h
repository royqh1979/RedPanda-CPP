#ifndef ISSUESTABLE_H
#define ISSUESTABLE_H

#include <QTableView>
#include <vector>
#include "../common.h"
#include <QAbstractTableModel>

class IssuesModel : public QAbstractTableModel {

    Q_OBJECT
public:
    explicit IssuesModel(QObject *parent = nullptr);

public slots:
    void addIssue(PCompileIssue issue);
    void clearIssues();

    void setErrorColor(QColor color);
    void setWarningColor(QColor color);
private:
    std::vector<PCompileIssue> mIssues;
    QColor mErrorColor;
    QColor mWarningColor;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
};

class IssuesTable : public QTableView
{
    Q_OBJECT
public:

    explicit IssuesTable(QWidget* parent = nullptr);

    const std::vector<PCompileIssue> & issues() const;

    IssuesModel* issuesModel();

    void setErrorColor(QColor color);
    void setWarningColor(QColor color);

public slots:
    void addIssue(PCompileIssue issue);

    void clearIssues();

private:
    IssuesModel * mModel;
};

#endif // ISSUESTABLE_H
