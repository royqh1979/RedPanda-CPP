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
#ifndef EDITORSTABWIDGET_H
#define EDITORSTABWIDGET_H

#include <QTabWidget>

class QDragEnterEvent;
class QDropEvent;

class EditorsTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    explicit EditorsTabWidget(QWidget* parent=nullptr);

signals:
    void middleButtonClicked(int index);
    // QWidget interface
protected:
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // EDITORSTABWIDGET_H
