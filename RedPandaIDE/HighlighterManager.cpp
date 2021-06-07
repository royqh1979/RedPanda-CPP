#include "HighlighterManager.h"
#include <QFileInfo>
#include <QObject>
#include "qsynedit/highlighter/cpp.h"

HighlighterManager highlighterManager;

HighlighterManager::HighlighterManager()
{

}

PSynHighlighter HighlighterManager::getHighlighter(const QString &filename)
{
    if (filename.isEmpty() || filename.startsWith(QObject::tr("untitled"))) {
        return getCppHighlighter();
    } else {
        QFileInfo info(filename);
        QString suffix = info.suffix();
        if (suffix.isEmpty() || suffix == "c" || suffix == "cpp" || suffix == "cxx"
                || suffix == "cc" || suffix == "h" || suffix == "hpp"
                || suffix == "hxx" || suffix == "hh") {
            return getCppHighlighter();
        }
    }
    return PSynHighlighter();
}

PSynHighlighter HighlighterManager::getCppHighlighter()
{
    SynEditCppHighlighter* highlighter = new SynEditCppHighlighter();
    PSynHighlighter pHighlighter(highlighter);
    highlighter->asmAttribute()->setForeground(QColorConstants::Blue);
    highlighter->charAttribute()->setForeground(QColorConstants::Black);
    highlighter->commentAttribute()->setForeground(0x8C8C8C);
    highlighter->commentAttribute()->setStyles(SynFontStyle::fsItalic);
    highlighter->classAttribute()->setForeground(0x008080);
    highlighter->floatAttribute()->setForeground(QColorConstants::Svg::purple);
    highlighter->functionAttribute()->setForeground(0x00627A);
    highlighter->globalVarAttribute()->setForeground(0x660E7A);
    highlighter->hexAttribute()->setForeground(QColorConstants::Svg::purple);
    highlighter->identifierAttribute()->setForeground(0x080808);
    highlighter->invalidAttribute()->setForeground(QColorConstants::Svg::red);
    highlighter->localVarAttribute()->setForeground(QColorConstants::Black);
    highlighter->numberAttribute()->setForeground(0x1750EB);
    highlighter->octAttribute()->setForeground(QColorConstants::Svg::purple);
    highlighter->direcAttribute()->setForeground(0x1f542e);
    highlighter->keyAttribute()->setForeground(0x0033b3);
    highlighter->whitespaceAttribute()->setForeground(QColorConstants::Svg::silver);
    highlighter->stringAttribute()->setForeground(0x007d17);
    highlighter->stringEscapeSequenceAttribute()->setForeground(QColorConstants::Svg::red);
    highlighter->symbolAttribute()->setForeground(0xc10000);
    highlighter->variableAttribute()->setForeground(0x400080);
    return pHighlighter;
}
