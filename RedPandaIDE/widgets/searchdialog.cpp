#include "searchdialog.h"
#include "ui_searchdialog.h"
#include <QTabBar>
#include "../editor.h"
#include "../mainwindow.h"
#include "../editorlist.h"
#include "../qsynedit/Search.h"
#include "../qsynedit/SearchRegex.h"
#include "../project.h"
#include <QMessageBox>
#include <QDebug>


SearchDialog::SearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchDialog),
    mSearchEngine()
{
    ui->setupUi(this);
    mTabBar = new QTabBar();
    mTabBar->addTab(tr("Find"));
    mTabBar->addTab(tr("Find in files"));
    mTabBar->addTab(tr("Replace"));
    mTabBar->addTab(tr("Replace in files"));
    mTabBar->setExpanding(false);
    ui->dialogLayout->insertWidget(0,mTabBar);
    connect(mTabBar,&QTabBar::currentChanged,this, &SearchDialog::onTabChanged);
    mSearchOptions&=0;
    mBasicSearchEngine= PSynSearchBase(new SynSearch());
    mRegexSearchEngine= PSynSearchBase(new SynSearchRegex());
}

SearchDialog::~SearchDialog()
{
    delete ui;
}

void SearchDialog::find(const QString &text)
{
    if (mTabBar->currentIndex()==0) {
        this->onTabChanged();
    } else {
        mTabBar->setCurrentIndex(0);
    }
    ui->cbFind->setCurrentText(text);
    show();
}

void SearchDialog::findNext()
{
    if (mTabBar->currentIndex()==0) { // it's a find action

        // Disable entire scope searching
        ui->rbEntireScope->setChecked(false);

        // Always search forwards
        ui->rbForward->setChecked(true);

        ui->btnExecute->click();
    }
}

void SearchDialog::findInFiles(const QString &text)
{
    mTabBar->setCurrentIndex(1);
    ui->cbFind->setCurrentText(text);
    show();
}

void SearchDialog::findInFiles(const QString &keyword, SearchFileScope scope, SynSearchOptions options)
{
    mTabBar->setCurrentIndex(1);

    ui->cbFind->setCurrentText(keyword);
    switch(scope) {
    case SearchFileScope::currentFile:
        ui->rbCurrentFile->setChecked(true);
        break;
    case SearchFileScope::openedFiles:
        ui->rbOpenFiles->setChecked(true);
        break;
    case SearchFileScope::wholeProject:
        ui->rbProject->setChecked(true);
        break;
    }
    // Apply options
    ui->chkRegExp->setChecked(options.testFlag(ssoRegExp));
    ui->chkCaseSensetive->setChecked(options.testFlag(ssoMatchCase));
    ui->chkWholeWord->setChecked(options.testFlag(ssoWholeWord));

    show();
}

void SearchDialog::replace(const QString &sFind, const QString &sReplace)
{
    mTabBar->setCurrentIndex(2);
    ui->cbFind->setCurrentText(sFind);
    ui->cbReplace->setCurrentText(sReplace);
    show();
}

void SearchDialog::onTabChanged()
{
    bool isfind = (mTabBar->currentIndex() == 0);
    bool isfindfiles = (mTabBar->currentIndex() == 1 || mTabBar->currentIndex() == 3 );
    bool isreplace = (mTabBar->currentIndex() == 2);

    ui->lblReplace->setVisible(isreplace);
    ui->cbReplace->setVisible(isreplace);

    ui->grpOrigin->setVisible(isfind || isreplace);
    ui->grpOrigin->setEnabled(isfind || isreplace);

    ui->grpScope->setVisible(isfind || isreplace);
    ui->grpScope->setEnabled(isreplace);
    ui->grpWhere->setVisible(isfindfiles);
    ui->grpWhere->setEnabled(isfindfiles);
    ui->grpDirection->setVisible(isfind || isreplace);
    ui->grpDirection->setEnabled(isfind || isreplace);

    // grpOption is always visible

    // Disable project search option when none is open
//    rbProjectFiles.Enabled := Assigned(MainForm.Project);
    ui->rbProject->setEnabled(pMainWindow->project()!=nullptr);
    ui->rbOpenFiles->setEnabled(pMainWindow->editorList()->pageCount()>0);
//    if not Assigned(MainForm.Project) then
//      rbOpenFiles.Checked := true;

    // Disable prompt when doing finds
    ui->chkPrompt->setEnabled(isreplace);

    if (isfind || isfindfiles) {
        ui->btnExecute->setText(tr("Find"));
    } else {
        ui->btnExecute->setText(tr("Replace"));
    }
    setWindowTitle(mTabBar->tabText(mTabBar->currentIndex()));
}

void SearchDialog::on_cbFind_currentTextChanged(const QString &)
{
    ui->btnExecute->setEnabled(!ui->cbFind->currentText().isEmpty());
}

void SearchDialog::on_btnCancel_clicked()
{
    this->close();
}

