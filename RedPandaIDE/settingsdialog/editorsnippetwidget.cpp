#include "editorsnippetwidget.h"
#include "ui_editorsnippetwidget.h"
#include "../mainwindow.h"
#include "../codesnippetsmanager.h"

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

void EditorSnippetWidget::on_tblSnippets_clicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    mUpdatingCode = true;
    PCodeSnippet snippet = mModel.snippets()[index.row()];
    ui->editCode->lines()->setText(snippet->code);
    mUpdatingCode = false;
}


void EditorSnippetWidget::on_btnAdd_triggered(QAction *arg1)
{
    mModel.addSnippet(QString("Code %1").arg(getNewFileNumber()),
                      "",
                      "",
                      "",
                      -1);
}


void EditorSnippetWidget::on_btnRemove_triggered(QAction *arg1)
{
    QModelIndex index = ui->tblSnippets->currentIndex();
    if (!index.isValid())
        return;
    mModel.remove(index.row());
}

