/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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

#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QStringList>
#include "terminalprocess.h"

class TerminalWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TerminalWidget(QWidget *parent = nullptr);

    void setWorkingDirectory(const QString &path);
    QString workingDirectory() const;
    void focusInput();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onInputReturn();
    void onProcessOutput(const QString &html);
    void onCommandFinished(int exitCode);
    void onProcessWdChanged(const QString &path);

private:
    void appendHtml(const QString &html);
    void navigateHistory(bool up);
    void printPrompt();

    QPlainTextEdit *mOutput;
    QLineEdit *mInput;
    TerminalProcess *mProcess;

    // Per-instance command history
    QStringList mCommandHistory;
    int mHistoryIndex = -1;
};

#endif // TERMINALWIDGET_H
