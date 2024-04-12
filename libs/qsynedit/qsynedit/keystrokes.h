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
#ifndef SYNEDITKEYSTROKE_H
#define SYNEDITKEYSTROKE_H

#include <QKeySequence>
#include <QList>
#include <memory>
#include "gutter.h"
#include "qt_utils/utils.h"

namespace QSynedit {
//****************************************************************************
// NOTE!  If you add an editor command, you must also update the
//    EditorCommandStrs constant array in implementation section below, or the
//    command will not show up in the IDE.
//****************************************************************************

// "Editor Commands".  Key strokes are translated from a table into these
// I used constants instead of a set so that additional commands could be
// added in descendants (you can't extend a set)

// There are two ranges of editor commands: the ecViewXXX commands are always
// valid, while the ecEditXXX commands are ignored when the editor is in
// read-only mode

enum class EditCommand {
    None             =    0, // Nothing. Useful for user event to handle command

    Left            = 1,    // Move cursor left one char
    Right           = 2,    // Move cursor right one char
    Up              = 3,    // Move cursor up one line
    Down            = 4,    // Move cursor down one line
    WordLeft        = 5,    // Move cursor left one word
    WordRight       = 6,    // Move cursor right one word
    LineStart       = 7,    // Move cursor to beginning of line
    LineEnd         = 8,    // Move cursor to end of line
    PageUp          = 9,    // Move cursor up one page
    PageDown        = 10,   // Move cursor down one page
    PageLeft        = 11,   // Move cursor right one page
    PageRight       = 12,   // Move cursor left one page
    PageTop         = 13,   // Move cursor to top of page
    PageBottom      = 14,   // Move cursor to bottom of page
    FileStart       = 15,   // Move cursor to absolute beginning
    FileEnd    = 16,   // Move cursor to absolute end
    GotoXY          = 17,   // Move cursor to specific coordinates, Data = PPoint
    BlockStart      = 18,   // Move cursor to begin of block
    BlockEnd        = 19,   // Move cursor to end of block

//******************************************************************************
// Maybe the command processor should just take a boolean that signifies if
// selection is affected or not?
//******************************************************************************

    Selection       = 100,  // Add this to ecXXX command to get equivalent
                          // command, but with selection enabled. This is not
                          // a command itself.
// Same as commands above, except they affect selection, too
    SelLeft         = Left + Selection,
    SelRight        = Right + Selection,
    SelUp           = Up + Selection,
    SelDown         = Down + Selection,
    SelWordLeft     = WordLeft + Selection,
    SelWordRight    = WordRight + Selection,
    SelLineStart    = LineStart + Selection,
    SelLineEnd      = LineEnd + Selection,
    SelPageUp       = PageUp + Selection,
    SelPageDown     = PageDown + Selection,
    SelPageLeft     = PageLeft + Selection,
    SelPageRight    = PageRight + Selection,
    SelPageTop      = PageTop + Selection,
    SelPageBottom   = PageBottom + Selection,
    SelFileStart    = FileStart + Selection,
    SelFileEnd = FileEnd + Selection,
    SelGotoXY       = GotoXY + Selection,  // Data = PPoint
    SelBlockStart      = BlockStart + Selection,   // Move cursor to begin of scope
    SelBlockEnd        = BlockEnd + Selection,   // Move cursor to end of scope


    Copy            = 201,  // Copy selection to clipboard
    SelWord         = 202,
    SelectAll       = 203,  // Select entire contents of editor, cursor to end
    ExpandSelection = 204,  // expand selection
    ShrinkSelection = 205,  // shrink selection

    ScrollUp        = 211,  // Scroll up one line leaving cursor position unchanged.
    ScrollDown      = 212,  // Scroll down one line leaving cursor position unchanged.
    ScrollLeft      = 213,  // Scroll left one char leaving cursor position unchanged.
    ScrollRight     = 214,  // Scroll right one char leaving cursor position unchanged.

    InsertMode      = 221,  // Set insert mode
    OverwriteMode   = 222,  // Set overwrite mode
    ToggleMode      = 223,  // Toggle ins/ovr mode

    MatchBracket    = 250,  // Go to matching bracket

    ContextHelp     = 490,  // Help on Word, Data = Word

