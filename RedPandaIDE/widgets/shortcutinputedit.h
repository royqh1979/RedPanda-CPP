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
#ifndef SHORTCUTINPUTEDIT_H
#define SHORTCUTINPUTEDIT_H

#include <QLineEdit>

class ShortcutInputEdit : public QLineEdit
{
    Q_OBJECT
public:
    ShortcutInputEdit(QWidget* parent);
signals:
    void inputFinished(QWidget* editor);
    // QWidget interface
protected:
    void keyPressEvent(QKeyEvent *event) override;

    // QObject interface
public:
    bool event(QEvent *event) override;

};

#endif // SHORTCUTINPUTEDIT_H
