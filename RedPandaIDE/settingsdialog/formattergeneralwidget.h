#ifndef FORMATTERGENERALWIDGET_H
#define FORMATTERGENERALWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class FormatterGeneralWidget;
}

class FormatterGeneralWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit FormatterGeneralWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~FormatterGeneralWidget();

private:
    Ui::FormatterGeneralWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // FORMATTERGENERALWIDGET_H
