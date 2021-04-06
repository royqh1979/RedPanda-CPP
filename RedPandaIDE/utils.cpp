#include "utils.h"
#include <QByteArray>
#include <QString>
#include <QTextCodec>

FileEncodingType GetFileEncodingType(const QByteArray& content){
    bool allAscii;
    int ii;
    int size;
    const QByteArray& s=content;
    size = s.length();
    if ( (size >= 3) && ((unsigned char)s[0]==0xEF) && ((unsigned char)s[1]==0xBB) && ((unsigned char)s[2]==0xBF)) {
        return etUTF8Bom;
    }
    allAscii = true;
    ii = 0;
    while (ii < size) {
        unsigned char ch = s[ii];
        if (ch < 0x80 ) {
            ii++; // is an ascii char
        } else if (ch < 0xC0) { // value between 0x80 and 0xC0 is an invalid UTF-8 char
            return etAnsi;
        } else if (ch < 0xE0) { // should be an 2-byte UTF-8 char
            if (ii>=size-1) {
                return etAnsi;
            }
            unsigned char ch2=s[ii+1];
            if ((ch2 & 0xC0) !=0x80)  {
                return etAnsi;
            }
            allAscii = false;
            ii+=2;
        } else if (ch < 0xF0) { // should be an 3-byte UTF-8 char
            if (ii>=size-2) {
                return etAnsi;
            }
            unsigned char ch2=s[ii+1];
            unsigned char ch3=s[ii+2];
            if (((ch2 & 0xC0)!=0x80) ||  ((ch3 & 0xC0)!=0x80)) {
                return etAnsi;
            }
            allAscii = false;
            ii+=3;
        } else { // invalid UTF-8 char
            return etAnsi;
        }
    }
    if (allAscii)
        return etAscii;
    return etUTF8;
}

QString UTF8toQString(const QByteArray& ba){
    QTextCodec* tc = QTextCodec::codecForName("UTF-8");
    if (tc == NULL)
        return QString();
    else
        return tc->toUnicode(ba);
}
