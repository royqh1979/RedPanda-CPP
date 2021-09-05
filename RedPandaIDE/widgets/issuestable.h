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
    PCompileIssue issue(int row);
private:
    QVector<PCompileIssue> mIssues;
    QColor mErrorColor;
    QColor mWarningColor;

    // QAbstractItemModel interface
public:
    int count();
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    const QVector<PCompileIssue> &issues() const;
};

class IssuesTable : public QTableView
{
    Q_OBJECT
public:

    explicit IssuesTable(QWidget* parent = nullptr);

    const QVector<PCompileIssue> & issues() const;

    IssuesModel* issuesModel();

    void setErrorColor(QColor color);
    void setWarningColor(QColor color);
    QString toHtml();
    QString toTxt();

public slots:
    void addIssue(PCompileIssue issue);

    PCompileIssue issue(const QModelIndex& index);
    PCompileIssue issue(const int row);
    int count();

    void clearIssues();

private:
    IssuesModel * mModel;
};

#endif // ISSUESTABLE_H
