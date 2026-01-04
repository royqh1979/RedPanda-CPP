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
#ifndef QPATCHEDCOMBOBOX_H
#define QPATCHEDCOMBOBOX_H

#include <QComboBox>
#include <QListView>
#include <QPaintEvent>
#include <QPainter>

class QPatchedComboBoxListView : public QListView
{
    Q_OBJECT
public:
    QPatchedComboBoxListView(QComboBox *cmb = nullptr) : combo(cmb) {}

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        resizeContents(viewport()->width(), contentsSize().height());
        QListView::resizeEvent(event);
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void initViewItemOption(QStyleOptionViewItem *option) const override
    {
        QListView::initViewItemOption(option);
        option->showDecorationSelected = true;
    }
#else
    QStyleOptionViewItem viewOptions() const override
    {
        QStyleOptionViewItem option = QListView::viewOptions();
        option.showDecorationSelected = true;
//        if (combo)
//            option.font = combo->font();
        return option;
    }
#endif

    void paintEvent(QPaintEvent *e) override
    {
        if (combo) {
            QStyleOptionComboBox opt;
            opt.initFrom(combo);
            opt.editable = combo->isEditable();
            if (combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo)) {
                //we paint the empty menu area to avoid having blank space that can happen when scrolling
                QStyleOptionMenuItem menuOpt;
                menuOpt.initFrom(this);
                menuOpt.palette = palette();
                menuOpt.state = QStyle::State_None;
                menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
                menuOpt.menuRect = e->rect();
                menuOpt.maxIconWidth = 0;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                menuOpt.reservedShortcutWidth = 0;
#else
                menuOpt.tabWidth = 0;
#endif
                QPainter p(viewport());
                combo->style()->drawControl(QStyle::CE_MenuEmptyArea, &menuOpt, &p, this);
            }
        }
        QListView::paintEvent(e);
    }

private:
    QComboBox *combo;
};

class QPatchedComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit QPatchedComboBox(QWidget *parent = nullptr);
};

#endif // QPATCHEDCOMBOBOX_H
