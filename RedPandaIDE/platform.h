#ifndef PLATFORM_H
#define PLATFORM_H
#include <QByteArray>
#include <QString>
#include <memory>
#include <QObject>

struct CharsetInfo{
    int codepage;
    QByteArray name;
    QString language;
    bool enabled;
    explicit CharsetInfo(int codepage,
                         const QByteArray& name,
                         const QString& language,
                         bool enabled);
};

using PCharsetInfo = std::shared_ptr<CharsetInfo>;

class CharsetInfoManager: public QObject {
    Q_OBJECT;
public:
    explicit CharsetInfoManager();
    QByteArray getDefaultSystemEncoding();
    PCharsetInfo findCharsetByCodepage(int codepage);
    QStringList languageNames();
    QList<PCharsetInfo> findCharsetsByLanguageName(const QString& languageName);
private:
    static QList<PCharsetInfo> mCodePages;
};

using PCharsetInfoManager = std::shared_ptr<CharsetInfo>;

extern CharsetInfoManager* pCharsetInfoManager;

#endif // PLATFORM_H
