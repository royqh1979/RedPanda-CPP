#ifndef EXECUTORPROBLEMSETWIDGET_H
#define EXECUTORPROBLEMSETWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ExecutorProblemSetWidget;
}

class ExecutorProblemSetWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ExecutorProblemSetWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ExecutorProblemSetWidget();

private:
    Ui::ExecutorProblemSetWidget *ui;
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // EXECUTORPROBLEMSETWIDGET_H
