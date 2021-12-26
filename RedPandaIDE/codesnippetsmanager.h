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
#ifndef CODESNIPPETSMANAGER_H
#define CODESNIPPETSMANAGER_H

#include <QObject>
#include "parser/parserutils.h"
#include <QAbstractListModel>

class CodeSnippetsModel: public QAbstractListModel {
    Q_OBJECT
public:
    void addSnippet(
            const QString& caption,
            const QString& prefix,
            const QString& code,
            const QString& description,
            int menuSection);
    void remove(int index);
    void clear();
    QModelIndex lastSnippetCaption();

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    const QList<PCodeSnippet> &snippets() const;
    void updateSnippets(const QList<PCodeSnippet>& snippets);

private:
    QList<PCodeSnippet> mSnippets;
};

class CodeSnippetsManager : public QObject
{
    Q_OBJECT
public:
    explicit CodeSnippetsManager(QObject *parent = nullptr);

    void load();
    void save();

    const QList<PCodeSnippet> &snippets() const;

    void setSnippets(const QList<PCodeSnippet> &newSnippets);

    const QString &newFileTemplate() const;
    void setNewFileTemplate(const QString &newNewFileTemplate);

private:
    void loadSnippets();
    void saveSnippets();
    void loadNewFileTemplate();
    void saveNewFileTemplate();

private:
    QList<PCodeSnippet> mSnippets;
    QString mNewFileTemplate;
};

using PCodeSnippetManager = std::shared_ptr<CodeSnippetsManager>;

#endif // CODESNIPPETSMANAGER_H
