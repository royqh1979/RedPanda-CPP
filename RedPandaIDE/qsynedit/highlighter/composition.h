/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SYNHIGHLIGHTCOMPOSITION_H
#define SYNHIGHLIGHTCOMPOSITION_H
#include "base.h"
#include <memory>
#include <QObject>

namespace QSynedit {
class SynSchema;
using PSynSchema = std::shared_ptr<SynSchema>;

using OnCheckMarker = std::function<void(PSynSchema Sender,int &StartPos, int &MarkerLen,
  std::shared_ptr<QString>& MarkerText , int Line)>;

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
public:
    explicit SynHighlightComposition();

    // SynHighligterBase interface
public:
    SynHighlighterClass getClass() const override;
    QString getName() const override;
};

}
#endif // SYNHIGHLIGHTCOMPOSITION_H
