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
#include "issuestable.h"
#include "../utils.h"
#include <QHeaderView>
#include "../settings.h"
#include "../mainwindow.h"
#include "../editor.h"
#include "../editorlist.h"


IssuesTable::IssuesTable(QWidget *parent):
    QTableView(parent)
{
    mModel = new IssuesModel(this);
    QItemSelectionModel *m=this->selectionModel();
    this->setModel(mModel);
    delete m;
    this->setColumnWidth(0,200);
    this->setColumnWidth(1,45);
    this->setColumnWidth(2,45);
}

const QVector<PCompileIssue> &IssuesTable::issues() const
{
    return mModel->issues();
}

IssuesModel *IssuesTable::issuesModel()
{
    return mModel;
}

void IssuesTable::setErrorColor(QColor color)
{
    mModel->setErrorColor(color);
}

void IssuesTable::setWarningColor(QColor color)
{
    mModel->setWarningColor(color);
}

QString IssuesTable::toHtml()
{
    QString result;
    result.append(
                QString("<table><thead><th>%1</th><th>%2</th><th>%3</th><th>%4</th></thead>")
                .arg(tr("Filename"))
                .arg(tr("Line"))
                .arg(tr("Col"))
                .arg(tr("Description")));
    foreach (const PCompileIssue& issue, mModel->issues()) {
        result.append(QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>\n")
                      .arg(issue->filename)
                      .arg(issue->line)
                      .arg(issue->column)
                      .arg(issue->description));
    }
    result.append(QString("</table>"));
    return result;
}

QString IssuesTable::toTxt()
{
    QString result;
    foreach (const PCompileIssue& issue, mModel->issues()) {
        result.append(QString("%1\t%2\t%3\t%4\n")
                      .arg(issue->filename)
                      .arg(issue->line)
                      .arg(issue->column)
                      .arg(issue->description));
    }
    return result;
}

IssuesModel::IssuesModel(QObject *parent):
    QAbstractTableModel(parent)
{

}

void IssuesModel::addIssue(PCompileIssue issue)
{
    beginInsertRows(QModelIndex(),mIssues.size(),mIssues.size());
    mIssues.push_back(issue);
    endInsertRows();
}

void IssuesModel::clearIssues()
{
    QSet<QString> issueFiles;
    foreach(const PCompileIssue& issue, mIssues) {
        if (!(issue->filename.isEmpty())){
            issueFiles.insert(issue->filename);
        }
    }
    foreach (const QString& filename, issueFiles) {
        Editor *e=pMainWindow->editorList()->getOpenedEditorByFilename(filename);
        if (e)
            e->clearSyntaxIssues();
    }
    if (mIssues.size()>0) {
        beginResetModel();
        mIssues.clear();
        endResetModel();
    }
}

void IssuesModel::setErrorColor(QColor color)
{
    mErrorColor = color;
}

void IssuesModel::setWarningColor(QColor color)
{
    mWarningColor = color;
}

PCompileIssue IssuesModel::issue(int row)
{
    if (row<0 || row>=static_cast<int>(mIssues.size())) {
        return PCompileIssue();
    }

    return mIssues[row];
}

const QVector<PCompileIssue> &IssuesModel::issues() const
{
    return mIssues;
}

int IssuesModel::count()
{
    return mIssues.size();
}

void IssuesTable::addIssue(PCompileIssue issue)
{
    mModel->addIssue(issue);
}

PCompileIssue IssuesTable::issue(const QModelIndex &index)
{
    if (!index.isValid())
        return PCompileIssue();
    return issue(index.row());
}

PCompileIssue IssuesTable::issue(const int row)
{
    return mModel->issue(row);
}

int IssuesTable::count()
{
    return mModel->count();
}

void IssuesTable::clearIssues()
{
    mModel->clearIssues();
}

int IssuesModel::rowCount(const QModelIndex &) const
{
    return mIssues.size();
}

int IssuesModel::columnCount(const QModelIndex &) const
{
    return 4;
}

QVariant IssuesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()<0 || index.row() >= static_cast<int>(mIssues.size()))
        return QVariant();
    PCompileIssue issue = mIssues[index.row()];
    if (!issue)
        return QVariant();
    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        switch (index.column()) {
        case 0: {
            if (role == Qt::DisplayRole)
                return extractFileName(issue->filename);
            else
                return issue->filename;
        }
        case 1:
            if (issue->line>0)
                return issue->line;
            else
                return "";
        case 2:
            if (issue->column>0)
                return issue->column;
            else
                return "";
        case 3:
            return issue->description;
        default:
            return QVariant();
        }
    case Qt::ForegroundRole:
        switch(issue->type) {
        case CompileIssueType::Error:
            return mErrorColor;
        case CompileIssueType::Warning:
            return mWarningColor;
        default:
            return QVariant();
        }
    case Qt::FontRole: {
        QFont newFont=((IssuesTable *)parent())->font();
        switch(issue->type) {
        case CompileIssueType::Error:
        case CompileIssueType::Warning:
        {
            newFont.setBold(true);
            break;
        }
        default:
            newFont.setBold(issue->line == 0);
        }
        return newFont;
    }
    default:
        return QVariant();
    }
}

QVariant IssuesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal ) {
        switch(role) {
        case Qt::DisplayRole:
            switch(section) {
            case 0:
                return tr("Filename");
            case 1:
                return tr("Line");
            case 2:
                return tr("Col");
            case 3:
                return tr("Description");
            }
            break;
        }
    }
    return QVariant();
}
