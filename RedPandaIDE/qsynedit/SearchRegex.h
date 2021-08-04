#ifndef SYNSEARCHREGEX_H
#define SYNSEARCHREGEX_H
#include "SearchBase.h"

#include <QRegularExpression>

class SynSearchRegex : public SynSearchBase
{
    Q_OBJECT
public:
    explicit SynSearchRegex(QObject* parent=nullptr);

    // SynSearchBase interface
public:
    int length(int aIndex) override;
    int result(int aIndex) override;
    int resultCount() override;
    int findAll(const QString &keyword) override;
    QString replace(const QString &aOccurrence, const QString &aReplacement) override;
    void setPattern(const QString &value) override;
    void setOptions(const SynSearchOptions &options) override;
private:
    void updateRegexOptions();
private:
    QRegularExpression mRegex;
    QList<int> mLengths;
    QList<int> mResults;
};

#endif // SYNSEARCHREGEX_H
