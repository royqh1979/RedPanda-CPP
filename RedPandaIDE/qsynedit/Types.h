#ifndef TYPES_H
#define TYPES_H
enum class SynSelectionMode {smNormal, smLine, smColumn};

struct BufferCoord {
    int Char;
    int Line;
};

struct DisplayCoord {
    int Column;
    int Row;
};
#endif // TYPES_H
