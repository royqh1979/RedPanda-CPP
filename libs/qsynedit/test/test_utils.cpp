#include "test_utils.h"

TokenInfoList parseLine(QSynedit::Syntaxer *syntaxer, const QString &line)
{
    TokenInfoList result;
    syntaxer->resetState();
    syntaxer->setLine(0,line,0);
    while(!syntaxer->eol()) {
        PTokenInfo info = std::make_shared<TokenInfo>();
        info->token = syntaxer->getToken();
        info->attribute = syntaxer->getTokenAttribute();
        info->startChar = syntaxer->getTokenPos();
        info->state = syntaxer->getState();
        result.append(info);
        syntaxer->next();
    }
    return result;
}

QList<TokenInfoList> parseLines(QSynedit::Syntaxer *syntaxer, const QStringList &text)
{
    QList<TokenInfoList> result;
    syntaxer->resetState();
    for (int i=0;i<text.size();i++) {
        TokenInfoList list;
        syntaxer->setLine(i,text[i],i);
        while(!syntaxer->eol()) {
            PTokenInfo info = std::make_shared<TokenInfo>();
            info->token = syntaxer->getToken();
            info->attribute = syntaxer->getTokenAttribute();
            info->startChar = syntaxer->getTokenPos();
            info->state = syntaxer->getState();
            list.append(info);
            syntaxer->next();
        }
        result.append(list);
    }
    return result;
}


QStringList filterTokens(const QList<TokenInfoList> &infoList, const std::unordered_set<QSynedit::PTokenAttribute> &targetAttributes)
{
    QList<QString> result;
    for (int i=0;i<infoList.size();i++) {
        for (int j=0;j<infoList[i].size();j++) {
            if (targetAttributes.find(infoList[i][j]->attribute)!=targetAttributes.end())
                result.append(infoList[i][j]->token);
        }
    }
    return result;
}

QStringList filterTokens(const QList<TokenInfoList> &infoList, const QSynedit::PTokenAttribute &targetAttribute)
{
    QList<QString> result;
    for (int i=0;i<infoList.size();i++) {
        for (int j=0;j<infoList[i].size();j++) {
            if (targetAttribute == infoList[i][j]->attribute)
                result.append(infoList[i][j]->token);
        }
    }
    return result;
}
