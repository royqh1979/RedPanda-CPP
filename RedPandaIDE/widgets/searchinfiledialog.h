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
#ifndef SEARCHINFILEDIALOG_H
#define SEARCHINFILEDIALOG_H

#include <QDialog>
#include <qsynedit/qsynedit.h>
#include "../utils.h"

namespace Ui {
class SearchInFileDialog;
}

struct SearchResultTreeItem;
class QTabBar;
class Editor;
class SearchInFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchInFileDialog(QWidget *parent = nullptr);
    ~SearchInFileDialog();
    void findInFiles(const QString& text);
    void findInFiles(const QString& keyword, SearchFileScope scope, QSynedit::SearchOptions options, const QString& folder, const QString& filters, bool searchSubfolders );
    QSynedit::PSynSearchBase searchEngine() const;

private slots:
   void on_cbFind_currentTextChanged(const QString &arg1);

   void on_btnCancel_clicked();

   void on_btnExecute_clicked();
   void on_btnReplace_clicked();

   void on_rbFolder_toggled(bool checked);

   void on_btnChangeFolder_clicked();

private:
   void doSearch(bool replace);
   int execute(QSynedit::QSynEdit* editor, const QString& sSearch,
               const QString& sReplace,
               QSynedit::SearchMathedProc matchCallback = nullptr,
               QSynedit::SearchConfirmAroundProc confirmAroundCallback = nullptr);
   std::shared_ptr<SearchResultTreeItem> batchFindInEditor(QSynedit::QSynEdit * editor,const QString& filename, const QString& keyword);
private:
    Ui::SearchInFileDialog *ui;
    QSynedit::SearchOptions mSearchOptions;
    QSynedit::PSynSearchBase mBasicSearchEngine;
    QSynedit::PSynSearchBase mRegexSearchEngine;

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
};

#endif // SEARCHINFILEDIALOG_H
