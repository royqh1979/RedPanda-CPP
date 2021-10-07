#ifndef TOOLSMANAGER_H
#define TOOLSMANAGER_H

#include <QObject>
#include <memory>

struct ToolItem {
    QString title;
    QString program;
    QString workingDirectory;
    QString parameters;
    bool pauseAfterExit;
};

using PToolItem = std::shared_ptr<ToolItem>;

class ToolsManager : public QObject
{
    Q_OBJECT
public:
    explicit ToolsManager(QObject *parent = nullptr);
    void load();
    void save();
    const QList<PToolItem> &tools() const;
    PToolItem findTool(const QString& title);
    void setTools(const QList<PToolItem> &newTools);

signals:
private:
    QList<PToolItem> mTools;
};

using PToolsManager = std::shared_ptr<ToolsManager>;

#endif // TOOLSMANAGER_H
