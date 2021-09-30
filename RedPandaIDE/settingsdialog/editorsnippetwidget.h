#ifndef EDITORSNIPPETWIDGET_H
#define EDITORSNIPPETWIDGET_H

#include <QWidget>
#include "settingswidget.h"
#include "../codesnippetsmanager.h"

namespace Ui {
class EditorSnippetWidget;
}

class EditorSnippetWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorSnippetWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorSnippetWidget();

private:
    Ui::EditorSnippetWidget *ui;
    CodeSnippetsModel mModel;
private:
    bool mUpdatingCode;
    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_tblSnippets_clicked(const QModelIndex &index);
    void on_btnAdd_triggered(QAction *arg1);
    void on_btnRemove_triggered(QAction *arg1);
};

#endif // EDITORSNIPPETWIDGET_H
