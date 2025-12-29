#include "searchdialog.h"
#include "ui_searchdialog.h"

#include <QCompleter>
#include <QMessageBox>
#include <memory>
#include <qsynedit/searcher/basicsearcher.h>
#include <qsynedit/searcher/regexsearcher.h>
#include "../utils.h"
#include "../editor.h"
#include "../editormanager.h"
#include "../mainwindow.h"

SearchDialog::SearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchDialog)
{
    setWindowFlag(Qt::WindowContextHelpButtonHint,false);
    ui->setupUi(this);
    mTabBar=new QTabBar(this);
    mTabBar->setExpanding(false);
    mSearchTabIdx = mTabBar->addTab(tr("Search"));
    mReplaceTabIdx = mTabBar->addTab(tr("Replace"));
    ui->dialogLayout->insertWidget(0,mTabBar);
    ui->dialogLayout->setContentsMargins(10, 10, 10, 10);

    mTabBar->setCurrentIndex(mSearchTabIdx);
    connect(mTabBar, &QTabBar::currentChanged, this,
            &SearchDialog::onTabBarCurrentChanged);
    onTabBarCurrentChanged(mSearchTabIdx);
    mBasicSearchEngine = std::make_shared<QSynedit::BasicSearcher>();
    mRegexSearchEngine = std::make_shared<QSynedit::RegexSearcher>();
    ui->cbFind->completer()->setCaseSensitivity(Qt::CaseSensitive);
    ui->cbReplace->completer()->setCaseSensitivity(Qt::CaseSensitive);

    connect(ui->rbGlobal, &QRadioButton::toggled, this, &SearchDialog::setOriginVisibility);
    connect(ui->rbSelection, &QRadioButton::toggled, this, &SearchDialog::setOriginVisibility);
}

SearchDialog::~SearchDialog()
{
    delete ui;
}

void SearchDialog::find(const QString &text)
{
    //Call show() first, to ensure cbFind has the default width.
    show();

    mTabBar->setCurrentIndex(mSearchTabIdx);
    setComboTextAndHistory(ui->cbFind,text,mSearchKeys);
    ui->cbFind->setFocus();
    activateWindow();
}

void SearchDialog::replace(const QString &text)
{
    //Call show() first, to ensure cbFind has the default width.
    show();

    mTabBar->setCurrentIndex(mReplaceTabIdx);
    setComboTextAndHistory(ui->cbFind,text,mSearchKeys);
    setComboTextAndHistory(ui->cbReplace,text,mReplaceKeys);
    ui->cbFind->setFocus();
    activateWindow();
}

void SearchDialog::findNext()
{
    doSearch(false);
    if (ui->chkCloseAfterSearch->isChecked())
        close();
}

void SearchDialog::findPrevious()
{
    doSearch(true);
    if (ui->chkCloseAfterSearch->isChecked())
        close();
}

