#ifndef CODEFOLDING_H
#define CODEFOLDING_H
#include <QColor>
#include <vector>
#include <memory>

struct QSynFoldRegion;
typedef std::shared_ptr<QSynFoldRegion> PSynFoldRegion;

class QSynFoldRegions {
private:
    std::vector<PSynFoldRegion> fRegions;
public:
    int count();
    int add(bool addEnding, const QChar& openSymbol, const QChar& closeSymbol, const QString& highlight);
    PSynFoldRegion get(int index);
};
typedef std::shared_ptr<QSynFoldRegions> PSynFoldRegions;


struct QSynFoldRegion {
    bool addEnding;
    QSynFoldRegions subFoldRegions;
    QChar openSymbol;
    QChar closeSymbol;
    QString highlight;
};

struct QSynCodeFolding {
      bool indentGuides;
      bool showCollapsedLine;
      QColor collapsedLineColor;
      QColor folderBarLinesColor;
      QColor indentGuidesColor;
      QSynFoldRegions foldRegions;
      QSynCodeFolding();
};

class QSynEditFoldRange;
typedef std::shared_ptr<QSynEditFoldRange> PSynEditFoldRange;
class QSynEditFoldRanges;
typedef std::shared_ptr<QSynEditFoldRanges> PSynEditFoldRanges;

class QSynEditFoldRanges{
public:
    std::vector<PSynEditFoldRange> ranges;
    PSynEditFoldRange foldRange(int index);
    int count();
    QSynEditFoldRanges();
    PSynEditFoldRange addByParts(PSynEditFoldRange aParent, PSynEditFoldRanges aAllFold,
                               int aFromLine, PSynFoldRegion aFoldRegion, int aToLine);

    int remove(int index);
    void addObject(PSynEditFoldRange foldRange);
};

// A single fold
class QSynEditFoldRange {
public:
    int fromLine; // Beginning line
    int toLine; // End line
    int linesCollapsed; // Number of collapsed lines
    QSynEditFoldRanges subFoldRanges; // Sub fold ranges
    bool collapsed; // Is collapsed?
    PSynEditFoldRanges allFoldRanges;// TAllFoldRanges pointer
    PSynFoldRegion foldRegion; // FoldRegion
    int hintMarkLeft;
    PSynEditFoldRange parent;
    bool parentCollapsed();
    void move(int count);
    explicit QSynEditFoldRange(PSynEditFoldRange aParent, PSynEditFoldRanges aAllFold,
                      int aFromLine, PSynFoldRegion aFoldRegion, int aToLine);
};

#endif // CODEFOLDING_H
