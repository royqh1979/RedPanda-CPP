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
#include "searchinfiledialog.h"
#include "ui_searchinfiledialog.h"
#include <QTabBar>
#include "../editor.h"
#include "../mainwindow.h"
#include "../editorlist.h"
#include <qsynedit/searcher/basicsearcher.h>
#include <qsynedit/searcher/regexsearcher.h>
#include "../project.h"
#include "../settings.h"
#include "../systemconsts.h"
#include <QMessageBox>
#include <QDebug>
#include <QProgressDialog>
#include <QCompleter>
#include <QStack>
#include <QFileDialog>


SearchInFileDialog::SearchInFileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchInFileDialog)
{
    setWindowFlag(Qt::WindowContextHelpButtonHint,false);
    ui->setupUi(this);
    mSearchOptions&=0;
    mBasicSearchEngine= QSynedit::PSynSearchBase(new QSynedit::BasicSearcher());
    mRegexSearchEngine= QSynedit::PSynSearchBase(new QSynedit::RegexSearcher());
    ui->cbFind->completer()->setCaseSensitivity(Qt::CaseSensitive);
    on_rbFolder_toggled(false);
}

SearchInFileDialog::~SearchInFileDialog()
{
    delete ui;
}


void SearchInFileDialog::findInFiles(const QString &text)
{
    ui->cbFind->setCurrentText(text);
    ui->cbFind->setFocus();
    show();
}

void SearchInFileDialog::findInFiles(const QString &keyword, SearchFileScope scope, QSynedit::SearchOptions options, const QString& folder, const QString& filters, bool searchSubfolders)
{
    ui->cbFind->setCurrentText(keyword);
    ui->cbFind->setFocus();

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
    case SearchFileScope::Folder:
        ui->txtFolder->setText(folder);
        ui->txtFilters->setText(filters);
        ui->cbSearchSubFolders->setChecked(searchSubfolders);
        ui->rbFolder->setChecked(true);
        break;
    }
    // Apply options
    ui->chkRegExp->setChecked(options.testFlag(QSynedit::ssoRegExp));
    ui->chkCaseSensetive->setChecked(options.testFlag(QSynedit::ssoMatchCase));
    ui->chkWholeWord->setChecked(options.testFlag(QSynedit::ssoWholeWord));
    show();
}

void SearchInFileDialog::on_cbFind_currentTextChanged(const QString &value)
{
    ui->btnExecute->setEnabled(!value.isEmpty());
    ui->btnReplace->setEnabled(!value.isEmpty());
}

void SearchInFileDialog::on_btnCancel_clicked()
{
    this->close();
}

void SearchInFileDialog::on_btnExecute_clicked()
{
    doSearch(false);
}

