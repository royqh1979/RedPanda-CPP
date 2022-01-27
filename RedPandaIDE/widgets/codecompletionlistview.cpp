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
#include "codecompletionlistview.h"
#include "../mainwindow.h"
#include "../editor.h"
#include "../editorlist.h"

#include <QPainter>
#include <QTextDocument>
#include <qabstracttextdocumentlayout.h>

CodeCompletionListView::CodeCompletionListView(QWidget *parent) : QListView(parent)
{
    setUniformItemSizes(true);
    setItemDelegate(&mDelegate);
}

void CodeCompletionListView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Up
            || event->key() == Qt::Key_Down
            || event->key() == Qt::Key_PageDown
            || event->key() == Qt::Key_PageUp
            || event->key() == Qt::Key_Home
            || event->key() == Qt::Key_End
            ) {
        QListView::keyPressEvent(event);
        return;
    }
    if (!mKeypressedCallback || !mKeypressedCallback(event)) {
        QListView::keyPressEvent(event);
    }
}

void CodeCompletionListView::focusInEvent(QFocusEvent *)
{
    Editor *editor = pMainWindow->editorList()->getEditor();
    if (editor) {
        editor->showCaret();
    }
}

const KeyPressedCallback &CodeCompletionListView::keypressedCallback() const
{
    return mKeypressedCallback;
}

void CodeCompletionListView::setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback)
{
    mKeypressedCallback = newKeypressedCallback;
}


void CodeCompletionListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant data = index.data();
    if (data.canConvert<QString>()) {
        painter->save();
        QString richText = qvariant_cast<QString>(data);

        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());

        QColor color = index.data(Qt::ForegroundRole).value<QColor>();
        if (!color.isValid()) {
            color = option.palette.color(QPalette::Text);
        }
        painter->setPen(color);
        QTextDocument doc;
        doc.setHtml(richText);
        doc.setDefaultFont(painter->font());
        QTransform transform;
        transform.translate(option.rect.left(),option.rect.top());
        painter->setTransform(transform);
        QRect clipRect = option.rect;
        clipRect.moveTopLeft(QPoint(0,0));
        painter->setClipRect(clipRect);
        QAbstractTextDocumentLayout::PaintContext ctx;

        ctx.palette.setColor(QPalette::Text, color);
        ctx.clip = clipRect;
        doc.documentLayout()->draw(painter,ctx);
        painter->restore();
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize CodeCompletionListItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant data = index.data();
    if (data.canConvert<QString>()) {
        QString richText = qvariant_cast<QString>(data);

        QTextDocument doc;
        doc.setHtml(richText);
        return QSize(doc.size().width(),doc.size().height());
    } else {
        return QStyledItemDelegate::sizeHint(option, index);
    }

}
