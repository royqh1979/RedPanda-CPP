#ifndef ENVIRONMENTFILEASSOCIATIONWIDGET_H
#define ENVIRONMENTFILEASSOCIATIONWIDGET_H

#include <QAbstractListModel>
#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EnvironmentFileAssociationWidget;
}
struct FileAssociationItem {
    QString name;
    QString suffix;
    int icon;
    bool selected;
    bool defaultSelected;
};
using PFileAssociationItem = std::shared_ptr<FileAssociationItem>;

class FileAssociationModel:public QAbstractListModel {
    Q_OBJECT
public:
    explicit FileAssociationModel(QObject* parent = nullptr);
    void addItem(const QString& name, const QString& suffix, int icon);
    void updateAssociationStates();
    void saveAssociations();
signals:
    void associationChanged();
private:
    bool checkAssociation(const QString& extension,
                          const QString& filetype,
                          const QString& verb,
                          const QString& serverApp);
    bool registerAssociation(const QString& extension,
                             const QString& filetype);
    bool unregisterAssociation(const QString& extension);
    bool unregisterFileType(const QString& fileType);
    bool registerFileType(const QString& filetype,
                            const QString& description,
                            const QString& verb,
                            const QString& serverApp,
                            int icon);
private:
    QList<PFileAssociationItem> mItems;


    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
};

class EnvironmentFileAssociationWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EnvironmentFileAssociationWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EnvironmentFileAssociationWidget();

private:
    Ui::EnvironmentFileAssociationWidget *ui;
    FileAssociationModel mModel;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};
#endif // ENVIRONMENTFILEASSOCIATIONWIDGET_H