    DeleteLastChar  = 501,  // Delete last char (i.e. backspace key)
    DeleteChar      = 502,  // Delete char at cursor (i.e. delete key)
    DeleteWordEnd   = 503,  // Delete from cursor to end of word
    DeleteWordStart  = 504,  // Delete from cursor to start of word
    DeleteBOL       = 505,  // Delete from cursor to beginning of line
    DeleteEOL       = 506,  // Delete from cursor to end of line
    DeleteLine      = 507,  // Delete current line
    ClearAll        = 508,  // Delete everything
    LineBreak       = 509,  // Break line at current position, move caret to new line
    InsertLine      = 510,  // Break line at current position, leave caret
    Char            = 511,  // Insert a character at current position
    DuplicateLine   = 512,  // Duplicate current line
    MoveSelUp       = 513,  // Move selection up
    MoveSelDown     = 514,  // Move selection down
    ImeStr          = 550,  // Insert character(s) from IME
    DeleteWord     = 551,  // Delete current Word

    Undo            = 601,  // Perform undo if available
    Redo            = 602,  // Perform redo if available
    Cut             = 603,  // Cut selection to clipboard
    Paste           = 604,  // Paste clipboard to current position

    BlockIndent     = 610,  // Indent selection
    BlockUnindent   = 611,  // Unindent selection
    Tab             = 612,  // Tab key
    ShiftTab        = 613,  // Shift+Tab key
    Comment         = 614,
    Uncomment       = 615,
    ToggleComment   = 616,
    ToggleBlockComment   = 617,

    UpperCase       = 620, // apply to the current or previous word
    LowerCase       = 621,
    ToggleCase      = 622,
    TitleCase       = 623,
    UpperCaseBlock  = 625, // apply to current selection, or current char if no selection
    LowerCaseBlock  = 626,
    ToggleCaseBlock = 627,

    String          = 630,  //Insert a whole string
    ZoomOut         = 631,  //Increase Font Size
    ZoomIn          = 632,  //Decrease Font Size

    LineBreakAtBegin  = 651, //add a line break at the begin of the line
    LineBreakAtEnd    = 652,

    TrimTrailingSpaces = 653,

    //### Code Folding ###
    Collapse          = 700,
    Uncollapse        = 701,
    CollapseLevel     = 702,
    UncollapseLevel   = 703,
    CollapseAll       = 704,
    UncollapseAll     = 705,
    //### End Code Folding ###

    UserFirst = 1001, // Start of user-defined commands
};

class EditKeyStroke
{
public:
    explicit EditKeyStroke();
    EditKeyStroke(const EditKeyStroke&)=delete;
    EditKeyStroke& operator=(const EditKeyStroke&)=delete;

    QKeySequence keySequence() const;
    void setKeySequence(QKeySequence& keySequence);
    int key() const;
    void setKey(int key);

    Qt::KeyboardModifiers keyModifiers() const;
    void setKeyModifiers(const Qt::KeyboardModifiers &keyModifiers);

    int key2() const;
    void setKey2(int key2);

    Qt::KeyboardModifiers keyModifiers2() const;
    void setKeyModifiers2(const Qt::KeyboardModifiers &keyModifiers2);

    EditCommand command() const;
    void setCommand(const EditCommand &command);

private:
    int mKey; // Virtual keycode, i.e. VK_xxx
    Qt::KeyboardModifiers mKeyModifiers;
    int mKey2;
    Qt::KeyboardModifiers mKeyModifiers2;
    EditCommand mCommand;

};

using PEditKeyStroke = std::shared_ptr<EditKeyStroke>;
using EditKeyStrokeList = QList<PEditKeyStroke>;

class EditKeyStrokes {
public:
    explicit EditKeyStrokes();
    EditKeyStrokes(const EditKeyStrokes&)=delete;
    EditKeyStrokes& operator=(const EditKeyStrokes&)=delete;

    PEditKeyStroke add(EditCommand command, int key, Qt::KeyboardModifiers modifiers);
    PEditKeyStroke findCommand(EditCommand command);
    PEditKeyStroke findKeycode(int key, Qt::KeyboardModifiers modifiers);
    PEditKeyStroke findKeycode2(int key, Qt::KeyboardModifiers modifiers,
                                   int key2, Qt::KeyboardModifiers modifiers2);
    PEditKeyStroke findKeySequence(const QKeySequence& keySeq);
    void clear();
    void resetDefaults();
    void setExtraKeyStrokes();
private:
    EditKeyStrokeList mList;
};

}

#endif // SYNEDITKEYSTROKE_H
