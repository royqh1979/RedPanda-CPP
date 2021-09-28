#include "platform.h"
#include <QObject>
#include <memory>
#include <QMap>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

CharsetInfoManager* pCharsetInfoManager;

static void initmCodePages(){
}

QByteArray CharsetInfoManager::getDefaultSystemEncoding()
{
    DWORD acp = GetACP();
    PCharsetInfo info = findCharsetByCodepage(acp);
    if (info) {
        return info->name;
    }
    return "unknown";
}

PCharsetInfo CharsetInfoManager::findCharsetByCodepage(int codepage)
{
    foreach (const PCharsetInfo& info, mCodePages) {
        if (info->codepage == codepage)
            return info;
    }
    return PCharsetInfo();
}

CharsetInfoManager::CharsetInfoManager():QObject()
{
    mCodePages.insert(37,std::make_shared<CharsetInfo>(37,"IBM037","",false));
    mCodePages.insert(437,std::make_shared<CharsetInfo>(437,"IBM437","",false));
    mCodePages.insert(500,std::make_shared<CharsetInfo>(500,"IBM500","",false));
    mCodePages.insert(708,std::make_shared<CharsetInfo>(708,"ASMO-708","",false));
    mCodePages.insert(709,std::make_shared<CharsetInfo>(709,"","",false));
    mCodePages.insert(710,std::make_shared<CharsetInfo>(710,"","",false));
    mCodePages.insert(720,std::make_shared<CharsetInfo>(720,"DOS-720",tr("Arabic"),false));
    mCodePages.insert(737,std::make_shared<CharsetInfo>(737,"ibm737",tr("Greek"),false));
    mCodePages.insert(775,std::make_shared<CharsetInfo>(775,"ibm775",tr("Baltic"),false));
    mCodePages.insert(850,std::make_shared<CharsetInfo>(850,"ibm850",tr("Western Europe"),false));
    mCodePages.insert(852,std::make_shared<CharsetInfo>(852,"ibm852",tr("Central Europe"),false));
    mCodePages.insert(855,std::make_shared<CharsetInfo>(855,"IBM855",tr("Cyrillic"),false));
    mCodePages.insert(857,std::make_shared<CharsetInfo>(857,"ibm857",tr("Turkish"),false));
    mCodePages.insert(858,std::make_shared<CharsetInfo>(858,"ibm858",tr("Western Europe"),false));
    mCodePages.insert(860,std::make_shared<CharsetInfo>(860,"IBM860",tr("Western Europe"),false));
    mCodePages.insert(861,std::make_shared<CharsetInfo>(861,"ibm861",tr("Northern Europe"),false));
    mCodePages.insert(862,std::make_shared<CharsetInfo>(862,"DOS-862",tr("Hebrew"),false));
    mCodePages.insert(863,std::make_shared<CharsetInfo>(863,"IBM863",tr("Western Europe"),false));
    mCodePages.insert(864,std::make_shared<CharsetInfo>(864,"IBM864","",false));
    mCodePages.insert(865,std::make_shared<CharsetInfo>(865,"IBM865",tr("Northern Europe"),false));
    mCodePages.insert(866,std::make_shared<CharsetInfo>(866,"cp866",tr("Cyrillic"),false));
    mCodePages.insert(869,std::make_shared<CharsetInfo>(869,"ibm869",tr("Greek"),false));
    mCodePages.insert(870,std::make_shared<CharsetInfo>(870,"IBM870","",false));
    mCodePages.insert(874,std::make_shared<CharsetInfo>(874,"tis-620",tr("Thai"),true));
    mCodePages.insert(875,std::make_shared<CharsetInfo>(875,"cp875","",false));
    mCodePages.insert(932,std::make_shared<CharsetInfo>(932,"shift_jis",tr("Japanese"),true));
    mCodePages.insert(936,std::make_shared<CharsetInfo>(936,"gbk",tr("Chinese"),true));
    mCodePages.insert(949,std::make_shared<CharsetInfo>(949,"windows-949",tr("Korean"),true));
    mCodePages.insert(950,std::make_shared<CharsetInfo>(950,"big5",tr("Chinese"),true));
    mCodePages.insert(1026,std::make_shared<CharsetInfo>(1026,"IBM1026","",false));
    mCodePages.insert(1047,std::make_shared<CharsetInfo>(1047,"IBM01047","",false));
    mCodePages.insert(1140,std::make_shared<CharsetInfo>(1140,"IBM01140","",false));
    mCodePages.insert(1141,std::make_shared<CharsetInfo>(1141,"IBM01141","",false));
    mCodePages.insert(1142,std::make_shared<CharsetInfo>(1142,"IBM01142","",false));
    mCodePages.insert(1143,std::make_shared<CharsetInfo>(1143,"IBM01143","",false));
    mCodePages.insert(1144,std::make_shared<CharsetInfo>(1144,"IBM01144","",false));
    mCodePages.insert(1145,std::make_shared<CharsetInfo>(1145,"IBM01145","",false));
    mCodePages.insert(1146,std::make_shared<CharsetInfo>(1146,"IBM01146","",false));
    mCodePages.insert(1147,std::make_shared<CharsetInfo>(1147,"IBM01147","",false));
    mCodePages.insert(1148,std::make_shared<CharsetInfo>(1148,"IBM01148","",false));
    mCodePages.insert(1149,std::make_shared<CharsetInfo>(1149,"IBM01149","",false));
    mCodePages.insert(1200,std::make_shared<CharsetInfo>(1200,"utf-16","",false));
    mCodePages.insert(1201,std::make_shared<CharsetInfo>(1201,"unicodeFFFE","",false));
    mCodePages.insert(1250,std::make_shared<CharsetInfo>(1250,"windows-1250",tr("Central Europe"),true));
    mCodePages.insert(1251,std::make_shared<CharsetInfo>(1251,"windows-1251",tr("Cyrillic"),true));
    mCodePages.insert(1252,std::make_shared<CharsetInfo>(1252,"windows-1252",tr("Western Europe"),true));
    mCodePages.insert(1253,std::make_shared<CharsetInfo>(1253,"windows-1253",tr("Greek"),true));
    mCodePages.insert(1254,std::make_shared<CharsetInfo>(1254,"windows-1254",tr("Turkish"),true));
    mCodePages.insert(1255,std::make_shared<CharsetInfo>(1255,"windows-1255",tr("Hebrew"),true));
    mCodePages.insert(1256,std::make_shared<CharsetInfo>(1256,"windows-1256",tr("Arabic"),true));
    mCodePages.insert(1257,std::make_shared<CharsetInfo>(1257,"windows-1257",tr("Baltic"),true));
    mCodePages.insert(1258,std::make_shared<CharsetInfo>(1258,"windows-1258",tr("Vietnamese"),true));
    mCodePages.insert(1361,std::make_shared<CharsetInfo>(1361,"Johab","",false));
    mCodePages.insert(10000,std::make_shared<CharsetInfo>(10000,"macintosh",tr("Cyrillic"),true));
    mCodePages.insert(10001,std::make_shared<CharsetInfo>(10001,"x-mac-japanese","",false));
    mCodePages.insert(10002,std::make_shared<CharsetInfo>(10002,"x-mac-chinesetrad","",false));
    mCodePages.insert(10003,std::make_shared<CharsetInfo>(10003,"x-mac-korean","",false));
    mCodePages.insert(10004,std::make_shared<CharsetInfo>(10004,"x-mac-arabic","",false));
    mCodePages.insert(10005,std::make_shared<CharsetInfo>(10005,"x-mac-hebrew","",false));
    mCodePages.insert(10006,std::make_shared<CharsetInfo>(10006,"x-mac-greek","",false));
    mCodePages.insert(10007,std::make_shared<CharsetInfo>(10007,"x-mac-cyrillic","",false));
    mCodePages.insert(10008,std::make_shared<CharsetInfo>(10008,"x-mac-chinesesimp","",false));
    mCodePages.insert(10010,std::make_shared<CharsetInfo>(10010,"x-mac-romanian","",false));
    mCodePages.insert(10017,std::make_shared<CharsetInfo>(10017,"x-mac-ukrainian","",false));
    mCodePages.insert(10021,std::make_shared<CharsetInfo>(10021,"x-mac-thai","",false));
    mCodePages.insert(10029,std::make_shared<CharsetInfo>(10029,"x-mac-ce","",false));
    mCodePages.insert(10079,std::make_shared<CharsetInfo>(10079,"x-mac-icelandic","",false));
    mCodePages.insert(10081,std::make_shared<CharsetInfo>(10081,"x-mac-turkish","",false));
    mCodePages.insert(10082,std::make_shared<CharsetInfo>(10082,"x-mac-croatian","",false));
    mCodePages.insert(12000,std::make_shared<CharsetInfo>(12000,"utf-32","",false));
    mCodePages.insert(12001,std::make_shared<CharsetInfo>(12001,"utf-32BE","",false));
    mCodePages.insert(20000,std::make_shared<CharsetInfo>(20000,"x-Chinese_CNS","",false));
    mCodePages.insert(20001,std::make_shared<CharsetInfo>(20001,"x-cp20001","",false));
    mCodePages.insert(20002,std::make_shared<CharsetInfo>(20002,"x_Chinese-Eten","",false));
    mCodePages.insert(20003,std::make_shared<CharsetInfo>(20003,"x-cp20003","",false));
    mCodePages.insert(20004,std::make_shared<CharsetInfo>(20004,"x-cp20004","",false));
    mCodePages.insert(20005,std::make_shared<CharsetInfo>(20005,"x-cp20005","",false));
    mCodePages.insert(20105,std::make_shared<CharsetInfo>(20105,"x-IA5","",false));
    mCodePages.insert(20106,std::make_shared<CharsetInfo>(20106,"x-IA5-German","",false));
    mCodePages.insert(20107,std::make_shared<CharsetInfo>(20107,"x-IA5-Swedish","",false));
    mCodePages.insert(20108,std::make_shared<CharsetInfo>(20108,"x-IA5-Norwegian","",false));
    mCodePages.insert(20127,std::make_shared<CharsetInfo>(20127,"us-ascii","",false));
    mCodePages.insert(20261,std::make_shared<CharsetInfo>(20261,"x-cp20261","",false));
    mCodePages.insert(20269,std::make_shared<CharsetInfo>(20269,"x-cp20269","",false));
    mCodePages.insert(20273,std::make_shared<CharsetInfo>(20273,"IBM273","",false));
    mCodePages.insert(20277,std::make_shared<CharsetInfo>(20277,"IBM277","",false));
    mCodePages.insert(20278,std::make_shared<CharsetInfo>(20278,"IBM278","",false));
    mCodePages.insert(20280,std::make_shared<CharsetInfo>(20280,"IBM280","",false));
    mCodePages.insert(20284,std::make_shared<CharsetInfo>(20284,"IBM284","",false));
    mCodePages.insert(20285,std::make_shared<CharsetInfo>(20285,"IBM285","",false));
    mCodePages.insert(20290,std::make_shared<CharsetInfo>(20290,"IBM290","",false));
    mCodePages.insert(20297,std::make_shared<CharsetInfo>(20297,"IBM297","",false));
    mCodePages.insert(20420,std::make_shared<CharsetInfo>(20420,"IBM420","",false));
    mCodePages.insert(20423,std::make_shared<CharsetInfo>(20423,"IBM423","",false));
    mCodePages.insert(20424,std::make_shared<CharsetInfo>(20424,"IBM424","",false));
    mCodePages.insert(20833,std::make_shared<CharsetInfo>(20833,"x-EBCDIC-KoreanExtended","",false));
    mCodePages.insert(20838,std::make_shared<CharsetInfo>(20838,"IBM-Thai","",false));
    mCodePages.insert(20866,std::make_shared<CharsetInfo>(20866,"koi8-r",tr("Cyrillic"),true));
    mCodePages.insert(20871,std::make_shared<CharsetInfo>(20871,"IBM871","",false));
    mCodePages.insert(20880,std::make_shared<CharsetInfo>(20880,"IBM880","",false));
    mCodePages.insert(20905,std::make_shared<CharsetInfo>(20905,"IBM905","",false));
    mCodePages.insert(20924,std::make_shared<CharsetInfo>(20924,"IBM00924","",false));
    mCodePages.insert(20932,std::make_shared<CharsetInfo>(20932,"EUC-JP","",false));
    mCodePages.insert(20936,std::make_shared<CharsetInfo>(20936,"x-cp20936","",false));
    mCodePages.insert(20949,std::make_shared<CharsetInfo>(20949,"x-cp20949","",false));
    mCodePages.insert(21025,std::make_shared<CharsetInfo>(21025,"cp1025","",false));
    mCodePages.insert(21027,std::make_shared<CharsetInfo>(21027,"","",false));
    mCodePages.insert(21866,std::make_shared<CharsetInfo>(21866,"koi8-u",tr("Cyrillic"),true));
    mCodePages.insert(28591,std::make_shared<CharsetInfo>(28591,"iso-8859-1",tr("Western Europe"),true));
    mCodePages.insert(28592,std::make_shared<CharsetInfo>(28592,"iso-8859-2",tr("Eastern Europe"),true));
    mCodePages.insert(28593,std::make_shared<CharsetInfo>(28593,"iso-8859-3",tr("Turkish"),true));
    mCodePages.insert(28594,std::make_shared<CharsetInfo>(28594,"iso-8859-4",tr("Baltic"),true));
    mCodePages.insert(28595,std::make_shared<CharsetInfo>(28595,"iso-8859-5",tr("Cyrillic"),true));
    mCodePages.insert(28596,std::make_shared<CharsetInfo>(28596,"iso-8859-6",tr("Arabic"),true));
    mCodePages.insert(28597,std::make_shared<CharsetInfo>(28597,"iso-8859-7",tr("Greek"),true));
    mCodePages.insert(28598,std::make_shared<CharsetInfo>(28598,"iso-8859-8",tr("Hebrew"),true));
    mCodePages.insert(28599,std::make_shared<CharsetInfo>(28599,"iso-8859-9",tr("Turkish"),true));
    mCodePages.insert(28603,std::make_shared<CharsetInfo>(28603,"iso-8859-13",tr("Baltic"),true));
    mCodePages.insert(-1,std::make_shared<CharsetInfo>(28605,"iso-8859-14",tr("Celtic"),true));
    mCodePages.insert(28605,std::make_shared<CharsetInfo>(28605,"iso-8859-15",tr("Western Europe"),true));
    mCodePages.insert(29001,std::make_shared<CharsetInfo>(29001,"x-Europa","",false));
    mCodePages.insert(38598,std::make_shared<CharsetInfo>(38598,"iso-8859-8-i","",false));
    mCodePages.insert(50220,std::make_shared<CharsetInfo>(50220,"iso-2022-jp","",false));
    mCodePages.insert(50221,std::make_shared<CharsetInfo>(50221,"csISO2022JP","",false));
    mCodePages.insert(50222,std::make_shared<CharsetInfo>(50222,"iso-2022-jp","",false));
    mCodePages.insert(50225,std::make_shared<CharsetInfo>(50225,"iso-2022-kr","",false));
    mCodePages.insert(50227,std::make_shared<CharsetInfo>(50227,"x-cp50227","",false));
    mCodePages.insert(50229,std::make_shared<CharsetInfo>(50229,"","",false));
    mCodePages.insert(50930,std::make_shared<CharsetInfo>(50930,"","",false));
    mCodePages.insert(50931,std::make_shared<CharsetInfo>(50931,"","",false));
    mCodePages.insert(50933,std::make_shared<CharsetInfo>(50933,"","",false));
    mCodePages.insert(50935,std::make_shared<CharsetInfo>(50935,"","",false));
    mCodePages.insert(50936,std::make_shared<CharsetInfo>(50936,"","",false));
    mCodePages.insert(50937,std::make_shared<CharsetInfo>(50937,"","",false));
    mCodePages.insert(50939,std::make_shared<CharsetInfo>(50939,"","",false));
    mCodePages.insert(51932,std::make_shared<CharsetInfo>(51932,"euc-jp",tr("Japanese"),true));
    mCodePages.insert(51936,std::make_shared<CharsetInfo>(51936,"euc-cn","",false));
    mCodePages.insert(51949,std::make_shared<CharsetInfo>(51949,"euc-kr",tr("Korean"),true));
    mCodePages.insert(51950,std::make_shared<CharsetInfo>(51950,"","",false));
    mCodePages.insert(52936,std::make_shared<CharsetInfo>(52936,"hz-gb-2312","",false));
    mCodePages.insert(54936,std::make_shared<CharsetInfo>(54936,"gb18030",tr("Chinese"),true));
    mCodePages.insert(57002,std::make_shared<CharsetInfo>(57002,"x-iscii-de","",false));
    mCodePages.insert(57003,std::make_shared<CharsetInfo>(57003,"x-iscii-be","",false));
    mCodePages.insert(57004,std::make_shared<CharsetInfo>(57004,"x-iscii-ta","",false));
    mCodePages.insert(57005,std::make_shared<CharsetInfo>(57005,"x-iscii-te","",false));
    mCodePages.insert(57006,std::make_shared<CharsetInfo>(57006,"x-iscii-as","",false));
    mCodePages.insert(57007,std::make_shared<CharsetInfo>(57007,"x-iscii-or","",false));
    mCodePages.insert(57008,std::make_shared<CharsetInfo>(57008,"x-iscii-ka","",false));
    mCodePages.insert(57009,std::make_shared<CharsetInfo>(57009,"x-iscii-ma","",false));
    mCodePages.insert(57010,std::make_shared<CharsetInfo>(57010,"x-iscii-gu","",false));
    mCodePages.insert(57011,std::make_shared<CharsetInfo>(57011,"x-iscii-pa","",false));
    mCodePages.insert(65000,std::make_shared<CharsetInfo>(65000,"utf-7","",false));
    mCodePages.insert(65001,std::make_shared<CharsetInfo>(65001,"utf-8","",false));

}

CharsetInfo::CharsetInfo(int codepage, const QByteArray &name, const QString &language, bool enabled)
{
    this->codepage = codepage;
    this->name = name;
    this->language = language;
    this->enabled = enabled;
}
