#ifndef UTILS_H
#define UTILS_H

class QByteArray;
class QString;

enum FileEncodingType {
    etAuto,
    etUTF8,
    etAscii,
    etAnsi,
    etUTF8Bom
};

FileEncodingType GetFileEncodingType(const QByteArray& content);

bool isTextAllAscii(const QString& text);

#endif // UTILS_H
