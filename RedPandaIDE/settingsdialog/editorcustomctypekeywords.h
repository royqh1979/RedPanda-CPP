#ifndef EDITORCUSTOMCTYPEKEYWORDS_H
#define EDITORCUSTOMCTYPEKEYWORDS_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class editorcustomctypekeywords;
}

class QListWidgetItem;
class EditorCustomCTypeKeywordsWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorCustomCTypeKeywordsWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorCustomCTypeKeywordsWidget();

private:
    Ui::editorcustomctypekeywords *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;

    // SettingsWidget interface
protected:
    void updateIcons(const QSize &size) override;
    QListWidgetItem * addKeyword(const QString& keyword) ;

private slots:
    void on_btnAdd_clicked();
    void on_btnRemove_clicked();
    void on_btnRemoveAll_clicked();
};

#endif // EDITORCUSTOMCTYPEKEYWORDS_H
