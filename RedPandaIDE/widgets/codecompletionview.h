#ifndef CODECOMPLETIONVIEW_H
#define CODECOMPLETIONVIEW_H

#include <QWidget>

namespace Ui {
class CodeCompletionView;
}

class CodeCompletionView : public QWidget
{
    Q_OBJECT

public:
    explicit CodeCompletionView(QWidget *parent = nullptr);
    ~CodeCompletionView();

private:
    Ui::CodeCompletionView *ui;
};

#endif // CODECOMPLETIONVIEW_H
