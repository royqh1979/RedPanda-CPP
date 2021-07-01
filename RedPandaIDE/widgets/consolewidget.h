#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QTextEdit>

class ConsoleWidget : public QTextEdit
{
    Q_OBJECT
public:
    explicit ConsoleWidget(QWidget* parent = nullptr);
private:
    QString mCurrentCommand;
    QStringList mCommandHistory;
    int mHistoryIndex;
    int mHistorySize;
    QString mPrompt;
};

#endif // CONSOLEWIDGET_H
