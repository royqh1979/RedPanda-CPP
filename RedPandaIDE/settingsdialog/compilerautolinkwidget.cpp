#include "compilerautolinkwidget.h"
#include "ui_compilerautolinkwidget.h"
#include "../mainwindow.h"
#include "../settings.h"

#include <QMessageBox>

CompilerAutolinkWidget::CompilerAutolinkWidget(const QString& name, const QString& group, QWidget* parent) :
    SettingsWidget(name,group,parent),
    mModel(this),
    ui(new Ui::CompilerAutolinkWidget)
{
    ui->setupUi(this);
    ui->tblAutolinks->setModel(&mModel);
}

CompilerAutolinkWidget::~CompilerAutolinkWidget()
{
    delete ui;
}

void CompilerAutolinkWidget::doLoad()
{
    ui->grpAutolink->setChecked(pSettings->editor().enableAutolink());
    mModel.setLinks(pAutolinkManager->links());
}

void CompilerAutolinkWidget::doSave()
{
    pSettings->editor().setEnableAutolink(ui->grpAutolink->isChecked());
    pSettings->editor().save();
    pAutolinkManager->clear();
    for (const PAutolink& link:mModel.links()) {
        if (!link->header.isEmpty()) {
            pAutolinkManager->setLink(
                        link->header,
                        link->linkOption
                        );
        }
    }
    try{
        pAutolinkManager->save();
    } catch (FileError e) {
        QMessageBox::critical(this,
                              tr("Save failed."),
                              e.reason(),
                              QMessageBox::Ok);
    }
}

AutolinkModel::AutolinkModel(CompilerAutolinkWidget* widget,QObject *parent):
    QAbstractTableModel(parent),
    mWidget(widget)
{

}

int AutolinkModel::rowCount(const QModelIndex &) const
{
    return mLinks.count();
}

int AutolinkModel::columnCount(const QModelIndex &) const
{
    return 2;
}

QVariant AutolinkModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role ==  Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Header");
        case 1:
            return tr("Link options");
        }
    }
    return QVariant();
}

QVariant AutolinkModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        int row = index.row();
        if (row<0 || row>=mLinks.count())
            return QVariant();
        PAutolink link = mLinks[row];
        switch(index.column()) {
        case 0:
            return link->header;
        case 1:
            return link->linkOption;
        }
    }
    return QVariant();
}

bool AutolinkModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (role == Qt::EditRole) {
        int row = index.row();
        if (row<0 || row>=mLinks.count())
            return false;
        PAutolink link = mLinks[row];
        QString s=value.toString().trimmed();
        if (index.column() == 0) {
            if (s.isEmpty())
                return false;
            if (findLink(s)>=0) {
                QMessageBox::warning(pMainWindow,
                                     tr("Header exists"),
                                     tr("Header already exists."),
                                     QMessageBox::Ok);
                return false;
            }
            //we must create a new link, becasue mList may share link pointer with the autolink manger
            PAutolink newLink = std::make_shared<Autolink>();
            newLink->header = s;
            newLink->linkOption = link->linkOption;
            mLinks[row]=newLink;
            mWidget->setSettingsChanged();
            return true;
        } else if (index.column() == 1) {
            //we must create a new link, becasue mList may share link pointer with the autolink manger
            PAutolink newLink = std::make_shared<Autolink>();
            newLink->header = link->header;
            newLink->linkOption = s;
            mLinks[row]=newLink;
            mWidget->setSettingsChanged();
            return true;
        }
    }
    return false;
}

Qt::ItemFlags AutolinkModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::NoItemFlags;
    if (index.isValid()) {
        flags = Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable ;
    }
    return flags;
}

bool AutolinkModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row<0 || row>mLinks.count())
        return false;
    beginInsertRows(parent,row,row+count-1);
    for (int i=row;i<row+count;i++) {
        PAutolink link = std::make_shared<Autolink>();
        mLinks.insert(i,link);
    }
    endInsertRows();
    return true;
}

bool AutolinkModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row<0 || row>=mLinks.count())
        return false;
    beginRemoveRows(parent,row,row+count-1);
    for (int i=row;i<row+count;i++) {
        if (i>=mLinks.count())
            break;
        mLinks.removeAt(i);
    }
    endRemoveRows();
    return true;

}

const QList<PAutolink> &AutolinkModel::links() const
{
    return mLinks;
}

void AutolinkModel::setLinks(const QMap<QString, PAutolink> &newLinks)
{
    beginResetModel();
    mLinks = newLinks.values();
    endResetModel();
}

int AutolinkModel::findLink(const QString &header)
{
    for (int i=0;i<mLinks.count();i++) {
        PAutolink link = mLinks[i];
        if (link->header == header)
            return i;
    }
    return -1;
}

void CompilerAutolinkWidget::on_btnAdd_pressed()
{
    mModel.insertRow(mModel.links().count());
}


void CompilerAutolinkWidget::on_btnRemove_pressed()
{
    QModelIndex index = ui->tblAutolinks->currentIndex();
    mModel.removeRow(index.row(),index.parent());
}