void SearchInFileDialog::doSearch(bool replace)
{
    saveComboHistory(ui->cbFind,ui->cbFind->currentText());

    mSearchOptions&=0;

    // Apply options
    if (ui->chkRegExp->isChecked()) {
        mSearchOptions.setFlag(QSynedit::ssoRegExp);
    }
    if (ui->chkCaseSensetive->isChecked()) {
        mSearchOptions.setFlag(QSynedit::ssoMatchCase);
    }
    if (ui->chkWholeWord->isChecked()) {
        mSearchOptions.setFlag(QSynedit::ssoWholeWord);
    }

    mSearchOptions.setFlag(QSynedit::ssoEntireScope);

    close();

    int findCount=0;
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
    } else if (ui->rbFolder->isChecked()) {
        PSearchResults results = pMainWindow->searchResultModel()->addSearchResults(
                    keyword,
                    mSearchOptions,
                    SearchFileScope::Folder,
                    ui->txtFolder->text(),
                    ui->txtFilters->text(),
                    ui->cbSearchSubFolders
                    );
        if (ui->txtFilters->text().trimmed().isEmpty()) {
            ui->txtFilters->setText("*.*");
        }
        QDir rootDir(ui->txtFolder->text());
        // loop through editors, add results to message control
        QProgressDialog progressDlg(
                    tr("Searching..."),
                    tr("Abort"),
                    0,
                    1,
                    pMainWindow);

        progressDlg.setWindowModality(Qt::WindowModal);
        QStack<QDir> dirs;
        QSet<QString> searched;
        QList<QFileInfo> files;
        dirs.push(rootDir);
        QDir::Filters filterOptions=QDir::Files | QDir::NoSymLinks;
        if (PATH_SENSITIVITY==Qt::CaseSensitive)
            filterOptions |= QDir::CaseSensitive;
        while (!dirs.isEmpty()) {
            QDir dir=dirs.back();
            dirs.pop_back();
            foreach(const QFileInfo& entry, dir.entryInfoList(QDir::NoSymLinks | QDir::Dirs)) {
                if (entry.fileName()==".." || entry.fileName()==".")
                    continue;
                if (!searched.contains(entry.absoluteFilePath())) {
                    dirs.push_back(QDir(entry.absoluteFilePath()));
                    searched.insert(entry.absoluteFilePath());
                }
            }
            foreach(const QFileInfo& entry, dir.entryInfoList(ui->txtFilters->text().split(";"), filterOptions)) {
                files.append(entry);
            }
        }
        progressDlg.setMaximum(files.count());
        int i=0;
        foreach (const QFileInfo &info, files) {
            QString curFilename =  info.absoluteFilePath();
            i++;
            progressDlg.setValue(i);
            progressDlg.setLabelText(tr("Searching...")+"<br/>"+curFilename);

            if (progressDlg.wasCanceled())
                break;
            Editor * e = pMainWindow->editorList()->getOpenedEditorByFilename(curFilename);
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
                QSynedit::QSynEdit editor;
                QByteArray realEncoding;
                try{
                    editor.document()->loadFromFile(curFilename,ENCODING_AUTO_DETECT, realEncoding);
                } catch (QSynedit::BinaryFileError e) {
                    continue;
                } catch (FileError e) {
                    continue;
                }
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
        if (!pMainWindow->project())
            return;
        PSearchResults results = pMainWindow->searchResultModel()->addSearchResults(
                    keyword,
                    mSearchOptions,
                    SearchFileScope::wholeProject
                    );
        QByteArray projectEncoding = pMainWindow->project()->options().encoding;
        QProgressDialog progressDlg(
                    tr("Searching..."),
                    tr("Abort"),
                    0,
                    pMainWindow->project()->unitList().count(),
                    pMainWindow);

        progressDlg.setWindowModality(Qt::WindowModal);
        int i=0;
        foreach (PProjectUnit unit, pMainWindow->project()->unitList()) {
            i++;
            progressDlg.setValue(i);
            progressDlg.setLabelText(tr("Searching...")+"<br/>"+unit->fileName());

            if (progressDlg.wasCanceled())
                break;
            Editor * e = pMainWindow->project()->unitEditor(unit);
            QString curFilename =  unit->fileName();
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
                QSynedit::QSynEdit editor;
                QByteArray encoding=unit->encoding();
                if (encoding==ENCODING_PROJECT)
                    encoding = projectEncoding;
                QByteArray realEncoding;
                try {
                    editor.document()->loadFromFile(curFilename,encoding, realEncoding);
                } catch (QSynedit::BinaryFileError e) {
                    continue;
                } catch (FileError e) {
                    continue;
                }
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
    pMainWindow->showSearchPanel(replace);

}

int SearchInFileDialog::execute(QSynedit::QSynEdit *editor, const QString &sSearch, const QString &sReplace,
                          QSynedit::SearchMathedProc matchCallback,
                          QSynedit::SearchConfirmAroundProc confirmAroundCallback)
{
    if (editor==nullptr)
        return 0;
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

    return editor->searchReplace(sSearch, sReplace, mSearchOptions,
                          searchEngine, matchCallback, confirmAroundCallback);
}

std::shared_ptr<SearchResultTreeItem> SearchInFileDialog::batchFindInEditor(QSynedit::QSynEdit *e, const QString& filename,const QString &keyword)
{
    //backup
    QSynedit::BufferCoord caretBackup = e->caretXY();
    QSynedit::BufferCoord blockBeginBackup = e->blockBegin();
    QSynedit::BufferCoord blockEndBackup = e->blockEnd();
    int topPosBackup = e->topPos();
    int leftPosBackup = e->leftPos();

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
        item->text = e->document()->getLine(Line-1);
        item->text.replace('\t',' ');
        parentItem->results.append(item);
        return QSynedit::SearchAction::Skip;
    });

    // restore
    e->setCaretXY(caretBackup);
    e->setTopPos(topPosBackup);
    e->setLeftPos(leftPosBackup);
    e->setCaretAndSelection(
                caretBackup,
                blockBeginBackup,
                blockEndBackup
                );
    return parentItem;
}

void SearchInFileDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    ui->rbProject->setEnabled(pMainWindow->project()!=nullptr);
    if (ui->rbProject->isChecked() && !ui->rbProject->isEnabled())
        ui->rbCurrentFile->setChecked(true);
    if (pSettings->environment().language()=="zh_CN") {
        ui->txtRegExpHelp->setText(
                    QString("<html><head/><body><p><a href=\"%1\"><span style=\" text-decoration: underline; color:#0000ff;\">(?)</span></a></p></body></html>")
                    .arg("https://www.runoob.com/regexp/regexp-tutorial.html"));
    } else {
        ui->txtRegExpHelp->setText(
                    QString("<html><head/><body><p><a href=\"%1\"><span style=\" text-decoration: underline; color:#0000ff;\">(?)</span></a></p></body></html>")
                    .arg("https://docs.microsoft.com/en-us/dotnet/standard/base-types/regular-expression-language-quick-reference"));
    }
}

void SearchInFileDialog::on_btnReplace_clicked()
{
    doSearch(true);
}


void SearchInFileDialog::on_rbFolder_toggled(bool checked)
{
//    ui->lblFilters->setVisible(checked);
    ui->txtFilters->setEnabled(checked);
//    ui->lblFolder->setVisible(checked);
    ui->txtFolder->setEnabled(checked);
    ui->btnChangeFolder->setEnabled(checked);
    ui->cbSearchSubFolders->setEnabled(checked);
    if (checked) {
        if (!directoryExists(ui->txtFolder->text()))
            ui->txtFolder->setText(pSettings->environment().currentFolder());
    }
}


void SearchInFileDialog::on_btnChangeFolder_clicked()
{
    QString folder = QFileDialog::getExistingDirectory(this,tr("Choose Folder"),
                                                       ui->txtFolder->text());
    if (directoryExists(folder))
        ui->txtFolder->setText(folder);
}

