#ifndef EDITORSTABWIDGET_H
#define EDITORSTABWIDGET_H

#include <QTabWidget>

class QDragEnterEvent;
class QDropEvent;

class EditorsTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    explicit EditorsTabWidget(QWidget* parent=nullptr);

    // QWidget interface
protected:
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
};

#endif // EDITORSTABWIDGET_H
