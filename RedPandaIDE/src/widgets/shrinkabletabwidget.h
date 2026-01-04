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
#ifndef SHRINKABLETABWIDGET_H
#define SHRINKABLETABWIDGET_H

#include <QTabWidget>

class ShrinkableTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    ShrinkableTabWidget(QWidget* parent=nullptr);

    void setShrinkedFlag(bool shrinked);
    void setShrinked(bool shrinked);
    bool isShrinked() const;
    void toggleShrined();
    void setBeforeShrinkSize(const QSize& size);
    QSize beforeShrinkSize() const;
    QSize currentSize() const;
    int beforeShrinkWidthOrHeight() const;
    Qt::Orientation shrinkOrientation() const;

    // QWidget interface
public:
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
private:
    bool mShrinked;
    static QHash<const ShrinkableTabWidget*,QSize> BeforeShrinkSizes;

    // QWidget interface
protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // SHRINKABLETABWIDGET_H
