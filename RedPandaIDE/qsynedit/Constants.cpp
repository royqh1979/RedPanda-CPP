#include "Constants.h"
const QSet<QChar> SynWordBreakChars{'.', ',', ';', ':',
      '"', '\'', '!', '?', '[', ']', '(', ')', '{', '}', '^', '-', '=', '+',
      '-', '*', '/', '\\', '|'};
const QChar SynTabChar('\t');
const QChar SynTabGlyph(0x2192);
const QChar SynSpaceGlyph('.');
const QChar SynLineBreakGlyph(0x2193);
const QChar SynSoftBreakGlyph(0x2193);
