#ifndef EDITORCOLORSCHEMEWIDGET_H
#define EDITORCOLORSCHEMEWIDGET_H

#include "settingswidget.h"
#include "../colorscheme.h"

#include <QMenu>
#include <QStandardItemModel>

namespace Ui {
class EditorColorSchemeWidget;
}

class EditorColorSchemeWidget : public SettingsWidget
{
    Q_OBJECT

public:
    enum {
        NameRole = Qt::UserRole+1
    };
    explicit EditorColorSchemeWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorColorSchemeWidget();

public slots:
    void onItemSelectionChanged();
    void onSettingChanged();
    void onForegroundChanged();
    void onBackgroundChanged();
    void onFontStyleChanged();
    void changeSchemeComboFont();

private:
    void addDefine(const QString& name, PColorSchemeItemDefine define);
    PColorSchemeItem getCurrentItem();
    PColorScheme getCurrentScheme();
    void connectModificationSlots();
    void disconnectModificationSlots();
    void setCurrentSchemeModified();

private:
    Ui::EditorColorSchemeWidget *ui;
    QStandardItemModel mDefinesModel;
    QFont mDefaultSchemeComboFont;
    QFont mModifiedSchemeComboFont;
    QSet<QString> mModifiedSchemes;
    QMenu mMenu;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;

private slots:
    void on_actionCopy_Scheme_triggered();
    void on_btnSchemeMenu_pressed();
    void on_actionImport_Scheme_triggered();
    void on_actionRename_Scheme_triggered();
    void on_actionReset_Scheme_triggered();
    void on_actionExport_Scheme_triggered();
    void on_actionDelete_Scheme_triggered();
};

#endif // EDITORCOLORSCHEMEWIDGET_H
