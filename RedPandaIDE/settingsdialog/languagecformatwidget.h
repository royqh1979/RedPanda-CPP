#ifndef LANGUAGECFORMATWIDGET_H
#define LANGUAGECFORMATWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class LanguageCFormatWidget;
}

class LanguageCFormatWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit LanguageCFormatWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~LanguageCFormatWidget();

private:
    Ui::LanguageCFormatWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // LANGUAGECFORMATWIDGET_H
