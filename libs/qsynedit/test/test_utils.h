#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <QObject>
#include <QSet>
#include <QHash>
#include <memory>
#include <unordered_set>
#include "qsynedit/syntaxer/syntaxer.h"

struct TokenInfo{
    QString token;
    QSynedit::PTokenAttribute attribute;
    QSynedit::PSyntaxState state;
    int startChar;
};

using PTokenInfo = std::shared_ptr<TokenInfo>;

using TokenInfoList = QList<PTokenInfo>;

TokenInfoList parseLine(QSynedit::Syntaxer *syntaxer,
                            const QString& line);

QList<TokenInfoList> parseLines(QSynedit::Syntaxer *syntaxer,
                                 const QStringList &text);

QStringList filterTokens(const QList<TokenInfoList> &infoList,
                            const std::unordered_set<QSynedit::PTokenAttribute> &targetAttributes);
QStringList filterTokens(const QList<TokenInfoList> &infoList,
                            const QSynedit::PTokenAttribute &targetAttribute);
#endif // TEST_UTILS_H
