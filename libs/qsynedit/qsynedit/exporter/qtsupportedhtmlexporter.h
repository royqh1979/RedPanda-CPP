#ifndef QTSUPPORTEDHTMLEXPORTER_H
#define QTSUPPORTEDHTMLEXPORTER_H

#include "exporter.h"

namespace QSynedit {
class QtSupportedHtmlExporter : public Exporter
{
public:
    explicit QtSupportedHtmlExporter(int tabSize,const QByteArray charset);
    QtSupportedHtmlExporter(const QtSupportedHtmlExporter&)=delete;
    QtSupportedHtmlExporter& operator=(const QtSupportedHtmlExporter&)=delete;

    bool createHTMLFragment() const;
    void setCreateHTMLFragment(bool createHTMLFragment);

protected:
    bool mCreateHTMLFragment;
private:
    PTokenAttribute mLastAttri;
    QString attriToCSS(PTokenAttribute attri);
    QString colorToHTML(const QColor &color);

    // SynExporter interface
protected:
    void formatAttributeDone(bool backgroundChanged, bool foregroundChanged, FontStyles fontStyles);
    void formatAttributeInit(bool backgroundChanged, bool foregroundChanged, FontStyles fontStyles);
    void formatAfterLastAttribute();
    void formatBeforeFirstAttribute(bool backgroundChanged, bool foregroundChanged, FontStyles fontStyles);
    void formatNewLine();
    QString getFooter();
    QString getFormatName();
    QString getHeader();
    void setTokenAttribute(PTokenAttribute Attri);
};
}
#endif // QTSUPPORTEDHTMLEXPORTER_H
