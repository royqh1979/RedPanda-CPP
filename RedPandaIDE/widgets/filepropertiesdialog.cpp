#include "filepropertiesdialog.h"
#include "ui_filepropertiesdialog.h"
#include "../mainwindow.h"
#include "../editorlist.h"
#include "../editor.h"
#include "../qsynedit/Constants.h"

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
                                    int &includeLines)
{
    totalLines = editor->lines()->count();
    codeLines = 0;
    commentLines = 0;
    emptyLines = 0;
    includeLines = 0;
    // iterate through all lines of file
    for (int i=0;i<editor->lines()->count();i++) {
        QString line = editor->lines()->getString(i);
        int j=0;
        while (j<line.length() && (line[j]=='\t' || line[j]==' '))
            j++;
        QString token;
        PSynHighlighterAttribute attr;
        if (editor->GetHighlighterAttriAtRowCol(BufferCoord{j+1,i+1},
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
                return baseFileName(editor->filename());
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
        ui->txtLines->setText(QString("%1").arg(editor->lines()->count()));

        int totalLines, codeLines,emptyLines,commentLines,includeLines;
        calcFile(editor,totalLines,commentLines,emptyLines,codeLines,includeLines);

        ui->txtLines->setText(QString("%1").arg(totalLines));
        ui->txtEmptyLines->setText(QString("%1").arg(emptyLines));
        ui->txtCodeLines->setText(QString("%1").arg(codeLines));
        ui->txtCommentLines->setText(QString("%1").arg(commentLines));
        ui->txtIncludes->setText(QString("%1").arg(includeLines));
    }
}


void FilePropertiesDialog::on_btnOK_clicked()
{
    hide();
}

