#include "compilersetdirectorieswidget.h"
#include "ui_compilersetdirectorieswidget.h"

#include <QStringListModel>

CompilerSetDirectoriesWidget::CompilerSetDirectoriesWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CompilerSetDirectoriesWidget)
{
    ui->setupUi(this);

    mModel = new CompilerSetDirectoriesWidget::ListModel();
    ui->listView->setModel(mModel);
}

CompilerSetDirectoriesWidget::~CompilerSetDirectoriesWidget()
{
    delete ui;
}

void CompilerSetDirectoriesWidget::setDirList(const QStringList &list)
{
    mModel->setStringList(list);
}

QStringList CompilerSetDirectoriesWidget::dirList() const
{
    return mModel->stringList();
}

Qt::ItemFlags CompilerSetDirectoriesWidget::ListModel::flags(const QModelIndex &index) const
{
    if (index.isValid()) {
        return Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
    }
}