static void saveComboHistory(QComboBox* cb,const QString& text) {
    QString s = text.trimmed();
    if (s.isEmpty())
        return;
    int i = cb->findText(s);
    if (i>=0) {
        cb->removeItem(i);
    }
    cb->insertItem(0,s);
    cb->setCurrentText(s);
}

void SearchDialog::on_btnExecute_clicked()
{
    int findCount = 0;
    saveComboHistory(ui->cbFind,ui->cbFind->currentText());
    saveComboHistory(ui->cbReplace,ui->cbReplace->currentText());

    SearchAction actionType;
    switch (mTabBar->currentIndex()) {
    case 0:
        actionType = SearchAction::Find;
        break;
    case 1:
        actionType = SearchAction::FindFiles;
        break;
    case 2:
        actionType = SearchAction::Replace;
        break;
    case 3:
        actionType = SearchAction::ReplaceFiles;
        break;
    default:
        return;
    }

    mSearchOptions&=0;

    // Apply options
    if (ui->chkRegExp->isChecked()) {
        mSearchOptions.setFlag(ssoRegExp);
    }
    if (ui->chkCaseSensetive->isChecked()) {
        mSearchOptions.setFlag(ssoMatchCase);
    }
    if (ui->chkWholeWord->isChecked()) {
        mSearchOptions.setFlag(ssoWholeWord);
    }

    // Apply scope, when enabled
    if (ui->grpScope->isEnabled()) {
        if (ui->rbSelection->isChecked()) {
            mSearchOptions.setFlag(ssoSelectedOnly);
        }
    }

    // Apply direction, when enabled
    if (ui->grpDirection->isEnabled()) {
        if (ui->rbBackward->isChecked()) {
            mSearchOptions.setFlag(ssoBackwards);
        }
    }

    // Apply origin, when enabled
    if (ui->grpOrigin->isEnabled()) {
        if (ui->rbEntireScope->isChecked()) {
            mSearchOptions.setFlag(ssoEntireScope);
        }
    }

    // Use entire scope for file finding/replacing
    if (actionType == SearchAction::FindFiles || actionType == SearchAction::ReplaceFiles) {
        mSearchOptions.setFlag(ssoEntireScope);
    }

    this->close();

    // Find the first one, then quit
    if (actionType == SearchAction::Find) {
        Editor *e = pMainWindow->editorList()->getEditor();
        if (e!=nullptr) {
            findCount+=execute(e,ui->cbFind->currentText(),"");
        }
    } else if (actionType == SearchAction::Replace) {
        Editor *e = pMainWindow->editorList()->getEditor();
        if (e!=nullptr) {
            bool doPrompt = ui->chkPrompt->isChecked();
            findCount+=execute(e,ui->cbFind->currentText(),ui->cbReplace->currentText(),
                               [&doPrompt](const QString& sSearch,
                               const QString& /*sReplace*/, int /*Line*/, int /*ch*/, int /*wordLen*/){
                if (doPrompt) {
                    switch(QMessageBox::question(pMainWindow,
                                          tr("Replace"),
                                          tr("Replace this occurrence of ''%1''?").arg(sSearch),
                                          QMessageBox::Yes|QMessageBox::YesAll|QMessageBox::No|QMessageBox::Cancel,
                                          QMessageBox::Yes)) {
                    case QMessageBox::Yes:
                        return SynSearchAction::Replace;
                    case QMessageBox::YesAll:
                        return SynSearchAction::ReplaceAll;
                    case QMessageBox::No:
                        return SynSearchAction::Skip;
                    case QMessageBox::Cancel:
                        return SynSearchAction::Exit;
                    default:
                        return SynSearchAction::Exit;
                    }
                } else {
                    return SynSearchAction::ReplaceAll;
                }
            });
        }

    } else if (actionType == SearchAction::FindFiles || actionType == SearchAction::ReplaceFiles) {
        int fileSearched = 0;
        int fileHitted = 0;
        QString keyword = ui->cbFind->currentText();
        if (ui->rbOpenFiles->isChecked()) {
            PSearchResults results = pMainWindow->searchResultModel()->addSearchResults(
                        keyword,
                        mSearchOptions,
                        SearchFileScope::openedFiles
                        );
            // loop through editors, add results to message control
            for (int i=0;i<pMainWindow->editorList()->pageCount();i++) {
                Editor * e=pMainWindow->editorList()->operator[](i);
                if (e!=nullptr) {
                    fileSearched++;
                    PSearchResultTreeItem parentItem = batchFindInEditor(
                                e,
                                e->filename(),
                                keyword);
                    int t = parentItem->results.size();
                    findCount+=t;
                    if (t>0) {
                        fileHitted++;
                        results->results.append(parentItem);
                    }
                }
            }
            pMainWindow->searchResultModel()->notifySearchResultsUpdated();
        } else if (ui->rbCurrentFile->isChecked()) {
            PSearchResults results = pMainWindow->searchResultModel()->addSearchResults(
                        keyword,
                        mSearchOptions,
                        SearchFileScope::currentFile
                        );
            Editor * e= pMainWindow->editorList()->getEditor();
            if (e!=nullptr) {
                fileSearched++;
                PSearchResultTreeItem parentItem = batchFindInEditor(
                            e,
                            e->filename(),
                            keyword);
                int t = parentItem->results.size();
                findCount+=t;
                if (t>0) {
                    fileHitted++;
                    results->results.append(parentItem);
                }
            }
            pMainWindow->searchResultModel()->notifySearchResultsUpdated();
        } else if (ui->rbProject->isChecked()) {
            PSearchResults results = pMainWindow->searchResultModel()->addSearchResults(
                        keyword,
                        mSearchOptions,
                        SearchFileScope::wholeProject
                        );
            for (int i=0;i<pMainWindow->project()->units().count();i++) {
                Editor * e = pMainWindow->project()->units()[i]->editor();
                QString curFilename =  pMainWindow->project()->units()[i]->fileName();
                if (e) {
                    fileSearched++;
                    PSearchResultTreeItem parentItem = batchFindInEditor(
                                e,
                                e->filename(),
                                keyword);
                    int t = parentItem->results.size();
                    findCount+=t;
                    if (t>0) {
                        fileHitted++;
                        results->results.append(parentItem);
                    }
                } else if (fileExists(curFilename)) {
                    SynEdit editor;
                    QByteArray realEncoding;
                    editor.lines()->loadFromFile(curFilename,ENCODING_AUTO_DETECT, realEncoding);
                    fileSearched++;
                    PSearchResultTreeItem parentItem = batchFindInEditor(
                                &editor,
                                curFilename,
                                keyword);
                    int t = parentItem->results.size();
                    findCount+=t;
                    if (t>0) {
                        fileHitted++;
                        results->results.append(parentItem);
                    }

                }
            }
            pMainWindow->searchResultModel()->notifySearchResultsUpdated();
        }
        pMainWindow->showSearchPanel(actionType == SearchAction::ReplaceFiles);
    }
}

