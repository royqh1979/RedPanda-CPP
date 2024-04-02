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
#ifndef OJPROBLEMSETMODEL_H
#define OJPROBLEMSETMODEL_H

#include <QAbstractTableModel>
#include <memory>
#include "../problems/ojproblemset.h"

class OJProblemModel: public QAbstractTableModel {
    Q_OBJECT
public:
    explicit OJProblemModel(QObject *parent = nullptr);
    const POJProblem &problem() const;
    void setProblem(const POJProblem &newProblem);
    void addCase(POJProblemCase problemCase);
    void removeCase(int index);
    void removeCases();
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
    int mMoveTargetRow;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    // QAbstractItemModel interface
public:
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // QAbstractItemModel interface
public:
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    bool insertRows(int row, int count, const QModelIndex &parent) override;
    bool removeRows(int row, int count, const QModelIndex &parent) override;
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
    QString name() const;
    QString exportFilename() const;
    void addProblem(const POJProblem& problem);
    void addProblems(const QList<POJProblem> &problems);
    const QList<POJProblem> &problems() const;
    POJProblem problem(int index) const;
    void removeProblem(int index);
    bool problemNameUsed(const QString& name);
    void removeAllProblems();
    void saveToFile(const QString& fileName, int currentIndex=-1);
    void loadFromFile(const QString& fileName, int& currentIndex);
    void load(int& currentIndex);
    void save(int currentIndex);
    void updateProblemAnswerFilename(const QString& oldFilename, const QString& newFilename);

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

    // QAbstractItemModel interface
public:
    Qt::DropActions supportedDropActions() const override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;
};

#endif // OJPROBLEMSETMODEL_H
