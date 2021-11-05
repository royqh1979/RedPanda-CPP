#include "environmentfileassociationwidget.h"
#include "ui_environmentfileassociationwidget.h"
#include "../systemconsts.h"
#include "../settings.h"

#include <QMessageBox>
#include <QSettings>
#include <windows.h>

EnvironmentFileAssociationWidget::EnvironmentFileAssociationWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentFileAssociationWidget)
{
    ui->setupUi(this);
    mModel.addItem("C Source File","c",1);
    mModel.addItem("C++ Source File","cpp",2);
    mModel.addItem("C++ Source File","cxx",2);
    mModel.addItem("C++ Source File","cc",2);
    mModel.addItem("C/C++ Header File","h",3);
    mModel.addItem("C++ Header File","hpp",4);
    mModel.addItem("C++ Header File","hxx",4);
    mModel.addItem("Dev-C++ Project File","dev",5);
    ui->lstFileTypes->setModel(&mModel);
    connect(&mModel, &FileAssociationModel::associationChanged,
            [this](){
        setSettingsChanged();
    });
}

EnvironmentFileAssociationWidget::~EnvironmentFileAssociationWidget()
{
    delete ui;
}

void EnvironmentFileAssociationWidget::doLoad()
{
    mModel.updateAssociationStates();
}

void EnvironmentFileAssociationWidget::doSave()
{
    mModel.saveAssociations();
    mModel.updateAssociationStates();
}

FileAssociationModel::FileAssociationModel(QObject *parent) : QAbstractListModel(parent)
{

}

void FileAssociationModel::addItem(const QString &name, const QString &suffix, int icon)
{
    beginInsertRows(QModelIndex(), mItems.count(), mItems.count());
    PFileAssociationItem item = std::make_shared<FileAssociationItem>();
    item->name = name;
    item->suffix = suffix;
    item->icon = icon;
    item->selected = false;
    item->defaultSelected = false;
    mItems.append(item);
    endInsertRows();
}

void FileAssociationModel::updateAssociationStates()
{
    beginResetModel();
    foreach (const PFileAssociationItem& item, mItems) {
        item->selected = checkAssociation(
                    "."+item->suffix,
                    "DevCpp."+item->suffix,
//                    item->name,
                    "Open",
                    pSettings->dirs().executable()+" \"%1\""
                    );
        item->defaultSelected = item->selected;
    }
    endResetModel();
}

void FileAssociationModel::saveAssociations()
{
    QSet<QString> fileTypeUsed;
    QSet<QString> fileTypes;
    QMap<QString,PFileAssociationItem> fileTypeDescriptions;

    foreach (const PFileAssociationItem& item, mItems) {
        if (item->selected == item->defaultSelected
                && !item->selected)
            continue;
        bool ok;
        fileTypes.insert("DevCpp."+item->suffix);
        fileTypeDescriptions.insert("DevCpp."+item->suffix,item);
        if (!item->selected) {
            ok = unregisterAssociation("."+item->suffix);
        } else {
            fileTypeUsed.insert("DevCpp."+item->suffix);
            ok = registerAssociation(
                        "."+item->suffix,
                        "DevCpp."+item->suffix
            );
        }
        if (!ok) {
            QMessageBox::critical(NULL,
                                  tr("Register File Association Error"),
                                  tr("Don't have privilege to register file types!"));
            return;
        }
    }
    foreach (const QString& fileType, fileTypes) {
        bool ok;
        if (fileTypeUsed.contains(fileType)) {
            PFileAssociationItem item = fileTypeDescriptions[fileType];
            ok = registerFileType(fileType,
                                  item->name,
                                  "Open",
                                  pSettings->dirs().executable(),
                                  item->icon);
        } else {
            ok = unregisterFileType(fileType);
        }
        if (!ok) {
            QMessageBox::critical(NULL,
                                  tr("Register File Type Error"),
                                  tr("Don't have privilege to register file types!"));
            return;
        }
    }

}