int SearchDialog::execute(SynEdit *editor, const QString &sSearch, const QString &sReplace, SynSearchMathedProc matchCallback)
{
    if (editor==nullptr)
        return 0;
    // Modify the caret when using 'from cursor' and when the selection is ignored
    if (!mSearchOptions.testFlag(ssoEntireScope) && !mSearchOptions.testFlag(ssoSelectedOnly)
            && editor->selAvail()) {
        // start at end of selection
        if (mSearchOptions.testFlag(ssoBackwards)) {
            editor->setCaretXY(editor->blockBegin());
        } else {
            editor->setCaretXY(editor->blockEnd());
        }
    }

    if (mSearchOptions.testFlag(ssoRegExp)) {
        mSearchEngine = mRegexSearchEngine;
    } else {
        mSearchEngine = mBasicSearchEngine;
    }

    return editor->searchReplace(sSearch, sReplace, mSearchOptions,
                          mSearchEngine, matchCallback);
}

std::shared_ptr<SearchResultTreeItem> SearchDialog::batchFindInEditor(SynEdit *e, const QString& filename,const QString &keyword)
{
    //backup
    BufferCoord caretBackup = e->caretXY();
    BufferCoord blockBeginBackup = e->blockBegin();
    BufferCoord blockEndBackup = e->blockEnd();
    int toplineBackup = e->topLine();
    int leftCharBackup = e->leftChar();

    PSearchResultTreeItem parentItem = std::make_shared<SearchResultTreeItem>();
    parentItem->filename = filename;
    parentItem->parent = nullptr;
    execute(e,keyword,"",
                    [e,&parentItem, filename](const QString&,
                    const QString&, int Line, int ch, int wordLen){
        PSearchResultTreeItem item = std::make_shared<SearchResultTreeItem>();
        item->filename = filename;
        item->line = Line;
        item->start = ch;
        item->len = wordLen;
        item->parent = parentItem.get();
        item->text = e->lines()->getString(Line-1);
        item->text.replace('\t',' ');
        parentItem->results.append(item);
        return SynSearchAction::Skip;
    });

    // restore
    e->setCaretXY(caretBackup);
    e->setTopLine(toplineBackup);
    e->setLeftChar(leftCharBackup);
    e->setCaretAndSelection(
                caretBackup,
                blockBeginBackup,
                blockEndBackup
                );
    return parentItem;
}

QTabBar *SearchDialog::tabBar() const
{
    return mTabBar;
}

PSynSearchBase SearchDialog::searchEngine() const
{
    return mSearchEngine;
}

void SearchDialog::findPrevious()
{
    if (mTabBar->currentIndex()==0) { // it's a find action

        // Disable entire scope searching
        ui->rbEntireScope->setChecked(false);

        // Always search backward
        ui->rbBackward->setChecked(true);

        ui->btnExecute->click();
    }
}
