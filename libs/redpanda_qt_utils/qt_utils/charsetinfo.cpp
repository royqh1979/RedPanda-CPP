/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "charsetinfo.h"
#include <QObject>
#include <memory>
#include <QMap>
#include <QSet>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

CharsetInfoManager* pCharsetInfoManager;

QByteArray CharsetInfoManager::getDefaultSystemEncoding()
{
#ifdef Q_OS_WIN
    DWORD acp = GetACP();
    PCharsetInfo info = findCharsetByCodepage(acp);
    if (info) {
        return info->name;
    }
    return "unknown";
#else
    return "UTF-8";
#endif
}

PCharsetInfo CharsetInfoManager::findCharsetByCodepage(int codepage)
{
    foreach (const PCharsetInfo& info, mCodePages) {
        if (info->codepage == codepage)
            return info;
    }
    return PCharsetInfo();
}

QStringList CharsetInfoManager::languageNames()
{
    QSet<QString> languages;
    foreach (const PCharsetInfo& info, mCodePages) {
        if (info->enabled)
            languages.insert(info->language);
    }
    QStringList lst;
    foreach (const QString& s, languages)
        lst.append(s);
    lst.sort(Qt::CaseInsensitive);
    return lst;
}

QList<PCharsetInfo> CharsetInfoManager::findCharsetsByLanguageName(const QString &languageName)
{
    QList<PCharsetInfo> result;
    foreach (const PCharsetInfo& info, mCodePages) {
        if (info->enabled && info->language == languageName)
            result.append(info);
    }
    std::sort(result.begin(),result.end(),[](const PCharsetInfo& info1,const PCharsetInfo& info2){
        return (info1->name < info2->name);
    });
    return result;
}

QList<PCharsetInfo> CharsetInfoManager::findCharsetByLocale(const QString &localeName)
{
    QList<PCharsetInfo> result;
    foreach (const PCharsetInfo& info, mCodePages) {
        if (info->enabled && info->localeName == localeName)
            result.append(info);
    }
    return result;
}

QString CharsetInfoManager::findLanguageByCharsetName(const QString &encodingName)
{

    foreach (const PCharsetInfo& info, mCodePages) {
        if (info->enabled &&
                QString::compare(info->name, encodingName, Qt::CaseInsensitive)==0)
            return info->language;
    }
    return "Unknown";
}

const QString &CharsetInfoManager::localeName() const
{
    return mLocaleName;
}

