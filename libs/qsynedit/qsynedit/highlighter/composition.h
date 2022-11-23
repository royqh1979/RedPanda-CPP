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
class HighlighterSchema;
using PHighlighterSchema = std::shared_ptr<HighlighterSchema>;

//using OnCheckMarker = std::function<void(PHighlighterSchema Sender,int &StartPos, int &MarkerLen,
//  std::shared_ptr<QString>& MarkerText , int Line)>;

class HighlighterSchema : public QObject {
    Q_OBJECT
public:
    explicit HighlighterSchema(QObject* parent=nullptr);
    QString endExpr() const;
    void setEndExpr(const QString &endExpr);

    QString getStartExpr() const;
    void setStartExpr(const QString &value);

    PHighlighter getHighlighter() const;
    void setHighlighter(const PHighlighter &highlighter);

    PHighlighterAttribute getMarkerAttribute() const;

    QString getSchemeName() const;
    void setSchemeName(const QString &schemeName);

    int getCaseSensitive() const;
    void setCaseSensitive(int caseSensitive);

private:
    QString mEndExpr;
    QString StartExpr;
    PHighlighter mHighlighter;
    PHighlighterAttribute mMarkerAttribute;
    QString mSchemeName;
    int mCaseSensitive;
//    OnCheckMarker mOnCheckStartMarker;
//    OnCheckMarker mOnCheckEndMarker;
    QString ConvertExpression(const QString& Value);
private slots:
    void MarkerAttriChanged();
};

}
#endif // SYNHIGHLIGHTCOMPOSITION_H