void SearchDialog::doSearch(bool backward)
{
    if (ui->cbFind->currentText().isEmpty())
        return;
    updateComboHistory(mSearchKeys, ui->cbFind->currentText());
    bool regex, searchInSelection;
    QSynedit::SearchOptions searchOptions = prepareOptions(false, regex, searchInSelection);

    Editor *editor = pMainWindow->editorManager()->getEditor();
    if (editor) {
        QSynedit::CharPos searchBegin, searchEnd, newSearchEnd;
        if (searchInSelection) {
            if (editor->selAvail()) {
                searchBegin = editor->selBegin();
                searchEnd = editor->selEnd();
            } else
                return;
        } else {
            searchBegin = editor->fileBegin();
            searchEnd = editor->fileEnd();
        }
        QSynedit::PSearcher searchEngine;
        if (regex) {
            searchEngine = mRegexSearchEngine;
        } else {
            searchEngine = mBasicSearchEngine;
        }
        int foundCount = editor->searchReplace(
                    ui->cbFind->currentText(),
                    "",
                    searchBegin,
                    searchEnd,
                    newSearchEnd,
                    searchOptions,
                    searchEngine.get(), nullptr, [this,backward](){
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
        if (foundCount==0) {
            QWidget *p;
            if (isVisible()) {
                p=this;
            } else {
                p=pMainWindow;
            }
            QMessageBox::information(
                p,
                tr("Not Found"),
                tr("Can't find '%1'").arg(ui->cbFind->currentText()));
        } else {
            if (ui->rbEntireScope->isChecked()) {
                ui->rbFromCaret->setChecked(true);
            }
        }
    }
}

void SearchDialog::doReplace(bool replaceAll)
{
    updateComboHistory(mSearchKeys,ui->cbFind->currentText());
    updateComboHistory(mReplaceKeys,ui->cbReplace->currentText());
    if (ui->rbEntireScope->isChecked()) {
        ui->rbFromCaret->setChecked(true);
    }
    bool regex, searchInSelection;
    QSynedit::SearchOptions searchOptions = prepareOptions(false, regex, searchInSelection);
    searchOptions.setFlag(QSynedit::SearchOption::ssoIncludeCurrentSelection);

    Editor *editor = pMainWindow->editorManager()->getEditor();
    if (editor) {
        QSynedit::CharPos searchBegin, searchEnd, newSearchEnd;
        if (searchInSelection) {
            if (editor->selAvail()) {
                searchBegin = editor->selBegin();
                searchEnd = editor->selEnd();
            } else
                return;
        } else {
            searchBegin = editor->fileBegin();
            searchEnd = editor->fileEnd();
        }
        QSynedit::PSearcher searchEngine;
        if (regex) {
            searchEngine = mRegexSearchEngine;
        } else {
            searchEngine = mBasicSearchEngine;
        }
        editor->searchReplace(
                    ui->cbFind->currentText(),
                    ui->cbReplace->currentText(),
                    searchBegin,
                    searchEnd,
                    newSearchEnd,
                    searchOptions,
                    searchEngine.get(),
                    [&replaceAll](const QString& /*sFound*/,
                    const QString& /*sReplace*/, const QSynedit::CharPos &, int /*wordLen*/){
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

QSynedit::SearchOptions SearchDialog::prepareOptions(bool backward,bool &regex, bool &searchInSelection)
{
    QSynedit::SearchOptions searchOptions= QSynedit::ssoNone;

    // Apply options
    if (backward) {
        searchOptions.setFlag(QSynedit::ssoBackwards);
    }

    regex = ui->chkRegExp->isChecked();
    if (ui->chkCaseSensetive->isChecked()) {
        searchOptions.setFlag(QSynedit::ssoMatchCase);
    }
    if (ui->chkWholeWord->isChecked()) {
        searchOptions.setFlag(QSynedit::ssoWholeWord);
    }
    if (ui->chkWrapAround->isChecked()) {
        searchOptions.setFlag(QSynedit::ssoWrapAround);
    }

    // Apply scope, when enabled
    searchInSelection = (ui->grpScope->isEnabled())
            &&  (ui->rbSelection->isChecked());

    // Apply origin, when enabled
    if (ui->grpOrigin->isEnabled()) {
        if (ui->rbFromCaret->isChecked()) {
            searchOptions.setFlag(QSynedit::ssoFromCaret);
        }
    }
    return searchOptions;
}

void SearchDialog::setOriginVisibility()
{
    ui->grpOrigin->setEnabled(ui->rbGlobal->isChecked());
}



void SearchDialog::onTabBarCurrentChanged(int currentIndex)
{
    if(currentIndex==mSearchTabIdx) {
        ui->lbReplace->setVisible(false);
        ui->cbReplace->setVisible(false);
        ui->chkCloseAfterSearch->setVisible(true);
        ui->btnReplace->setVisible(false);
        ui->btnReplaceAll->setVisible(false);
    } else {
        ui->lbReplace->setVisible(true);
        ui->cbReplace->setVisible(true);
        ui->chkCloseAfterSearch->setVisible(false);
        ui->btnReplace->setVisible(true);
        ui->btnReplaceAll->setVisible(true);
    }
}

void SearchDialog::on_cbFind_currentTextChanged(const QString &value)
{
    ui->btnNext->setEnabled(!value.isEmpty());
    ui->btnPrevious->setEnabled(!value.isEmpty());
    ui->btnReplace->setEnabled(!value.isEmpty());
    ui->btnReplaceAll->setEnabled(!value.isEmpty());
}

void SearchDialog::on_btnClose_clicked()
{
    close();
}

void SearchDialog::on_btnNext_clicked()
{
    findNext();
}

void SearchDialog::on_btnPrevious_clicked()
{
    findPrevious();
}


void SearchDialog::on_btnReplace_clicked()
{
    doReplace(false);
}


void SearchDialog::on_btnReplaceAll_clicked()
{
    doReplace(true);
    close();
}
