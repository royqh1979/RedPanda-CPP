#ifndef SYNRTFEXPORTER_H
#define SYNRTFEXPORTER_H

#include "synexporter.h"

class SynRTFExporter : public SynExporter
{
public:
    explicit SynRTFExporter();
private:
    bool mAttributesChanged;
    QList<QColor> mListColors;
    QString ColorToRTF(const QColor& AColor) const;
    int GetColorIndex(const QColor& AColor);
    QString GetFontTable();

    // SynExporter interface
protected:
    void FormatAttributeDone(bool BackgroundChanged, bool ForegroundChanged, SynFontStyles FontStylesChanged) override;
    void FormatAttributeInit(bool BackgroundChanged, bool ForegroundChanged, SynFontStyles FontStylesChanged) override;
    void FormatAfterLastAttribute() override;
    void FormatBeforeFirstAttribute(bool BackgroundChanged, bool ForegroundChanged, SynFontStyles FontStylesChanged) override;
    void FormatNewLine() override;
    QString GetFooter() override;
    QString GetFormatName() override;
    QString GetHeader() override;
};

#endif // SYNRTFEXPORTER_H
