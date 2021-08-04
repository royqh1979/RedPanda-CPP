#include "searchdialog.h"
#include "ui_searchdialog.h"
#include <QTabBar>
#include "../editor.h"
#include "../mainwindow.h"
#include "../editorlist.h"
#include "../qsynedit/Search.h"
#include "../qsynedit/SearchRegex.h"
#include <QMessageBox>


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
    bool isfindfiles = (mTabBar->currentIndex() == 1);
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
    ui->rbProject->setEnabled(false);
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

void SearchDialog::on_btnExecute_clicked()
{
    int findcount = 0;

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
            findcount+=execute(e,ui->cbFind->currentText(),"");
        }
    } else {
        Editor *e = pMainWindow->editorList()->getEditor();
        if (e!=nullptr) {
            bool doPrompt = ui->chkPrompt->isChecked();
            findcount+=execute(e,ui->cbFind->currentText(),ui->cbReplace->currentText(),
                               [&doPrompt](const QString& sSearch,
                               const QString& sReplace, int Line, int ch, int wordLen){
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

    }

//    // Replace first, find to next
//end else if actiontype = faReplace then begin
//  e := MainForm.EditorList.GetEditor;

//  if Assigned(e) then begin
//    Inc(findcount, Execute(e.Text, faReplace));
//    if findcount > 0 then begin
//      Exclude(fSearchOptions, ssoReplace);
//      Inc(findcount, Execute(e.Text, faFind));
//    end;
//  end;
//  // Or find everything
//end else if actiontype = faFindFiles then begin
//  fileSearched:=0;
//  fileHitted:=0;
//  MainForm.FindOutput.BeginFind(cboFindText.Text);
//  try

//    // loop through pagecontrol
//    if rbOpenFiles.Checked then begin

//      // loop through editors, add results to message control
//      for I := 0 to MainForm.EditorList.PageCount - 1 do begin
//        e := MainForm.EditorList[i];
//        if Assigned(e) then begin
//          inc(fileSearched);
//          fCurFile := e.FileName;
//          t:=Execute(e.Text, actiontype);
//          Inc(findcount, t);
//          if t>0 then
//            inc(filehitted);
//        end;
//      end;

//      // loop through project
//    end else if rbProjectFiles.Checked then begin
//      for I := 0 to MainForm.Project.Units.Count - 1 do begin
//        e := MainForm.Project.Units[i].Editor;
//        fCurFile := MainForm.Project.Units[i].FileName;

//        // file is already open, use memory
//        if Assigned(e) then begin begin
//          inc(fileSearched);
//          t:=Execute(e.Text, actiontype);
//          Inc(findcount, t);
//          if t>0 then
//            inc(filehitted);
//        end;

//          // not open? load from disk
//        end else if FileExists(fCurFile) then begin
//            // Only finding...
//          fTempSynEdit.Lines.LoadFromFile(fCurFile);
//          inc(fileSearched);
//          t:=Execute(fTempSynEdit, actiontype);
//          Inc(findcount, t);
//          if t>0 then
//            inc(filehitted);
//        end;
//      end;

//      // Don't loop, only pass single file
//    end else if rbCurFile.Checked then begin
//      e := MainForm.EditorList.GetEditor;

//      if Assigned(e) then begin

//        fCurFile := e.FileName;

//        inc(fileSearched);
//        t:=Execute(e.Text, actiontype);
//        Inc(findcount, t);
//        if t>0 then
//          inc(filehitted);
//      end;
//    end;
//  finally
//    MainForm.FindOutput.EndFind(cboFindText.Text,findCount,
//      filehitted,filesearched);
//  end;
//end else if actiontype = faReplaceFiles then begin
//  // loop through pagecontrol
//  if rbOpenFiles.Checked then begin

//    // loop through editors, add results to message control
//    for I := 0 to MainForm.EditorList.PageCount - 1 do begin
//      e := MainForm.EditorList[i];
//      if Assigned(e) then begin
//        fCurFile := e.FileName;
//        if (ssoPrompt in fSearchOptions) then
//            e.Activate;
//        Inc(findcount, Execute(e.Text, actiontype));
//      end;
//    end;

//      // loop through project
//  end else if rbProjectFiles.Checked then begin
//    for I := 0 to MainForm.Project.Units.Count - 1 do begin
//      e := MainForm.Project.Units[i].Editor;
//      fCurFile := MainForm.Project.Units[i].FileName;

//      // file is already open, use memory
//      if Assigned(e) then begin
//        if (ssoPrompt in fSearchOptions) then
//            e.Activate;
//        Inc(findcount, Execute(e.Text, actiontype));

//          // not open? load from disk
//      end else if FileExists(fCurFile) then begin
//        // we have to open an editor...
//        if ssoPrompt in fSearchOptions then begin
//          e := MainForm.EditorList.GetEditorFromFileName(fCurFile);
//          if Assigned(e) then begin
//            e.Activate;

//            Inc(findcount, Execute(e.Text, actiontype));

//            // Save and close
//            e.Save;
//            MainForm.Project.CloseUnit(MainForm.Project.Units.Indexof(e));
//          end;
//        end else begin
//          // Stealth replace
//          fTempSynEdit.Lines.LoadFromFile(fCurFile);
//          Inc(findcount, Execute(fTempSynEdit, actiontype));
//          fTempSynEdit.Lines.SaveToFile(fCurFile);
//        end;
//      end;
//    end;
//      // Don't loop, only pass single file
//  end else if rbCurFile.Checked then begin
//    e := MainForm.EditorList.GetEditor;

//    if Assigned(e) then begin
//      fCurFile := e.FileName;
//      Inc(findcount, Execute(e.Text, actiontype));
//    end;
//  end;
//end;

//if actiontype = faFindFiles then begin
//  MainForm.MessageControl.ActivePageIndex := 4; // Find Tab
//  if findcount > 0 then
//    MainForm.FindSheet.Caption := Lang[ID_SHEET_FIND];
//  MainForm.OpenCloseMessageSheet(TRUE);
//  self.Close;
//end else if findcount = 0 then begin
//  MessageBox(
//    Self.Handle,
//    PAnsiChar(Format(Lang[ID_MSG_TEXTNOTFOUND], [cboFindText.Text])),
//    PAnsiChar(Lang[ID_INFO]),
//    MB_ICONINFORMATION or MB_TOPMOST);
//  cboFindText.SetFocus;
//end;
//if actiontype = faFind then begin
//  self.Close;
    //end;
}

int SearchDialog::execute(Editor *editor, const QString &sSearch, const QString &sReplace, SynSearchMathedProc matchCallback)
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


//    // When using find in files, report each find using OnReplaceText
//    if action = faFindFiles then
//      editor.OnReplaceText := FindAllAction;

//    // Swap search engire for ours
//    if (ssoRegExp in fSearchOptions) then
//      editor.SearchEngine := fRegExpSearchEngine
//    else
//      editor.SearchEngine := fSearchEngine;
//    result := editor.SearchReplace(cboFindText.Text, cboReplaceText.Text, fSearchOptions);

//    // Don't touch editors which we are only scanning
//    if action in [faFindFiles] then begin
//      // Put backup back into place
//      editor.CaretXY := caretbackup;
//      editor.BlockBegin := blockbeginbackup;
//      editor.BlockEnd := blockendbackup;
//      editor.TopLine := toplinebackup;
//    end;

//    editor.OnReplaceText := onreplacebackup;
//    editor.SearchEngine := enginebackup;
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
