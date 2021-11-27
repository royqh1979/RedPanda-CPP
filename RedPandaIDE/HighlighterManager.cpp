#include "HighlighterManager.h"
#include <QFileInfo>
#include <QObject>
#include "qsynedit/highlighter/cpp.h"
#include "qsynedit/highlighter/asm.h"
#include "qsynedit/Constants.h"
#include "colorscheme.h"

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
                || suffix == "hxx" || suffix == "hh" || suffix == "C"
                || suffix == "CPP" || suffix =="H" || suffix == "c++"
                || suffix == "h++") {
            return getCppHighlighter();
        }
    }
    return PSynHighlighter();
}

PSynHighlighter HighlighterManager::copyHighlighter(PSynHighlighter highlighter)
{
    if (!highlighter)
        return PSynHighlighter();
    if (highlighter->getName() == SYN_HIGHLIGHTER_CPP)
        return getCppHighlighter();
    //todo
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
    highlighter->preprocessorAttribute()->setForeground(0x1f542e);
    highlighter->keywordAttribute()->setForeground(0x0033b3);
    highlighter->whitespaceAttribute()->setForeground(QColorConstants::Svg::silver);
    highlighter->stringAttribute()->setForeground(0x007d17);
    highlighter->stringEscapeSequenceAttribute()->setForeground(QColorConstants::Svg::red);
    highlighter->symbolAttribute()->setForeground(0xc10000);
    highlighter->variableAttribute()->setForeground(0x400080);
    return pHighlighter;
}

PSynHighlighter HighlighterManager::getAsmHighlighter()
{
    SynEditASMHighlighter* highlighter = new SynEditASMHighlighter();
    PSynHighlighter pHighlighter(highlighter);
    highlighter->commentAttribute()->setForeground(0x8C8C8C);
    highlighter->commentAttribute()->setStyles(SynFontStyle::fsItalic);
    highlighter->identifierAttribute()->setForeground(0x080808);
    highlighter->keywordAttribute()->setForeground(0x0033b3);
    highlighter->numberAttribute()->setForeground(0x1750EB);
    highlighter->whitespaceAttribute()->setForeground(QColorConstants::Svg::silver);
    highlighter->stringAttribute()->setForeground(0x007d17);
    highlighter->symbolAttribute()->setForeground(0xc10000);
    return pHighlighter;
}

void HighlighterManager::applyColorScheme(PSynHighlighter highlighter, const QString &schemeName)
{
    if (!highlighter)
        return;
    if ( (highlighter->getName() == SYN_HIGHLIGHTER_CPP)
         || (highlighter->getName() == SYN_HIGHLIGHTER_ASM)
         ) {
        for (QString name: highlighter->attributes().keys()) {
            PColorSchemeItem item = pColorManager->getItem(schemeName,name);
            if (item) {
                PSynHighlighterAttribute attr = highlighter->attributes()[name];
                attr->setBackground(item->background());
                attr->setForeground(item->foreground());
                SynFontStyles styles = SynFontStyle::fsNone;
                styles.setFlag(SynFontStyle::fsBold, item->bold());
                styles.setFlag(SynFontStyle::fsItalic, item->italic());
                styles.setFlag(SynFontStyle::fsUnderline, item->underlined());
                styles.setFlag(SynFontStyle::fsStrikeOut, item->strikeout());
                attr->setStyles(styles);
            }
        }
    }
}
