#include "compilersetdirectorieswidget.h"
#include "ui_compilersetdirectorieswidget.h"

#include <QFileDialog>
#include <QStringListModel>
#include <QDebug>

CompilerSetDirectoriesWidget::CompilerSetDirectoriesWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CompilerSetDirectoriesWidget)
{
    ui->setupUi(this);

    mModel = new CompilerSetDirectoriesWidget::ListModel();
    ui->listView->setModel(mModel);
    connect(ui->listView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CompilerSetDirectoriesWidget::selectionChanged);
    ui->listView->setSelectionMode(QAbstractItemView::SingleSelection);
}

CompilerSetDirectoriesWidget::~CompilerSetDirectoriesWidget()
{
    delete ui;
}

void CompilerSetDirectoriesWidget::setDirList(const QStringList &list)
{
    mModel->setStringList(list);
    QModelIndexList lst =ui->listView->selectionModel()->selectedIndexes();
    ui->btnDelete->setEnabled(lst.count()>0);
}

QStringList CompilerSetDirectoriesWidget::dirList() const
{
    return mModel->stringList();
}

Qt::ItemFlags CompilerSetDirectoriesWidget::ListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::NoItemFlags;
    if (index.isValid()) {
        flags = Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable ;
    } else if (index.row() == -1) {
        // -1 means it's a drop target?
        flags = Qt::ItemIsDropEnabled;
    }
    return flags;
}

void CompilerSetDirectoriesWidget::on_btnAdd_pressed()
{
    QString folder = QFileDialog::getExistingDirectory(this,tr("Choose Folder"));
    if (!folder.isEmpty()) {
        int row = mModel->rowCount();
        mModel->insertRow(row);
        QModelIndex index= mModel->index(row,0);
        mModel->setData(index,folder,Qt::DisplayRole);
    }
}

void CompilerSetDirectoriesWidget::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    ui->btnDelete->setEnabled(!selected.isEmpty());
}

void CompilerSetDirectoriesWidget::on_btnDelete_pressed()
{
    QModelIndexList lst =ui->listView->selectionModel()->selectedIndexes();
    if (lst.count()>0) {
        mModel->removeRow(lst[0].row());
    }
}


void CompilerSetDirectoriesWidget::on_btnRemoveInvalid_pressed()
{
    QStringList lst;
    for (const QString& folder : dirList() ) {
        QFileInfo info(folder);
        if (info.exists() && info.isDir() ) {
            lst.append(folder.trimmed());
        }
    }
    setDirList(lst);
}
