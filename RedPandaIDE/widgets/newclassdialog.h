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
#ifndef NEWCLASSDIALOG_H
#define NEWCLASSDIALOG_H

#include <QDialog>
#include "../parser/cppparser.h"
#include <QAbstractListModel>

namespace Ui {
class NewClassDialog;
}

class NewClassCandidatesModel: public QAbstractListModel {
    Q_OBJECT
public:
    explicit NewClassCandidatesModel(PCppParser parser);
    PStatement getCandidate(int row) const;
private:
    void fillClasses();
    void fillClassesInNamespace(PStatement ns);
private:
    PCppParser mParser;
    QVector<PStatement> mCandidates;
    QSet<QString> mClassNames;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};

class NewClassDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewClassDialog(PCppParser parser, QWidget *parent = nullptr);
    ~NewClassDialog();

    QString className() const;
    PStatement baseClass() const;
    QString headerName() const;
    QString sourceName() const;
    QString path() const;
    void setPath(const QString& location);

private slots:
    void on_btnCancel_clicked();

    void on_btnCreate_clicked();

    void on_btnBrowsePath_clicked();

    void on_txtClassName_textChanged(const QString &arg1);

private:
    Ui::NewClassDialog *ui;
    QList<PStatement> mClasses;
    NewClassCandidatesModel mModel;
private:
    void onUpdateIcons();

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // NEWCLASSDIALOG_H
