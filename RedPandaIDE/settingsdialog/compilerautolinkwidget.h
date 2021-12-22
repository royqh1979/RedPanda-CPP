#ifndef COMPILERAUTOLINKWIDGET_H
#define COMPILERAUTOLINKWIDGET_H

#include <QAbstractTableModel>
#include <QWidget>
#include "settingswidget.h"
#include "../autolinkmanager.h"

namespace Ui {
class CompilerAutolinkWidget;
}

class CompilerAutolinkWidget;
class AutolinkModel: public QAbstractTableModel {
    Q_OBJECT
public:
    explicit AutolinkModel(CompilerAutolinkWidget* widget,QObject* parent=nullptr);

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool insertRows(int row, int count, const QModelIndex &parent) override;
    bool removeRows(int row, int count, const QModelIndex &parent) override;

    const QList<PAutolink> &links() const;
    void setLinks(const QMap<QString, PAutolink> &newLinks);
private:
    int findLink(const QString& header);
private:
    QList<PAutolink> mLinks;
    CompilerAutolinkWidget * mWidget;

};

class CompilerAutolinkWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit CompilerAutolinkWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~CompilerAutolinkWidget();

private:
    AutolinkModel mModel;
private:
    Ui::CompilerAutolinkWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnAdd_pressed();
    void on_btnRemove_pressed();

    // SettingsWidget interface
protected:
    void updateIcons(const QSize &size) override;
};

#endif // COMPILERAUTOLINKWIDGET_H
