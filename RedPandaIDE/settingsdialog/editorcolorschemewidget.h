#ifndef EDITORCOLORSCHEMEWIDGET_H
#define EDITORCOLORSCHEMEWIDGET_H

#include "settingswidget.h"
#include "../colorscheme.h"

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

private:
    void addDefine(const QString& name, PColorSchemeItemDefine define);
    PColorSchemeItem getCurrentItem();
    PColorScheme getCurrentScheme();

private:
    Ui::EditorColorSchemeWidget *ui;
    QStandardItemModel mDefinesModel;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;

};

#endif // EDITORCOLORSCHEMEWIDGET_H
