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
#include <qsynedit/document.h>
#include "editorsnippetwidget.h"
#include "ui_editorsnippetwidget.h"
#include "../mainwindow.h"
#include "../codesnippetsmanager.h"
#include "../iconsmanager.h"
#include "../syntaxermanager.h"

#include <QItemSelectionModel>

EditorSnippetWidget::EditorSnippetWidget(const QString& name, const QString& group,
                                         QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorSnippetWidget)
{
    mUpdatingCode = false;
    ui->setupUi(this);
    QItemSelectionModel* m=ui->tblSnippets->selectionModel();
    ui->tblSnippets->setModel(&mModel);
    delete m;
    connect(ui->editCode, &Editor::changed,
            [this] {
        if (mUpdatingCode)
            return;
        QModelIndex index = ui->tblSnippets->currentIndex();
        if (!index.isValid())
            return;
        PCodeSnippet snippet = mModel.snippets()[index.row()];
        snippet->code = ui->editCode->text();
        setSettingsChanged();
    });
    connect(ui->tblSnippets->selectionModel(), &QItemSelectionModel::currentChanged,
            [this] {
        QModelIndex index = ui->tblSnippets->currentIndex();
        if (!index.isValid()) {
            ui->editCode->setEnabled(false);
            ui->editCode->document()->clear();
        } else {
            mUpdatingCode = true;
            ui->editCode->setEnabled(true);
            PCodeSnippet snippet = mModel.snippets()[index.row()];
            ui->editCode->document()->setText(snippet->code);
            mUpdatingCode = false;
        }
    });
    connect(ui->editCppFileTemplate,&Editor::changed,
            this, &SettingsWidget::setSettingsChanged);
    connect(ui->editCFileTemplate,&Editor::changed,
            this, &SettingsWidget::setSettingsChanged);
    connect(ui->editGASFileTemplate,&Editor::changed,
            this, &SettingsWidget::setSettingsChanged);
    ui->editGASFileTemplate->setSyntaxer(syntaxerManager.getSyntaxer(QSynedit::ProgrammingLanguage::ATTAssembly));
}

EditorSnippetWidget::~EditorSnippetWidget()
{
    delete ui;
}

void EditorSnippetWidget::doLoad()
{
    mModel.updateSnippets(pMainWindow->codeSnippetManager()->snippets());
    ui->editCppFileTemplate->document()->setText(pMainWindow->codeSnippetManager()->newCppFileTemplate());
    ui->editCFileTemplate->document()->setText(pMainWindow->codeSnippetManager()->newCFileTemplate());
    ui->editGASFileTemplate->document()->setText(pMainWindow->codeSnippetManager()->newGASFileTemplate());
}

void EditorSnippetWidget::doSave()
{
    pMainWindow->codeSnippetManager()->setSnippets(mModel.snippets());
    pMainWindow->codeSnippetManager()->setNewCppFileTemplate(ui->editCppFileTemplate->text());
    pMainWindow->codeSnippetManager()->setNewCFileTemplate(ui->editCFileTemplate->text());
    pMainWindow->codeSnippetManager()->setNewGASFileTemplate(ui->editGASFileTemplate->text());
    pMainWindow->codeSnippetManager()->save();
}

void EditorSnippetWidget::on_btnAdd_clicked()
{
    mModel.addSnippet(QString("").arg(getNewFileNumber()),
                      "",
                      "",
                      "",
                      -1);
    ui->tblSnippets->setCurrentIndex(mModel.lastSnippetCaption());
    ui->tblSnippets->edit(mModel.lastSnippetCaption());
}

void EditorSnippetWidget::updateIcons(const QSize &/*size*/)
{
    pIconsManager->setIcon(ui->btnAdd,IconsManager::ACTION_MISC_ADD);
    pIconsManager->setIcon(ui->btnRemove,IconsManager::ACTION_MISC_REMOVE);
}


void EditorSnippetWidget::on_btnRemove_clicked()
{
    QModelIndex index = ui->tblSnippets->currentIndex();
    if (!index.isValid())
        return;
    mModel.remove(index.row());
}
