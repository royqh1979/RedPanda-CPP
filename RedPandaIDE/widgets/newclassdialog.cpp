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
#include "newclassdialog.h"
#include "ui_newclassdialog.h"
#include "../iconsmanager.h"
#include "../settings.h"
#include <QFileDialog>
#include <algorithm>

NewClassDialog::NewClassDialog(PCppParser parser, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewClassDialog),
    mModel(parser)
{
    setWindowFlag(Qt::WindowContextHelpButtonHint,false);
    ui->setupUi(this);
    resize(pSettings->ui().newClassDialogWidth(),pSettings->ui().newClassDialogHeight());
    onUpdateIcons();
    connect(pIconsManager,&IconsManager::actionIconsUpdated,
            this, &NewClassDialog::onUpdateIcons);
    ui->txtClassName->setFocus();
    ui->cbBaseClass->setModel(&mModel);
}

NewClassDialog::~NewClassDialog()
{
}

QString NewClassDialog::className() const
{
    return ui->txtClassName->text();
}

PStatement NewClassDialog::baseClass() const
{
    return mModel.getCandidate(ui->cbBaseClass->currentIndex());
}

QString NewClassDialog::headerName() const
{
    return ui->txtHeaderName->text();
}

QString NewClassDialog::sourceName() const
{
    return ui->txtSourceName->text();
}

QString NewClassDialog::path() const
{
    return ui->txtPath->text();
}

void NewClassDialog::setPath(const QString &location)
{
    ui->txtPath->setText(location);
}

void NewClassDialog::on_btnCancel_clicked()
{
    this->reject();
}

void NewClassDialog::closeEvent(QCloseEvent */*event*/)
{
    this->reject();
}


void NewClassDialog::on_btnCreate_clicked()
{
    this->accept();
}

void NewClassDialog::onUpdateIcons()
{
    pIconsManager->setIcon(ui->btnBrowsePath, IconsManager::ACTION_FILE_OPEN_FOLDER);
}


void NewClassDialog::on_btnBrowsePath_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(
                this,
                tr("Path"),
                ui->txtPath->text());
    ui->txtPath->setText(fileName);
}


void NewClassDialog::on_txtClassName_textChanged(const QString &/* arg1 */)
{
    ui->txtHeaderName->setText(ui->txtClassName->text().toLower()+".h");
    ui->txtSourceName->setText(ui->txtClassName->text().toLower()+".cpp");
}

NewClassCandidatesModel::NewClassCandidatesModel(PCppParser parser):QAbstractListModel(),
    mParser(parser)
{
    fillClasses();
}

PStatement NewClassCandidatesModel::getCandidate(int row) const
{
    if (row<0)
        return PStatement();
    if (row==0)
        return PStatement();
    return mCandidates[row-1];
}


void NewClassCandidatesModel::fillClasses()
{
    if (!mParser->enabled())
        return;
    if (!mParser->freeze())
        return;
    foreach( const PStatement& s, mParser->statementList().childrenStatements()) {
        if (s->kind==StatementKind::Class
                && s->inProject()
                && !s->command.startsWith("_")
                && !s->command.contains("<")
                && !mClassNames.contains(s->fullName)) {
            if (getFileType(s->fileName)==FileType::CHeader
                    || getFileType(s->fileName)==FileType::CppHeader) {
                mCandidates.append(s);
                mClassNames.insert(s->fullName);
            }
        } else if (s->kind == StatementKind::Namespace
                   && !s->command.startsWith("_")
                   && !s->command.contains("<")) {
            fillClassesInNamespace(s);
        }
    }
    mParser->unFreeze();
    std::sort(mCandidates.begin(),mCandidates.end(),[](const PStatement& s1, const PStatement& s2){
        return s1->fullName<s2->fullName;
    });

}

void NewClassCandidatesModel::fillClassesInNamespace(PStatement ns)
{
    foreach( const PStatement& s, mParser->statementList().childrenStatements(ns)) {
        if (s->kind==StatementKind::Class
                && s->inProject()
                && !s->command.startsWith("_")
                && !s->command.contains("<")
                && !mClassNames.contains(s->fullName)) {
            if (getFileType(s->fileName)==FileType::CHeader
                    || getFileType(s->fileName)==FileType::CppHeader) {
                mCandidates.append(s);
                mClassNames.insert(s->fullName);
            }
        } else if (s->kind == StatementKind::Namespace
                   && !s->command.startsWith("_")
                   && !s->command.contains("<")) {
            fillClassesInNamespace(s);
        }
    }
}

int NewClassCandidatesModel::rowCount(const QModelIndex &/*parent*/) const
{
    return mCandidates.count()+1;
}

QVariant NewClassCandidatesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role==Qt::DisplayRole) {
        if (index.row()==0)
            return "";
        return mCandidates[index.row()-1]->fullName;
    }
    return QVariant();
}
