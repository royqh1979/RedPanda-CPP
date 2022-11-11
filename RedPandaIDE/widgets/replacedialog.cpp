#include "replacedialog.h"
#include "ui_replacedialog.h"
#include <QMessageBox>
#include <memory>
#include <qsynedit/Search.h>
#include <qsynedit/SearchRegex.h>
#include "../utils.h"
#include "../editor.h"
#include "../editorlist.h"
#include "../mainwindow.h"

ReplaceDialog::ReplaceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReplaceDialog),
    mSearchOptions()
{
    ui->setupUi(this);
    mBasicSearchEngine= std::make_shared<QSynedit::BasicSearcher>();
    mRegexSearchEngine= std::make_shared<QSynedit::RegexSearcher>();
}

ReplaceDialog::~ReplaceDialog()
{
    delete ui;
}

void ReplaceDialog::replace(const QString &text)
{
    ui->cbFind->setCurrentText(text);
    ui->cbFind->setFocus();
    //ui->cbReplace->setCurrentText("");
    show();
}

void ReplaceDialog::doSearch(bool backward)
{
    saveComboHistory(ui->cbFind,ui->cbFind->currentText());
    prepareOptions(backward);

    Editor *editor = pMainWindow->editorList()->getEditor();
    if (editor) {
        // Modify the caret when using 'from cursor' and when the selection is ignored
        if (!mSearchOptions.testFlag(QSynedit::ssoEntireScope) && !mSearchOptions.testFlag(QSynedit::ssoSelectedOnly)
                && editor->selAvail()) {
            // start at end of selection
            if (mSearchOptions.testFlag(QSynedit::ssoBackwards)) {
                editor->setCaretXY(editor->blockBegin());
            } else {
                editor->setCaretXY(editor->blockEnd());
            }
        }
        QSynedit::PSynSearchBase searchEngine;
        if (mSearchOptions.testFlag(QSynedit::ssoRegExp)) {
            searchEngine = mRegexSearchEngine;
        } else {
            searchEngine = mBasicSearchEngine;
        }
        editor->searchReplace(
                    ui->cbFind->currentText(),
                    "",
                    mSearchOptions,
                    searchEngine, nullptr, [this,backward](){
                        QString msg;
                        if (backward) {
                            msg = tr("Beginning of file has been reached. ")
                                    +tr("Do you want to continue from file's end?");
                        } else {
                            msg = tr("End of file has been reached. ")
                                    +tr("Do you want to continue from file's beginning?");
                        }
                        QWidget *p;
                        if (isVisible()) {
                            p=this;
                        } else {
                            p=pMainWindow;
                        }
                        return QMessageBox::question(p,
                          tr("Continue Search"),
                          msg,
                          QMessageBox::Yes|QMessageBox::No,
                          QMessageBox::Yes) == QMessageBox::Yes;
        });
    }
}

void ReplaceDialog::doReplace(bool replaceAll)
{
    saveComboHistory(ui->cbFind,ui->cbFind->currentText());
    saveComboHistory(ui->cbReplace,ui->cbReplace->currentText());
    prepareOptions(false);
    Editor *editor = pMainWindow->editorList()->getEditor();
    if (editor) {
//        // Modify the caret when using 'from cursor' and when the selection is ignored
//        if (!mSearchOptions.testFlag(QSynedit::ssoEntireScope) && !mSearchOptions.testFlag(QSynedit::ssoSelectedOnly)
//                && editor->selAvail()) {
//            // start at end of selection
//            if (mSearchOptions.testFlag(QSynedit::ssoBackwards)) {
//                editor->setCaretXY(editor->blockBegin());
//            } else {
//                editor->setCaretXY(editor->blockEnd());
//            }
//        }
        QSynedit::PSynSearchBase searchEngine;
        if (mSearchOptions.testFlag(QSynedit::ssoRegExp)) {
            searchEngine = mRegexSearchEngine;
        } else {
            searchEngine = mBasicSearchEngine;
        }
        editor->searchReplace(
                    ui->cbFind->currentText(),
                    ui->cbReplace->currentText(),
                    mSearchOptions,
                    searchEngine,
                    [&replaceAll](const QString& /*sSearch*/,
                    const QString& /*sReplace*/, int /*Line*/, int /*ch*/, int /*wordLen*/){
                        if (replaceAll) {
                            return QSynedit::SearchAction::ReplaceAll;
                        } else {
                            return QSynedit::SearchAction::ReplaceAndExit;
                        }
                    },
                    [this](){
                        QString msg = tr("End of file has been reached. ")
                                    +tr("Do you want to continue from file's beginning?");
                        QWidget *p;
                        if (isVisible()) {
                            p=this;
                        } else {
                            p=pMainWindow;
                        }
                        return QMessageBox::question(p,
                          tr("Continue Search"),
                          msg,
                          QMessageBox::Yes|QMessageBox::No,
                          QMessageBox::Yes) == QMessageBox::Yes;
        });
    }
}

void ReplaceDialog::prepareOptions(bool backward)
{
    mSearchOptions&=0;

    // Apply options
    if (backward) {
        mSearchOptions.setFlag(QSynedit::ssoBackwards);
    }

    if (ui->chkRegExp->isChecked()) {
        mSearchOptions.setFlag(QSynedit::ssoRegExp);
    }
    if (ui->chkCaseSensetive->isChecked()) {
        mSearchOptions.setFlag(QSynedit::ssoMatchCase);
    }
    if (ui->chkWholeWord->isChecked()) {
        mSearchOptions.setFlag(QSynedit::ssoWholeWord);
    }
    if (ui->chkWrapAround->isChecked()) {
        mSearchOptions.setFlag(QSynedit::ssoWrapAround);
    }

    // Apply scope, when enabled
    if (ui->grpScope->isEnabled()) {
        if (ui->rbSelection->isChecked()) {
            mSearchOptions.setFlag(QSynedit::ssoSelectedOnly);
        }
    }

    // Apply origin, when enabled
    if (ui->grpOrigin->isEnabled()) {
        if (ui->rbEntireScope->isChecked()) {
            mSearchOptions.setFlag(QSynedit::ssoEntireScope);
        }
    }

}

void ReplaceDialog::on_cbFind_currentTextChanged(const QString &value)
{
    ui->btnNext->setEnabled(!value.isEmpty());
    ui->btnPrevious->setEnabled(!value.isEmpty());
    ui->btnReplace->setEnabled(!value.isEmpty());
    ui->btnReplaceAll->setEnabled(!value.isEmpty());
}

void ReplaceDialog::on_btnClose_clicked()
{
    close();
}

void ReplaceDialog::on_btnNext_clicked()
{
    doSearch(false);
}

void ReplaceDialog::on_btnPrevious_clicked()
{
    doSearch(true);
}

void ReplaceDialog::on_btnReplace_clicked()
{
    doReplace(false);
}


void ReplaceDialog::on_btnReplaceAll_clicked()
{
    doReplace(true);
    close();
}

