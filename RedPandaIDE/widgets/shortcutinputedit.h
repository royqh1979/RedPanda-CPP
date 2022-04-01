#ifndef SHORTCUTINPUTEDIT_H
#define SHORTCUTINPUTEDIT_H

#include <QLineEdit>

class ShortcutInputEdit : public QLineEdit
{
    Q_OBJECT
public:
    ShortcutInputEdit(QWidget* parent);
signals:
    void inputFinished(QWidget* editor);
    // QWidget interface
protected:
    void keyPressEvent(QKeyEvent *event) override;

    // QObject interface
public:
    bool event(QEvent *event) override;
};

#endif // SHORTCUTINPUTEDIT_H
