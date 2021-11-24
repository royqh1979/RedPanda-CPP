#ifndef SYMBOLUSAGEMANAGER_H
#define SYMBOLUSAGEMANAGER_H

#include <QObject>
#include <memory>
#include <QHash>
#include <QString>

struct SymbolUsage {
    QString fullName;
    int count;
};
using PSymbolUsage = std::shared_ptr<SymbolUsage>;

class SymbolUsageManager : public QObject
{
    Q_OBJECT
public:
    explicit SymbolUsageManager(QObject *parent = nullptr);
    void load();
    void save();
    void reset();
    PSymbolUsage findUsage(const QString& fullName) const;
    void updateUsage(const QString& symbol, int count);
private:
    QHash<QString, PSymbolUsage> mUsages;
};

using PSymbolUsageManager = std::shared_ptr<SymbolUsageManager>;

#endif // SYMBOLUSAGEMANAGER_H
