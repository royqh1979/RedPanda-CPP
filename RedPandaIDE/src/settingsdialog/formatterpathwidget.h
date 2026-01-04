#ifndef FORMATTERPATHWIDGET_H
#define FORMATTERPATHWIDGET_H

#include "settingswidget.h"
#include <QWidget>

namespace Ui {
class FormatterPathWidget;
}

class FormatterPathWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit FormatterPathWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~FormatterPathWidget();

private:
    Ui::FormatterPathWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
    void updateIcons(const QSize &size) override;
private slots:
    void on_btnChooseAstyle_clicked();
};

#endif // FORMATTERPATHWIDGET_H
