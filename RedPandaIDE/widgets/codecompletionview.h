#ifndef CODECOMPLETIONVIEW_H
#define CODECOMPLETIONVIEW_H

#include <QListView>
#include <QWidget>

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

class CodeCompletionView : public QWidget
{
    Q_OBJECT

public:
    explicit CodeCompletionView(QWidget *parent = nullptr);
    ~CodeCompletionView();

    void setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback);


private:
    CodeCompletionListView * mListView;
};

#endif // CODECOMPLETIONVIEW_H
