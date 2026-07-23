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

#include "terminalwidget.h"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QScrollBar>
#include <QApplication>
#include <QFont>
#include <QDir>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
    , mOutput(new QPlainTextEdit(this))
    , mInput(new QLineEdit(this))
    , mProcess(new TerminalProcess(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Output area
    mOutput->setReadOnly(true);
    mOutput->setUndoRedoEnabled(false);
    mOutput->setFont(QFont("Consolas", 10));
    mOutput->setStyleSheet(
        "QPlainTextEdit { background-color: #1e1e1e; color: #abb2bf; "
        "border: none; padding: 4px; }");
    mOutput->setContextMenuPolicy(Qt::DefaultContextMenu);
    layout->addWidget(mOutput);

    // Input line
    mInput->setFont(QFont("Consolas", 10));
    mInput->setStyleSheet(
        "QLineEdit { background-color: #252526; color: #abb2bf; "
        "border-top: 1px solid #3e3e42; padding: 4px; }");
    mInput->setPlaceholderText(tr("Type command here... (cd, make, git, etc.)"));
    layout->addWidget(mInput);

    connect(mInput, &QLineEdit::returnPressed,
            this, &TerminalWidget::onInputReturn);
    connect(mProcess, &TerminalProcess::outputReady,
            this, &TerminalWidget::onProcessOutput);
    connect(mProcess, &TerminalProcess::commandFinished,
            this, &TerminalWidget::onCommandFinished);
    connect(mProcess, &TerminalProcess::workingDirectoryChanged,
            this, &TerminalWidget::onProcessWdChanged);

    mInput->installEventFilter(this);

    printPrompt();
    mInput->setFocus();
}

void TerminalWidget::setWorkingDirectory(const QString &path)
{
    mProcess->setWorkingDirectory(path);
}

QString TerminalWidget::workingDirectory() const
{
    return mProcess->workingDirectory();
}

void TerminalWidget::focusInput()
{
    mInput->setFocus();
}

void TerminalWidget::onInputReturn()
{
    QString line = mInput->text().trimmed();
    mInput->clear();
    if (line.isEmpty())
        return;

    // Add to history
    mCommandHistory.append(line);
    mHistoryIndex = mCommandHistory.size();

    // Echo the command
    appendHtml(QString("<span style='color:#61afef'>$ %1</span><br>")
               .arg(line.toHtmlEscaped()));

    mProcess->execute(line);
}

void TerminalWidget::onProcessOutput(const QString &html)
{
    if (html.contains("terminal-clear")) {
        mOutput->clear();
        return;
    }
    appendHtml(html);
}

void TerminalWidget::onCommandFinished(int exitCode)
{
    if (exitCode != 0) {
        appendHtml(QString("<span style='color:#e06c75'>[exit code: %1]</span><br>")
                   .arg(exitCode));
    }
    printPrompt();
}

void TerminalWidget::onProcessWdChanged(const QString &)
{
    // Prompt is printed by onCommandFinished or the next onInputReturn
}

void TerminalWidget::appendHtml(const QString &html)
{
    mOutput->moveCursor(QTextCursor::End);
    mOutput->textCursor().insertHtml(html);
    QScrollBar *sb = mOutput->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void TerminalWidget::printPrompt()
{
    QString wd = mProcess->workingDirectory();
    QStringList parts = wd.split(QDir::separator(), Qt::SkipEmptyParts);
    QString shortPath;
    if (parts.size() > 2) {
        shortPath = QString("...%1%2%1%3")
            .arg(QDir::separator(), parts[parts.size()-2], parts[parts.size()-1]);
    } else {
        shortPath = wd;
    }

    appendHtml(QString("<span style='color:#98c379'>%1</span>"
                       "<span style='color:#61afef'> $ </span>")
               .arg(shortPath.toHtmlEscaped()));
}

bool TerminalWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == mInput && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Up:
            navigateHistory(true);
            return true;
        case Qt::Key_Down:
            navigateHistory(false);
            return true;
        case Qt::Key_C:
            if (keyEvent->modifiers() == Qt::ControlModifier) {
                if (mInput->text().isEmpty()) {
                    mProcess->stop();
                    appendHtml("<span style='color:#e5c07b'>^C</span><br>");
                    printPrompt();
                }
                return true;
            }
            break;
        case Qt::Key_L:
            if (keyEvent->modifiers() == Qt::ControlModifier) {
                mOutput->clear();
                printPrompt();
                return true;
            }
            break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void TerminalWidget::navigateHistory(bool up)
{
    if (mCommandHistory.isEmpty())
        return;

    if (mHistoryIndex == mCommandHistory.size()) {
        // Currently at "new input" position, save current text
        mHistoryIndex = up ? mCommandHistory.size() - 1 : mCommandHistory.size();
    } else if (up) {
        if (mHistoryIndex > 0)
            mHistoryIndex--;
    } else {
        if (mHistoryIndex < mCommandHistory.size() - 1)
            mHistoryIndex++;
        else {
            // Past the end — clear input
            mHistoryIndex = mCommandHistory.size();
            mInput->clear();
            return;
        }
    }

    if (mHistoryIndex >= 0 && mHistoryIndex < mCommandHistory.size())
        mInput->setText(mCommandHistory[mHistoryIndex]);
}
