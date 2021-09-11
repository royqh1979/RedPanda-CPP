#include "platform.h"
#include <QObject>
#include <memory>
#include <QMap>

#ifdef Q_OS_WIN
#include <windows.h>

struct CodePageInfo{
    int codepage;
    QByteArray name;
    QByteArray language;
    explicit CodePageInfo(int codepage,const QByteArray& name, const QByteArray& language){
        this->codepage = codepage;
        this->name = name;
        this->language = language;
    }
};

using PCodePageInfo = std::shared_ptr<CodePageInfo>;

static QMap<int,PCodePageInfo> CodePages;

static void initCodePages(){
    CodePages.insert(37,std::make_shared<CodePageInfo>(37,"IBM037",""));
    CodePages.insert(437,std::make_shared<CodePageInfo>(437,"IBM437",""));
    CodePages.insert(500,std::make_shared<CodePageInfo>(500,"IBM500",""));
    CodePages.insert(708,std::make_shared<CodePageInfo>(708,"ASMO-708",""));
    CodePages.insert(709,std::make_shared<CodePageInfo>(709,"",""));
    CodePages.insert(710,std::make_shared<CodePageInfo>(710,"",""));
    CodePages.insert(720,std::make_shared<CodePageInfo>(720,"DOS-720",""));
    CodePages.insert(737,std::make_shared<CodePageInfo>(737,"ibm737",""));
    CodePages.insert(775,std::make_shared<CodePageInfo>(775,"ibm775",""));
    CodePages.insert(850,std::make_shared<CodePageInfo>(850,"ibm850",""));
    CodePages.insert(852,std::make_shared<CodePageInfo>(852,"ibm852",""));
    CodePages.insert(855,std::make_shared<CodePageInfo>(855,"IBM855",""));
    CodePages.insert(857,std::make_shared<CodePageInfo>(857,"ibm857",""));
    CodePages.insert(858,std::make_shared<CodePageInfo>(858,"IBM00858",""));
    CodePages.insert(860,std::make_shared<CodePageInfo>(860,"IBM860",""));
    CodePages.insert(861,std::make_shared<CodePageInfo>(861,"ibm861",""));
    CodePages.insert(862,std::make_shared<CodePageInfo>(862,"DOS-862",""));
    CodePages.insert(863,std::make_shared<CodePageInfo>(863,"IBM863",""));
    CodePages.insert(864,std::make_shared<CodePageInfo>(864,"IBM864",""));
    CodePages.insert(865,std::make_shared<CodePageInfo>(865,"IBM865",""));
    CodePages.insert(866,std::make_shared<CodePageInfo>(866,"cp866",""));
    CodePages.insert(869,std::make_shared<CodePageInfo>(869,"ibm869",""));
    CodePages.insert(870,std::make_shared<CodePageInfo>(870,"IBM870",""));
    CodePages.insert(874,std::make_shared<CodePageInfo>(874,"windows-874",""));
    CodePages.insert(875,std::make_shared<CodePageInfo>(875,"cp875",""));
    CodePages.insert(932,std::make_shared<CodePageInfo>(932,"shift_jis",""));
    CodePages.insert(936,std::make_shared<CodePageInfo>(936,"gbk","Chinese"));
    CodePages.insert(949,std::make_shared<CodePageInfo>(949,"ks_c_5601-1987",""));
    CodePages.insert(950,std::make_shared<CodePageInfo>(950,"big5","Chinese_TC"));
    CodePages.insert(1026,std::make_shared<CodePageInfo>(1026,"IBM1026",""));
    CodePages.insert(1047,std::make_shared<CodePageInfo>(1047,"IBM01047",""));
    CodePages.insert(1140,std::make_shared<CodePageInfo>(1140,"IBM01140",""));
    CodePages.insert(1141,std::make_shared<CodePageInfo>(1141,"IBM01141",""));
    CodePages.insert(1142,std::make_shared<CodePageInfo>(1142,"IBM01142",""));
    CodePages.insert(1143,std::make_shared<CodePageInfo>(1143,"IBM01143",""));
    CodePages.insert(1144,std::make_shared<CodePageInfo>(1144,"IBM01144",""));
    CodePages.insert(1145,std::make_shared<CodePageInfo>(1145,"IBM01145",""));
    CodePages.insert(1146,std::make_shared<CodePageInfo>(1146,"IBM01146",""));
    CodePages.insert(1147,std::make_shared<CodePageInfo>(1147,"IBM01147",""));
    CodePages.insert(1148,std::make_shared<CodePageInfo>(1148,"IBM01148",""));
    CodePages.insert(1149,std::make_shared<CodePageInfo>(1149,"IBM01149",""));
    CodePages.insert(1200,std::make_shared<CodePageInfo>(1200,"utf-16",""));
    CodePages.insert(1201,std::make_shared<CodePageInfo>(1201,"unicodeFFFE",""));
    CodePages.insert(1250,std::make_shared<CodePageInfo>(1250,"windows-1250",""));
    CodePages.insert(1251,std::make_shared<CodePageInfo>(1251,"windows-1251",""));
    CodePages.insert(1252,std::make_shared<CodePageInfo>(1252,"windows-1252",""));
    CodePages.insert(1253,std::make_shared<CodePageInfo>(1253,"windows-1253",""));
    CodePages.insert(1254,std::make_shared<CodePageInfo>(1254,"windows-1254",""));
    CodePages.insert(1255,std::make_shared<CodePageInfo>(1255,"windows-1255",""));
    CodePages.insert(1256,std::make_shared<CodePageInfo>(1256,"windows-1256",""));
    CodePages.insert(1257,std::make_shared<CodePageInfo>(1257,"windows-1257",""));
    CodePages.insert(1258,std::make_shared<CodePageInfo>(1258,"windows-1258",""));
    CodePages.insert(1361,std::make_shared<CodePageInfo>(1361,"Johab",""));
    CodePages.insert(10000,std::make_shared<CodePageInfo>(10000,"macintosh",""));
    CodePages.insert(10001,std::make_shared<CodePageInfo>(10001,"x-mac-japanese",""));
    CodePages.insert(10002,std::make_shared<CodePageInfo>(10002,"x-mac-chinesetrad",""));
    CodePages.insert(10003,std::make_shared<CodePageInfo>(10003,"x-mac-korean",""));
    CodePages.insert(10004,std::make_shared<CodePageInfo>(10004,"x-mac-arabic",""));
    CodePages.insert(10005,std::make_shared<CodePageInfo>(10005,"x-mac-hebrew",""));
    CodePages.insert(10006,std::make_shared<CodePageInfo>(10006,"x-mac-greek",""));
    CodePages.insert(10007,std::make_shared<CodePageInfo>(10007,"x-mac-cyrillic",""));
    CodePages.insert(10008,std::make_shared<CodePageInfo>(10008,"x-mac-chinesesimp",""));
    CodePages.insert(10010,std::make_shared<CodePageInfo>(10010,"x-mac-romanian",""));
    CodePages.insert(10017,std::make_shared<CodePageInfo>(10017,"x-mac-ukrainian",""));
    CodePages.insert(10021,std::make_shared<CodePageInfo>(10021,"x-mac-thai",""));
    CodePages.insert(10029,std::make_shared<CodePageInfo>(10029,"x-mac-ce",""));
    CodePages.insert(10079,std::make_shared<CodePageInfo>(10079,"x-mac-icelandic",""));
    CodePages.insert(10081,std::make_shared<CodePageInfo>(10081,"x-mac-turkish",""));
    CodePages.insert(10082,std::make_shared<CodePageInfo>(10082,"x-mac-croatian",""));
    CodePages.insert(12000,std::make_shared<CodePageInfo>(12000,"utf-32",""));
    CodePages.insert(12001,std::make_shared<CodePageInfo>(12001,"utf-32BE",""));
    CodePages.insert(20000,std::make_shared<CodePageInfo>(20000,"x-Chinese_CNS",""));
    CodePages.insert(20001,std::make_shared<CodePageInfo>(20001,"x-cp20001",""));
    CodePages.insert(20002,std::make_shared<CodePageInfo>(20002,"x_Chinese-Eten",""));
    CodePages.insert(20003,std::make_shared<CodePageInfo>(20003,"x-cp20003",""));
    CodePages.insert(20004,std::make_shared<CodePageInfo>(20004,"x-cp20004",""));
    CodePages.insert(20005,std::make_shared<CodePageInfo>(20005,"x-cp20005",""));
    CodePages.insert(20105,std::make_shared<CodePageInfo>(20105,"x-IA5",""));
    CodePages.insert(20106,std::make_shared<CodePageInfo>(20106,"x-IA5-German",""));
    CodePages.insert(20107,std::make_shared<CodePageInfo>(20107,"x-IA5-Swedish",""));
    CodePages.insert(20108,std::make_shared<CodePageInfo>(20108,"x-IA5-Norwegian",""));
    CodePages.insert(20127,std::make_shared<CodePageInfo>(20127,"us-ascii",""));
    CodePages.insert(20261,std::make_shared<CodePageInfo>(20261,"x-cp20261",""));
    CodePages.insert(20269,std::make_shared<CodePageInfo>(20269,"x-cp20269",""));
    CodePages.insert(20273,std::make_shared<CodePageInfo>(20273,"IBM273",""));
    CodePages.insert(20277,std::make_shared<CodePageInfo>(20277,"IBM277",""));
    CodePages.insert(20278,std::make_shared<CodePageInfo>(20278,"IBM278",""));
    CodePages.insert(20280,std::make_shared<CodePageInfo>(20280,"IBM280",""));
    CodePages.insert(20284,std::make_shared<CodePageInfo>(20284,"IBM284",""));
    CodePages.insert(20285,std::make_shared<CodePageInfo>(20285,"IBM285",""));
    CodePages.insert(20290,std::make_shared<CodePageInfo>(20290,"IBM290",""));
    CodePages.insert(20297,std::make_shared<CodePageInfo>(20297,"IBM297",""));
    CodePages.insert(20420,std::make_shared<CodePageInfo>(20420,"IBM420",""));
    CodePages.insert(20423,std::make_shared<CodePageInfo>(20423,"IBM423",""));
    CodePages.insert(20424,std::make_shared<CodePageInfo>(20424,"IBM424",""));
    CodePages.insert(20833,std::make_shared<CodePageInfo>(20833,"x-EBCDIC-KoreanExtended",""));
    CodePages.insert(20838,std::make_shared<CodePageInfo>(20838,"IBM-Thai",""));
    CodePages.insert(20866,std::make_shared<CodePageInfo>(20866,"koi8-r",""));
    CodePages.insert(20871,std::make_shared<CodePageInfo>(20871,"IBM871",""));
    CodePages.insert(20880,std::make_shared<CodePageInfo>(20880,"IBM880",""));
    CodePages.insert(20905,std::make_shared<CodePageInfo>(20905,"IBM905",""));
    CodePages.insert(20924,std::make_shared<CodePageInfo>(20924,"IBM00924",""));
    CodePages.insert(20932,std::make_shared<CodePageInfo>(20932,"EUC-JP",""));
    CodePages.insert(20936,std::make_shared<CodePageInfo>(20936,"x-cp20936",""));
    CodePages.insert(20949,std::make_shared<CodePageInfo>(20949,"x-cp20949",""));
    CodePages.insert(21025,std::make_shared<CodePageInfo>(21025,"cp1025",""));
    CodePages.insert(21027,std::make_shared<CodePageInfo>(21027,"",""));
    CodePages.insert(21866,std::make_shared<CodePageInfo>(21866,"koi8-u",""));
    CodePages.insert(28591,std::make_shared<CodePageInfo>(28591,"iso-8859-1",""));
    CodePages.insert(28592,std::make_shared<CodePageInfo>(28592,"iso-8859-2",""));
    CodePages.insert(28593,std::make_shared<CodePageInfo>(28593,"iso-8859-3",""));
    CodePages.insert(28594,std::make_shared<CodePageInfo>(28594,"iso-8859-4",""));
    CodePages.insert(28595,std::make_shared<CodePageInfo>(28595,"iso-8859-5",""));
    CodePages.insert(28596,std::make_shared<CodePageInfo>(28596,"iso-8859-6",""));
    CodePages.insert(28597,std::make_shared<CodePageInfo>(28597,"iso-8859-7",""));
    CodePages.insert(28598,std::make_shared<CodePageInfo>(28598,"iso-8859-8",""));
    CodePages.insert(28599,std::make_shared<CodePageInfo>(28599,"iso-8859-9",""));
    CodePages.insert(28603,std::make_shared<CodePageInfo>(28603,"iso-8859-13",""));
    CodePages.insert(28605,std::make_shared<CodePageInfo>(28605,"iso-8859-15",""));
    CodePages.insert(29001,std::make_shared<CodePageInfo>(29001,"x-Europa",""));
    CodePages.insert(38598,std::make_shared<CodePageInfo>(38598,"iso-8859-8-i",""));
    CodePages.insert(50220,std::make_shared<CodePageInfo>(50220,"iso-2022-jp",""));
    CodePages.insert(50221,std::make_shared<CodePageInfo>(50221,"csISO2022JP",""));
    CodePages.insert(50222,std::make_shared<CodePageInfo>(50222,"iso-2022-jp",""));
    CodePages.insert(50225,std::make_shared<CodePageInfo>(50225,"iso-2022-kr",""));
    CodePages.insert(50227,std::make_shared<CodePageInfo>(50227,"x-cp50227",""));
    CodePages.insert(50229,std::make_shared<CodePageInfo>(50229,"",""));
    CodePages.insert(50930,std::make_shared<CodePageInfo>(50930,"",""));
    CodePages.insert(50931,std::make_shared<CodePageInfo>(50931,"",""));
    CodePages.insert(50933,std::make_shared<CodePageInfo>(50933,"",""));
    CodePages.insert(50935,std::make_shared<CodePageInfo>(50935,"",""));
    CodePages.insert(50936,std::make_shared<CodePageInfo>(50936,"",""));
    CodePages.insert(50937,std::make_shared<CodePageInfo>(50937,"",""));
    CodePages.insert(50939,std::make_shared<CodePageInfo>(50939,"",""));
    CodePages.insert(51932,std::make_shared<CodePageInfo>(51932,"euc-jp",""));
    CodePages.insert(51936,std::make_shared<CodePageInfo>(51936,"EUC-CN",""));
    CodePages.insert(51949,std::make_shared<CodePageInfo>(51949,"euc-kr",""));
    CodePages.insert(51950,std::make_shared<CodePageInfo>(51950,"",""));
    CodePages.insert(52936,std::make_shared<CodePageInfo>(52936,"hz-gb-2312",""));
    CodePages.insert(54936,std::make_shared<CodePageInfo>(54936,"GB18030",""));
    CodePages.insert(57002,std::make_shared<CodePageInfo>(57002,"x-iscii-de",""));
    CodePages.insert(57003,std::make_shared<CodePageInfo>(57003,"x-iscii-be",""));
    CodePages.insert(57004,std::make_shared<CodePageInfo>(57004,"x-iscii-ta",""));
    CodePages.insert(57005,std::make_shared<CodePageInfo>(57005,"x-iscii-te",""));
    CodePages.insert(57006,std::make_shared<CodePageInfo>(57006,"x-iscii-as",""));
    CodePages.insert(57007,std::make_shared<CodePageInfo>(57007,"x-iscii-or",""));
    CodePages.insert(57008,std::make_shared<CodePageInfo>(57008,"x-iscii-ka",""));
    CodePages.insert(57009,std::make_shared<CodePageInfo>(57009,"x-iscii-ma",""));
    CodePages.insert(57010,std::make_shared<CodePageInfo>(57010,"x-iscii-gu",""));
    CodePages.insert(57011,std::make_shared<CodePageInfo>(57011,"x-iscii-pa",""));
    CodePages.insert(65000,std::make_shared<CodePageInfo>(65000,"utf-7",""));
    CodePages.insert(65001,std::make_shared<CodePageInfo>(65001,"utf-8",""));
}

QByteArray getDefaultSystemEncoding()
{
    if (CodePages.isEmpty())
        initCodePages();
    DWORD acp = GetACP();
    PCodePageInfo info = CodePages.value(acp,PCodePageInfo());
    if (info) {
        return info->name;
    }
    return "unknown";
}

#endif

