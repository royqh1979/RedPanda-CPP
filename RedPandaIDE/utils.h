#ifndef UTILS_H
#define UTILS_H

class QByteArray;
class QString;

#define ENCODING_AUTO_DETECT "AUTO"
#define ENCODING_UTF8   "UTF-8"
#define ENCODING_UTF8_BOM "UTF-8 BOM"
#define ENCODING_SYSTEM_DEFAULT   "SYSTEM"
#define ENCODING_ASCII  "ASCII"
const QByteArray GuessTextEncoding(const QByteArray& text);

bool isTextAllAscii(const QString& text);

#endif // UTILS_H
