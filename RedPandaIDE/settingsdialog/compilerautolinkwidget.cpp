#include "compilerautolinkwidget.h"
#include "ui_compilerautolinkwidget.h"
#include "../mainwindow.h"
#include "../settings.h"

#include <QMessageBox>

CompilerAutolinkWidget::CompilerAutolinkWidget(const QString& name, const QString& group, QWidget* parent) :
    SettingsWidget(name,group,parent),
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
    auto iter = mModel.links().cbegin();
    while (iter!=mModel.links().cend()) {
        PAutolink link = iter.value();
        pAutolinkManager->setLink(
                    link->header,
                    link->linkOption
                    );
        iter++;
    }
    pAutolinkManager->save();
}

AutolinkModel::AutolinkModel(QObject *parent):QAbstractTableModel(parent)
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
        QList<PAutolink> links = mLinks.values();
        if (row<0 || row>=links.count())
            return QVariant();
        PAutolink link = links[row];
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
        QList<PAutolink> links = mLinks.values();
        if (row<0 || row>=links.count())
            return false;
        PAutolink link = links[row];
        QString s=value.toString();
        if (index.column() == 0) {
            if (s.isEmpty())
                return false;
            PAutolink oldLink = mLinks.value(s,PAutolink());
            if (oldLink) {
                QMessageBox::warning(pMainWindow,
                                     tr("Header exists"),
                                     tr("Header already exists."),
                                     QMessageBox::Yes);
                return false;
            }
            mLinks.remove(link->header);
            PAutolink newLink = std::make_shared<Autolink>();
            newLink->header = s;
            newLink->linkOption = link->linkOption;
            mLinks.insert(newLink->header,newLink);
            return true;
        } else if (index.column() == 1) {
            PAutolink newLink = std::make_shared<Autolink>();
            newLink->header = link->header;
            newLink->linkOption = s;
            mLinks.insert(newLink->header,newLink);
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

const QMap<QString, PAutolink> &AutolinkModel::links() const
{
    return mLinks;
}

void AutolinkModel::setLinks(const QMap<QString, PAutolink> &newLinks)
{
    mLinks = newLinks;
}
