#ifndef CODECOMPLETIONLISTVIEW_H
#define CODECOMPLETIONLISTVIEW_H

#include <QListView>
#include <QKeyEvent>
#include "../parser/parserutils.h"
using KeyPressedCallback = std::function<bool (QKeyEvent *)>;

class CodeCompletionListView: public QListView {
    Q_OBJECT
public:
    explicit CodeCompletionListView(QWidget *parent = nullptr);

    // QWidget interface
    const KeyPressedCallback &keypressedCallback() const;
    void setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback);

protected:
    void keyPressEvent(QKeyEvent *event) override;
private:
    KeyPressedCallback mKeypressedCallback;
};


#endif // CODECOMPLETIONLISTVIEW_H
