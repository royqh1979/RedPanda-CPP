#ifndef TOOLSGENERALWIDGET_H
#define TOOLSGENERALWIDGET_H

#include <QAbstractListModel>
#include <QWidget>
#include "settingswidget.h"
#include "../widgets/macroinfomodel.h"
#include "../toolsmanager.h"

namespace Ui {
class ToolsGeneralWidget;
}

class ToolsModel: public QAbstractListModel {
public:
    explicit ToolsModel(QObject* parent = nullptr);

    const QList<PToolItem> &tools() const;
    void setTools(const QList<PToolItem> &newTools);
    void addTool(PToolItem item);
    PToolItem getTool(int index);
    void removeTool(int index);

private:
    QList<PToolItem> mTools;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};


class ToolsGeneralWidget : public SettingsWidget
{
    Q_OBJECT
public:
    enum class EditType {
        Add,
        Edit,
        None
    };
    explicit ToolsGeneralWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ToolsGeneralWidget();
private:
    void onToolsCurrentChanged();
private:
    void finishEditing(bool askSave);
    void prepareEdit();
private slots:
    void updateDemo();
    void on_btnAdd_clicked();

    void on_btnEditOk_clicked();

    void on_btnEditCancel_clicked();

    void on_btnRemove_clicked();

    void on_btnInsertMacro_clicked();

    void on_btnBrowseWorkingDirectory_clicked();

    void on_btnBrowseProgram_clicked();

private:
    Ui::ToolsGeneralWidget *ui;
    MacroInfoModel mMacroInfoModel;
    ToolsModel mToolsModel;
    EditType mEditType;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;

    // SettingsWidget interface
protected:
    void updateIcons(const QSize &size) override;
};

#endif // TOOLSGENERALWIDGET_H
