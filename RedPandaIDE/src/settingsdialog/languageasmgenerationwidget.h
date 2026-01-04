#ifndef LANGUAGEASMGENERATIONWIDGET_H
#define LANGUAGEASMGENERATIONWIDGET_H

#include "settingswidget.h"

namespace Ui {
class LanguageAsmGenerationWidget;
}

class LanguageAsmGenerationWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit LanguageAsmGenerationWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~LanguageAsmGenerationWidget();

private:
    Ui::LanguageAsmGenerationWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // LANGUAGEASMGENERATIONWIDGET_H
