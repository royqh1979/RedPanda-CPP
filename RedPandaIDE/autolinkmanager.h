#ifndef AUTOLINKMANAGER_H
#define AUTOLINKMANAGER_H

#include <QObject>
#include <QString>
#include <memory>
#include <QVector>
#include <QMap>

#define AUTOLINK_CONFIG "autolink.json"

struct Autolink {
    QString header;
    QString linkOption;
};
using PAutolink = std::shared_ptr<Autolink>;

class AutolinkManager
{
public:
    explicit AutolinkManager();
    PAutolink getLink(const QString& header) const;
    void load();
    void save();
    void setLink(const QString& header,
                       const QString& linkOption);
    void removeLink(const QString& header);
    const QMap<QString,PAutolink>& links() const;
    void clear();
    QJsonArray toJson();
    void fromJson(QJsonArray json);
private:
    QMap<QString,PAutolink> mLinks;
};

extern AutolinkManager* pAutolinkManager;

#endif // AUTOLINKMANAGER_H
