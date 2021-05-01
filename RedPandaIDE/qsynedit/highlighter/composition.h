#ifndef SYNHIGHLIGHTCOMPOSITION_H
#define SYNHIGHLIGHTCOMPOSITION_H
#include "base.h"
#include <memory>
#include <QObject>

class SynSchema;
using PSynSchema = std::shared_ptr<SynSchema>;

using OnCheckMarker = void (*)(PSynSchema Sender,int &StartPos, int &MarkerLen,
  std::shared_ptr<QString>& MarkerText , int Line);

class SynScheme : public QObject {
    Q_OBJECT
public:
    explicit SynScheme(QObject& parent=nullptr);
private:
    QString mEndExpr;
    QString StartExpr;
    PSynHighligterBase mHighlighter;
    PSynHighlighterAttribute mMarkerAttribute;
    QString mSchemeName;
    int mCaseSensitive;
    OnCheckMarker mOnCheckStartMarker;
    OnCheckMarker mOnCheckEndMarker;
    QString ConvertExpression(const QString& Value);
private slots:
    void MarkerAttriChanged();
};

public
  constructor Create(Collection: TCollection); override;
  destructor Destroy; override;
published
  property CaseSensitive: Boolean read fCaseSensitive write SetCaseSensitive
    default True;
  property StartExpr: string read fStartExpr write SetStartExpr;
  property EndExpr: string read fEndExpr write SetEndExpr;
  property Highlighter: TSynCustomHighlighter read fHighlighter
    write SetHighlighter;
  property MarkerAttri: TSynHighlighterAttributes read fMarkerAttri
    write SetMarkerAttri;
  property SchemeName: TComponentName read fSchemeName write fSchemeName;
  property OnCheckStartMarker: TOnCheckMarker read fOnCheckStartMarker write fOnCheckStartMarker;
  property OnCheckEndMarker: TOnCheckMarker read fOnCheckEndMarker write fOnCheckEndMarker;
end;


class SynHighlightComposition : public SynHighligterBase
{
    Q_OBJECT
public:
    explicit SynHighlightComposition(QObject *parent = nullptr);
};

#endif // SYNHIGHLIGHTCOMPOSITION_H
