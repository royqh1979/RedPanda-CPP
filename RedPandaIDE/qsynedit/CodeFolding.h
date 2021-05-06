#ifndef CODEFOLDING_H
#define CODEFOLDING_H
#include <QColor>
#include <vector>
#include <memory>
#include <QVector>

struct SynEditFoldRegion;
typedef std::shared_ptr<SynEditFoldRegion> PSynEditFoldRegion;

class SynEditFoldRegions {
private:
    std::vector<PSynEditFoldRegion> fRegions;
public:
    int count();
    int add(bool addEnding, const QChar& openSymbol, const QChar& closeSymbol, const QString& highlight);
    PSynEditFoldRegion get(int index);
};
typedef std::shared_ptr<SynEditFoldRegions> PSynFoldRegions;


struct SynEditFoldRegion {
    bool addEnding;
    SynEditFoldRegions subFoldRegions;
    QChar openSymbol;
    QChar closeSymbol;
    QString highlight;
};

struct SynEditCodeFolding {
      bool indentGuides;
      bool showCollapsedLine;
      QColor collapsedLineColor;
      QColor folderBarLinesColor;
      QColor indentGuidesColor;
      SynEditFoldRegions foldRegions;
      SynEditCodeFolding();
};

class SynEditFoldRange;
typedef std::shared_ptr<SynEditFoldRange> PSynEditFoldRange;
class SynEditFoldRanges;
typedef std::shared_ptr<SynEditFoldRanges> PSynEditFoldRanges;

class SynEditFoldRanges{
public:
    QVector<PSynEditFoldRange> ranges;
    PSynEditFoldRange foldRange(int index);
    int count();
    SynEditFoldRanges();
    PSynEditFoldRange addByParts(PSynEditFoldRange aParent, PSynEditFoldRanges aAllFold,
                               int aFromLine, PSynEditFoldRegion aFoldRegion, int aToLine);

    int remove(int index);
    void addObject(PSynEditFoldRange foldRange);
};

// A single fold
class SynEditFoldRange {
public:
    int fromLine; // Beginning line
    int toLine; // End line
    int linesCollapsed; // Number of collapsed lines
    SynEditFoldRanges subFoldRanges; // Sub fold ranges
    bool collapsed; // Is collapsed?
    PSynEditFoldRanges allFoldRanges;// TAllFoldRanges pointer
    PSynEditFoldRegion foldRegion; // FoldRegion
    int hintMarkLeft;
    PSynEditFoldRange parent;
    bool parentCollapsed();
    void move(int count);
    explicit SynEditFoldRange(PSynEditFoldRange aParent, PSynEditFoldRanges aAllFold,
                      int aFromLine, PSynEditFoldRegion aFoldRegion, int aToLine);
};

#endif // CODEFOLDING_H
