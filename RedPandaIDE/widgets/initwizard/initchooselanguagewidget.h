#ifndef INITCHOOSELANGUAGEWIDGET_H
#define INITCHOOSELANGUAGEWIDGET_H

#include <QDialog>

namespace Ui {
class InitChooseLanguageWidget;
}

class InitChooseLanguageWidget : public QDialog
{
    Q_OBJECT

public:
    explicit InitChooseLanguageWidget(QWidget *parent = nullptr);
    ~InitChooseLanguageWidget();

private:
    Ui::InitChooseLanguageWidget *ui;
};

#endif // INITCHOOSELANGUAGEWIDGET_H