CharsetInfoManager::CharsetInfoManager(const QString& localeName):
    QObject(),
    mLocaleName(localeName)
{
    mCodePages.append(std::make_shared<CharsetInfo>(37,"IBM037","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(437,"IBM437","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(500,"IBM500","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(708,"ASMO-708","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(709,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(710,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(720,"DOS-720",tr("Arabic"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(737,"ibm737",tr("Greek"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(775,"ibm775",tr("Baltic"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(850,"ibm850",tr("Western Europe"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(852,"ibm852",tr("Central Europe"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(855,"IBM855",tr("Cyrillic"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(857,"ibm857",tr("Turkish"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(858,"ibm858",tr("Western Europe"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(860,"IBM860",tr("Western Europe"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(861,"ibm861",tr("Northern Europe"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(862,"DOS-862",tr("Hebrew"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(863,"IBM863",tr("Western Europe"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(864,"IBM864","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(865,"IBM865",tr("Northern Europe"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(866,"cp866",tr("Cyrillic"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(869,"ibm869",tr("Greek"),"",false));
    mCodePages.append(std::make_shared<CharsetInfo>(870,"IBM870","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(874,"tis-620",tr("Thai"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(875,"cp875","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(932,"shift_jis",tr("Japanese"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(936,"gbk",tr("Chinese"),"zh_CN",true));
    mCodePages.append(std::make_shared<CharsetInfo>(949,"windows-949",tr("Korean"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(950,"big5",tr("Chinese"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(1026,"IBM1026","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1047,"IBM01047","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1140,"IBM01140","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1141,"IBM01141","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1142,"IBM01142","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1143,"IBM01143","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1144,"IBM01144","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1145,"IBM01145","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1146,"IBM01146","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1147,"IBM01147","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1148,"IBM01148","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1149,"IBM01149","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1200,"utf-16",tr("Unicode"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(1201,"unicodeFFFE","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(1250,"windows-1250",tr("Central Europe"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(1251,"windows-1251",tr("Cyrillic"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(1252,"windows-1252",tr("Western Europe"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(1253,"windows-1253",tr("Greek"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(1254,"windows-1254",tr("Turkish"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(1255,"windows-1255",tr("Hebrew"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(1256,"windows-1256",tr("Arabic"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(1257,"windows-1257",tr("Baltic"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(1258,"windows-1258",tr("Vietnamese"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(1361,"Johab","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10000,"macintosh",tr("Cyrillic"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(10001,"x-mac-japanese","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10002,"x-mac-chinesetrad","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10003,"x-mac-korean","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10004,"x-mac-arabic","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10005,"x-mac-hebrew","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10006,"x-mac-greek","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10007,"x-mac-cyrillic","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10008,"x-mac-chinesesimp","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10010,"x-mac-romanian","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10017,"x-mac-ukrainian","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10021,"x-mac-thai","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10029,"x-mac-ce","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10079,"x-mac-icelandic","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10081,"x-mac-turkish","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(10082,"x-mac-croatian","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(12000,"utf-32",tr("Unicode"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(12001,"utf-32BE","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20000,"x-Chinese_CNS","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20001,"x-cp20001","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20002,"x_Chinese-Eten","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20003,"x-cp20003","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20004,"x-cp20004","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20005,"x-cp20005","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20105,"x-IA5","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20106,"x-IA5-German","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20107,"x-IA5-Swedish","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20108,"x-IA5-Norwegian","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20127,"us-ascii","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20261,"x-cp20261","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20269,"x-cp20269","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20273,"IBM273","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20277,"IBM277","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20278,"IBM278","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20280,"IBM280","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20284,"IBM284","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20285,"IBM285","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20290,"IBM290","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20297,"IBM297","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20420,"IBM420","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20423,"IBM423","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20424,"IBM424","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20833,"x-EBCDIC-KoreanExtended","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20838,"IBM-Thai","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20866,"koi8-r",tr("Cyrillic"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(20871,"IBM871","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20880,"IBM880","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20905,"IBM905","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20924,"IBM00924","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20932,"EUC-JP","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20936,"x-cp20936","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(20949,"x-cp20949","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(21025,"cp1025","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(21027,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(21866,"koi8-u",tr("Cyrillic"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(28591,"iso-8859-1",tr("Western Europe"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(28592,"iso-8859-2",tr("Eastern Europe"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(28593,"iso-8859-3",tr("Turkish"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(28594,"iso-8859-4",tr("Baltic"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(28595,"iso-8859-5",tr("Cyrillic"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(28596,"iso-8859-6",tr("Arabic"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(28597,"iso-8859-7",tr("Greek"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(28598,"iso-8859-8",tr("Hebrew"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(28599,"iso-8859-9",tr("Turkish"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(28603,"iso-8859-13",tr("Baltic"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(-1,"iso-8859-14",tr("Celtic"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(28605,"iso-8859-15",tr("Western Europe"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(29001,"x-Europa","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(38598,"iso-8859-8-i","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50220,"iso-2022-jp","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50221,"csISO2022JP","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50222,"iso-2022-jp","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50225,"iso-2022-kr","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50227,"x-cp50227","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50229,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50930,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50931,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50933,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50935,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50936,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50937,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(50939,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(51932,"euc-jp",tr("Japanese"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(51936,"euc-cn","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(51949,"euc-kr",tr("Korean"),"",true));
    mCodePages.append(std::make_shared<CharsetInfo>(51950,"","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(52936,"hz-gb-2312","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(54936,"gb18030",tr("Chinese"),"zh_CN",true));
    mCodePages.append(std::make_shared<CharsetInfo>(57002,"x-iscii-de","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(57003,"x-iscii-be","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(57004,"x-iscii-ta","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(57005,"x-iscii-te","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(57006,"x-iscii-as","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(57007,"x-iscii-or","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(57008,"x-iscii-ka","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(57009,"x-iscii-ma","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(57010,"x-iscii-gu","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(57011,"x-iscii-pa","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(65000,"utf-7","","",false));
    mCodePages.append(std::make_shared<CharsetInfo>(65001,"utf-8",tr("Unicode"),"",true));

}

CharsetInfo::CharsetInfo(int codepage, const QByteArray &name, const QString &language,const QString& localeName, bool enabled)
{
    this->codepage = codepage;
    this->name = name;
    this->language = language;
    this->localeName = localeName;
    this->enabled = enabled;
}
