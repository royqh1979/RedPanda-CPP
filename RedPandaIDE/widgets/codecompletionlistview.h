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
#ifndef CODECOMPLETIONLISTVIEW_H
#define CODECOMPLETIONLISTVIEW_H

#include <QListView>
#include <QKeyEvent>
#include "../parser/parserutils.h"
using KeyPressedCallback = std::function<bool (QKeyEvent *)>;
using InputMethodCallback = std::function<bool (QInputMethodEvent*)>;

class CodeCompletionListView: public QListView {
    Q_OBJECT
public:
    explicit CodeCompletionListView(QWidget *parent = nullptr);

    // QWidget interface
    const KeyPressedCallback &keypressedCallback() const;
    void setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback);

    const InputMethodCallback &inputMethodCallback() const;
    void setInputMethodCallback(const InputMethodCallback &newInputMethodCallback);

protected:
    void keyPressEvent(QKeyEvent *event) override;
private:
    KeyPressedCallback mKeypressedCallback;

    // QWidget interface
protected:
    void focusInEvent(QFocusEvent *event) override;

    // QWidget interface
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};


#endif // CODECOMPLETIONLISTVIEW_H
