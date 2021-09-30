#include "editorsnippetwidget.h"
#include "ui_editorsnippetwidget.h"
#include "../mainwindow.h"
#include "../codesnippetsmanager.h"

#include <QItemSelectionModel>

EditorSnippetWidget::EditorSnippetWidget(const QString& name, const QString& group,
                                         QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorSnippetWidget)
{
    mUpdatingCode = false;
    ui->setupUi(this);
    ui->tblSnippets->setModel(&mModel);
    connect(ui->editCode, &Editor::changed,
            [this] {
        if (mUpdatingCode)
            return;
        QModelIndex index = ui->tblSnippets->currentIndex();
        if (!index.isValid())
            return;
        PCodeSnippet snippet = mModel.snippets()[index.row()];
        snippet->code = ui->editCode->lines()->text();
        setSettingsChanged();
    });
    connect(ui->tblSnippets->selectionModel(), &QItemSelectionModel::currentChanged,
            [this] {
        QModelIndex index = ui->tblSnippets->currentIndex();
        if (!index.isValid()) {
            ui->editCode->setEnabled(false);
            ui->editCode->lines()->clear();
        } else {
            mUpdatingCode = true;
            ui->editCode->setEnabled(true);
            PCodeSnippet snippet = mModel.snippets()[index.row()];
            ui->editCode->lines()->setText(snippet->code);
            mUpdatingCode = false;
        }
    });
}

EditorSnippetWidget::~EditorSnippetWidget()
{
    delete ui;
}

void EditorSnippetWidget::doLoad()
{
    mModel.updateSnippets(pMainWindow->codeSnippetManager()->snippets());
}

void EditorSnippetWidget::doSave()
{
    pMainWindow->codeSnippetManager()->setSnippets(mModel.snippets());
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


void EditorSnippetWidget::on_btnRemove_clicked()
{
    QModelIndex index = ui->tblSnippets->currentIndex();
    if (!index.isValid())
        return;
    mModel.remove(index.row());
}