bool FileAssociationModel::checkAssociation(const QString &extension, const QString &filetype, const QString &verb, const QString &serverApp)
{
    HKEY key;
    LONG result;

    result = RegOpenKeyExA(HKEY_CLASSES_ROOT,extension.toLocal8Bit(),0,KEY_READ,&key);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS )
        return false;

    result = RegOpenKeyExA(HKEY_CLASSES_ROOT,filetype.toLocal8Bit(),0,KEY_READ,&key);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS )
        return false;

    QString keyString = QString("%1\\Shell\\%2\\Command").arg(filetype).arg(verb);
    QString value1,value2;
    if (!readRegistry(HKEY_CLASSES_ROOT,keyString.toLocal8Bit(),"",value1))
        return false;
    if (!readRegistry(HKEY_CLASSES_ROOT,extension.toLocal8Bit(),"",value2))
        return false;

    return (value2 == filetype)
            && (value1.compare(serverApp,PATH_SENSITIVITY)==0);
}

bool writeRegistry(HKEY parentKey, const QByteArray& subKey, const QByteArray& value) {
    DWORD disposition;
    HKEY key;
    LONG result = RegCreateKeyExA(
                parentKey,
                subKey,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &key,
                &disposition);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS ) {
        return false;
    }
    result = RegSetKeyValueA(
                HKEY_CLASSES_ROOT,
                subKey,
                "",
                REG_SZ,
                (const BYTE*)value.data(),
                value.length()+1);
    return result == ERROR_SUCCESS;
}
bool FileAssociationModel::registerAssociation(const QString &extension, const QString &filetype)
{
    if (!writeRegistry(HKEY_CLASSES_ROOT,
                         extension.toLocal8Bit(),
                         filetype.toLocal8Bit())){
        return false;
    }
    return true;
}

bool FileAssociationModel::unregisterAssociation(const QString &extension)
{
    LONG result;
    HKEY key;
    result = RegOpenKeyExA(HKEY_CLASSES_ROOT,extension.toLocal8Bit(),0,KEY_READ,&key);
    if (result != ERROR_SUCCESS )
        return true;
    RegCloseKey(key);

    result = RegDeleteTreeA(HKEY_CLASSES_ROOT,extension.toLocal8Bit());
    return result == ERROR_SUCCESS;

}

bool FileAssociationModel::unregisterFileType(const QString &fileType)
{
    LONG result;
    HKEY key;
    result = RegOpenKeyExA(HKEY_CLASSES_ROOT,fileType.toLocal8Bit(),0,KEY_READ,&key);
    if (result != ERROR_SUCCESS )
        return true;
    RegCloseKey(key);

    result = RegDeleteTreeA(HKEY_CLASSES_ROOT,fileType.toLocal8Bit());
    return result == ERROR_SUCCESS;
}

bool FileAssociationModel::registerFileType(const QString &filetype, const QString &description, const QString &verb, const QString &serverApp, int icon)
{
    if (!writeRegistry(HKEY_CLASSES_ROOT,
                       filetype.toLocal8Bit(),
                       description.toLocal8Bit()))
        return false;

    QString keyString = QString("%1\\DefaultIcon").arg(filetype);
    QString value = QString("%1,%2").arg(serverApp).arg(icon);
    if (!writeRegistry(HKEY_CLASSES_ROOT,
                         keyString.toLocal8Bit(),
                         value.toLocal8Bit()))
        return false;
    keyString = QString("%1\\Shell\\%2\\Command").arg(filetype).arg(verb);
    value = serverApp+" \"%1\"";
    if (!writeRegistry(HKEY_CLASSES_ROOT,
                         keyString.toLocal8Bit(),
                         value.toLocal8Bit()))
        return false;
    return true;
}

int FileAssociationModel::rowCount(const QModelIndex &) const
{
    return mItems.count();
}

QVariant FileAssociationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    PFileAssociationItem item = mItems[index.row()];
    if (role == Qt::DisplayRole) {
        return QString("%1 (*.%2)").arg(item->name).arg(item->suffix);
    } else if (role == Qt::CheckStateRole) {
        return (item->selected)? Qt::Checked : Qt::Unchecked;
    }
    return QVariant();
}

bool FileAssociationModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (role == Qt::CheckStateRole) {
        PFileAssociationItem item = mItems[index.row()];
        item->selected = value.toBool();
        emit associationChanged();
        return true;
    }
    return false;
}

Qt::ItemFlags FileAssociationModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
}
