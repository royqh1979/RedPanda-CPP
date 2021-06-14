#ifndef EDITORCOLORSCHEMEWIDGET_H
#define EDITORCOLORSCHEMEWIDGET_H

#include <QWidget>

namespace Ui {
class EditorColorSchemeWidget;
}

class EditorColorSchemeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditorColorSchemeWidget(QWidget *parent = nullptr);
    ~EditorColorSchemeWidget();

private:
    Ui::EditorColorSchemeWidget *ui;
};

#endif // EDITORCOLORSCHEMEWIDGET_H
