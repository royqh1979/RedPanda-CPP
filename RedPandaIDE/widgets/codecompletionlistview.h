#ifndef CODECOMPLETIONLISTVIEW_H
#define CODECOMPLETIONLISTVIEW_H

#include <QListView>
#include <QKeyEvent>
#include "../parser/parserutils.h"
using KeyPressedCallback = std::function<bool (QKeyEvent *)>;
using InputMethodCallback = std::function<bool (QInputMethodEvent*)>;

class CodeCompletionListView: public QListView {
    Q_OBJECT
public:
    explicit CodeCompletionListView(QWidget *parent = nullptr);

    // QWidget interface
    const KeyPressedCallback &keypressedCallback() const;
    void setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback);

    const InputMethodCallback &inputMethodCallback() const;
    void setInputMethodCallback(const InputMethodCallback &newInputMethodCallback);

protected:
    void keyPressEvent(QKeyEvent *event) override;
private:
    KeyPressedCallback mKeypressedCallback;

    // QWidget interface
protected:
    void focusInEvent(QFocusEvent *event) override;
};


#endif // CODECOMPLETIONLISTVIEW_H
