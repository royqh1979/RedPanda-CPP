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
#include "editorstabwidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include "../editor.h"
#include "../editorlist.h"
#include "../mainwindow.h"

EditorsTabWidget::EditorsTabWidget(QWidget* parent):QTabWidget(parent)
{
    setAcceptDrops(true);
}

void EditorsTabWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        QStringList files;
        foreach(const QUrl& url, event->mimeData()->urls()){
            if (!url.isLocalFile())
                continue;
            QString file = url.toLocalFile();
            files.append(file);
        }
        pMainWindow->openFiles(files);
    }
}

void EditorsTabWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()){
        event->acceptProposedAction();
    }
}

void EditorsTabWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::MiddleButton) {
        int idx = this->tabBar()->tabAt(event->pos());
        if (idx>=0)
            emit middleButtonClicked(idx);
    }
    QTabWidget::mousePressEvent(event);
}
