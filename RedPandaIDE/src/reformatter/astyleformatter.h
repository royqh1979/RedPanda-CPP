#ifndef ASTYLE_REFORMATTER_H
#define ASTYLE_REFORMATTER_H
#include "basereformatter.h"
#include "../utils/types.h"

class AStyleReformatter : public BaseReformatter {
    Q_OBJECT
public:
    AStyleReformatter(const QString& astylePath, const QStringList& args, LoggerFunc newLoggerFunc, QObject* parent = nullptr);
    QString refomat(const QString& content, QString &errorMessage, bool &isOk) override;
private:
    QString mAstylePath;
    QStringList mArgs;
    LoggerFunc mLoggerFunc;
};

#endif
