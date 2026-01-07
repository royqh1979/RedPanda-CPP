#ifndef BASE_REFORMATTER_H
#define BASE_REFORMATTER_H
#include <QObject>

class BaseReformatter : public QObject {
    Q_OBJECT
public:
    BaseReformatter(QObject* parent = nullptr);
    virtual QString refomat(const QString& content, QString &errorMessage, bool &isOk) = 0;
};

#endif
