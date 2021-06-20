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

    QStyleOptionViewItem viewOptions() const override
    {
        QStyleOptionViewItem option = QListView::viewOptions();
        option.showDecorationSelected = true;
//        if (combo)
//            option.font = combo->font();
        return option;
    }

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
                menuOpt.tabWidth = 0;
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
