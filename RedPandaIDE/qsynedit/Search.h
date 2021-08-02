#ifndef SYNSEARCH_H
#define SYNSEARCH_H
#include "SearchBase.h"


class SynSearch : public SynSearchBase
{
    Q_OBJECT
public:
    explicit SynSearch(QObject* parent = nullptr);

    // SynSearchBase interface
public:
    int length(int aIndex) override;
    int result(int aIndex) override;
    int resultCount() override;
    int findAll(const QString &newText) override;
    QString replace(const QString &aOccurrence, const QString &aReplacement) override;
private:
    QList<int> mResults;
};

#endif // SYNSEARCH_H
