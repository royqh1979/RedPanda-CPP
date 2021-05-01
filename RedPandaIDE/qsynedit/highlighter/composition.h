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
    explicit SynScheme(QObject* parent=nullptr);
    QString endExpr() const;
    void setEndExpr(const QString &endExpr);

    QString getStartExpr() const;
    void setStartExpr(const QString &value);

    PSynHighlighter getHighlighter() const;
    void setHighlighter(const PSynHighlighter &highlighter);

    PSynHighlighterAttribute getMarkerAttribute() const;

    QString getSchemeName() const;
    void setSchemeName(const QString &schemeName);

    int getCaseSensitive() const;
    void setCaseSensitive(int caseSensitive);

private:
    QString mEndExpr;
    QString StartExpr;
    PSynHighlighter mHighlighter;
    PSynHighlighterAttribute mMarkerAttribute;
    QString mSchemeName;
    int mCaseSensitive;
    OnCheckMarker mOnCheckStartMarker;
    OnCheckMarker mOnCheckEndMarker;
    QString ConvertExpression(const QString& Value);
private slots:
    void MarkerAttriChanged();
};



class SynHighlightComposition : public SynHighlighter
{
    Q_OBJECT
public:
    explicit SynHighlightComposition(QObject *parent = nullptr);

    // SynHighligterBase interface
public:
    SynHighlighterClass getClass() const override;
    QString getName() const override;
};

#endif // SYNHIGHLIGHTCOMPOSITION_H
