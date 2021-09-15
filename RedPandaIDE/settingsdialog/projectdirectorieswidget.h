#ifndef PROJECTDIRECTORIESWIDGET_H
#define PROJECTDIRECTORIESWIDGET_H

#include <QWidget>
#include "settingswidget.h"
#include "compilersetdirectorieswidget.h"

namespace Ui {
class ProjectDirectoriesWidget;
}

class CompilerSetDirectoriesWidget;
class ProjectDirectoriesWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectDirectoriesWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectDirectoriesWidget();

private:
    Ui::ProjectDirectoriesWidget *ui;
    CompilerSetDirectoriesWidget *mLibDirWidget;
    CompilerSetDirectoriesWidget *mIncludeDirWidget;
    CompilerSetDirectoriesWidget *mResourceDirWidget;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // PROJECTDIRECTORIESWIDGET_H
