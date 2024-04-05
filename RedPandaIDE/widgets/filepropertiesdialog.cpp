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
#include "filepropertiesdialog.h"
#include "systemconsts.h"
#include "ui_filepropertiesdialog.h"
#include "../mainwindow.h"
#include "../editorlist.h"
#include "../editor.h"
#include <qsynedit/constants.h>

#include <QFileInfo>

FilePropertiesDialog::FilePropertiesDialog(Editor* activeEditor,QWidget *parent) :
    QDialog(parent),
    mActiveEditor(activeEditor),
    ui(new Ui::FilePropertiesDialog)
{
    ui->setupUi(this);
    ui->cbFiles->setModel(&mModel);
}

FilePropertiesDialog::~FilePropertiesDialog()
{
    delete ui;
}

void FilePropertiesDialog::calcFile(Editor *editor,
                                    int &totalLines,
                                    int &commentLines,
                                    int &emptyLines,
                                    int &codeLines,
                                    int &includeLines,
                                    int &charCounts)
{
    totalLines = editor->lineCount();
    codeLines = 0;
    commentLines = 0;
    emptyLines = 0;
    includeLines = 0;
    charCounts = 0;
    int lineBreakerLen = QString(LINE_BREAKER).length();
    // iterate through all lines of file
    for (int i=1;i<=editor->lineCount();i++) {
        QString line = editor->lineText(i);
        charCounts+=line.length()+lineBreakerLen;
//        while (j<line.length() && (line[j]=='\t' || line[j]==' '))
//            j++;
        QString token;
        QSynedit::PTokenAttribute attr;
        if (editor->getTokenAttriAtRowCol(QSynedit::BufferCoord{1,i},
                                                token,attr)) {
            // if it is preprocessor...
            if (attr->name() == SYNS_AttrPreprocessor) {
                // check for includes
                token.remove(0,1);
                token=token.trimmed();
                if (token.startsWith("include"))
                    includeLines++;

                // preprocessor directives are considered as code
                codeLines++;
            } else if (attr->name() == SYNS_AttrComment) {
                commentLines++;
            } else {
                codeLines++;
            }
        } else {
            // if we don't get a token type, this line is empty or contains only spaces
            emptyLines++;
        }

    }
}

void FilePropertiesDialog::showEvent(QShowEvent *)
{
    for (int i=0;i<pMainWindow->editorList()->pageCount();i++) {
        Editor * editor =  (*(pMainWindow->editorList()))[i];
        if (editor == mActiveEditor) {
            ui->cbFiles->setCurrentIndex(i);
            break;
        }
    }
}

FilePropertiesModel::FilePropertiesModel(QObject *parent):QAbstractListModel(parent)
{

}

int FilePropertiesModel::rowCount(const QModelIndex &) const
{
    return pMainWindow->editorList()->pageCount();
}

QVariant FilePropertiesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    int row = index.row();
    if (role == Qt::DisplayRole) {
        if (row>=0 && row < pMainWindow->editorList()->pageCount()) {
            Editor *editor = (*(pMainWindow->editorList()))[row];
            if (editor) {
                return extractFileName(editor->filename());
            }
        }
    }
    return QVariant();
}

void FilePropertiesDialog::on_cbFiles_currentIndexChanged(int index)
{
    Editor *editor = (*(pMainWindow->editorList()))[index];
    if (editor) {
        QFileInfo info(editor->filename());

        int fileSize = info.size();
        // Pretty print total file size
        ui->txtFileSize->setText(getSizeString(fileSize));
        ui->txtFileDate->setText( QLocale::system().toString(info.lastModified(), QLocale::LongFormat));
        ui->txtProject->setText("-");
        ui->txtPath->setText(editor->filename());
        ui->txtRelativeToProject->setText("_");
        ui->txtLines->setText(QString("%1").arg(editor->lineCount()));

        int totalLines, codeLines,emptyLines,commentLines,includeLines, charCounts;
        calcFile(editor,totalLines,commentLines,emptyLines,codeLines,includeLines,charCounts);

        ui->txtLines->setText(QString("%1").arg(totalLines));
        ui->txtEmptyLines->setText(QString("%1").arg(emptyLines));
        ui->txtCodeLines->setText(QString("%1").arg(codeLines));
        ui->txtCommentLines->setText(QString("%1").arg(commentLines));
        ui->txtIncludes->setText(QString("%1").arg(includeLines));
        ui->txtCharacters->setText(QString("%1").arg(charCounts));
    }
}


void FilePropertiesDialog::on_btnOK_clicked()
{
    hide();
}

