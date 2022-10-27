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
#include "MiscClasses.h"
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
    ecNone             =    0, // Nothing. Useful for user event to handle command
    ecViewCommandFirst =    0,
    ecViewCommandLast  =  500,
    ecEditCommandFirst =  501,
    ecEditCommandLast  = 1000,

    ecLeft            = 1,    // Move cursor left one char
    ecRight           = 2,    // Move cursor right one char
    ecUp              = 3,    // Move cursor up one line
    ecDown            = 4,    // Move cursor down one line
    ecWordLeft        = 5,    // Move cursor left one word
    ecWordRight       = 6,    // Move cursor right one word
    ecLineStart       = 7,    // Move cursor to beginning of line
    ecLineEnd         = 8,    // Move cursor to end of line
    ecPageUp          = 9,    // Move cursor up one page
    ecPageDown        = 10,   // Move cursor down one page
    ecPageLeft        = 11,   // Move cursor right one page
    ecPageRight       = 12,   // Move cursor left one page
    ecPageTop         = 13,   // Move cursor to top of page
    ecPageBottom      = 14,   // Move cursor to bottom of page
    ecEditorStart       = 15,   // Move cursor to absolute beginning
    ecEditorEnd    = 16,   // Move cursor to absolute end
    ecGotoXY          = 17,   // Move cursor to specific coordinates, Data = PPoint
    ecBlockStart      = 18,   // Move cursor to begin of block
    ecBlockEnd        = 19,   // Move cursor to end of block

//******************************************************************************
// Maybe the command processor should just take a boolean that signifies if
// selection is affected or not?
//******************************************************************************

    ecSelection       = 100,  // Add this to ecXXX command to get equivalent
                          // command, but with selection enabled. This is not
                          // a command itself.
// Same as commands above, except they affect selection, too
    ecSelLeft         = ecLeft + ecSelection,
    ecSelRight        = ecRight + ecSelection,
    ecSelUp           = ecUp + ecSelection,
    ecSelDown         = ecDown + ecSelection,
    ecSelWordLeft     = ecWordLeft + ecSelection,
    ecSelWordRight    = ecWordRight + ecSelection,
    ecSelLineStart    = ecLineStart + ecSelection,
    ecSelLineEnd      = ecLineEnd + ecSelection,
    ecSelPageUp       = ecPageUp + ecSelection,
    ecSelPageDown     = ecPageDown + ecSelection,
    ecSelPageLeft     = ecPageLeft + ecSelection,
    ecSelPageRight    = ecPageRight + ecSelection,
    ecSelPageTop      = ecPageTop + ecSelection,
    ecSelPageBottom   = ecPageBottom + ecSelection,
    ecSelEditorStart    = ecEditorStart + ecSelection,
    ecSelEditorEnd = ecEditorEnd + ecSelection,
    ecSelGotoXY       = ecGotoXY + ecSelection,  // Data = PPoint
    ecSelBlockStart      = ecBlockStart + ecSelection,   // Move cursor to begin of scope
    ecSelBlockEnd        = ecBlockEnd + ecSelection,   // Move cursor to end of scope


    ecCopy            = 201,  // Copy selection to clipboard
    ecSelWord         = 202,
    ecSelectAll       = 203,  // Select entire contents of editor, cursor to end
    ecExpandSelection = 204,  // expand selection
    ecShrinkSelection = 205,  // shrink selection

    ecScrollUp        = 211,  // Scroll up one line leaving cursor position unchanged.
    ecScrollDown      = 212,  // Scroll down one line leaving cursor position unchanged.
    ecScrollLeft      = 213,  // Scroll left one char leaving cursor position unchanged.
    ecScrollRight     = 214,  // Scroll right one char leaving cursor position unchanged.

    ecInsertMode      = 221,  // Set insert mode
    ecOverwriteMode   = 222,  // Set overwrite mode
    ecToggleMode      = 223,  // Toggle ins/ovr mode

    ecNormalSelect    = 231,  // Normal selection mode
    ecColumnSelect    = 232,  // Column selection mode
    ecLineSelect      = 233,  // Line selection mode

    ecMatchBracket    = 250,  // Go to matching bracket

    ecContextHelp     = 490,  // Help on Word, Data = Word

    ecDeleteLastChar  = 501,  // Delete last char (i.e. backspace key)
    ecDeleteChar      = 502,  // Delete char at cursor (i.e. delete key)
    ecDeleteWordEnd   = 503,  // Delete from cursor to end of word
    ecDeleteWordStart  = 504,  // Delete from cursor to start of word
    ecDeleteBOL       = 505,  // Delete from cursor to beginning of line
    ecDeleteEOL       = 506,  // Delete from cursor to end of line
    ecDeleteLine      = 507,  // Delete current line
    ecClearAll        = 508,  // Delete everything
    ecLineBreak       = 509,  // Break line at current position, move caret to new line
    ecInsertLine      = 510,  // Break line at current position, leave caret
    ecChar            = 511,  // Insert a character at current position
    ecDuplicateLine   = 512,  // Duplicate current line
    ecMoveSelUp       = 513,  // Move selection up
    ecMoveSelDown     = 514,  // Move selection down
    ecImeStr          = 550,  // Insert character(s) from IME
    ecDeleteWord     = 551,  // Delete current Word

    ecUndo            = 601,  // Perform undo if available
    ecRedo            = 602,  // Perform redo if available
    ecCut             = 603,  // Cut selection to clipboard
    ecPaste           = 604,  // Paste clipboard to current position

    ecBlockIndent     = 610,  // Indent selection
    ecBlockUnindent   = 611,  // Unindent selection
    ecTab             = 612,  // Tab key
    ecShiftTab        = 613,  // Shift+Tab key
    ecComment         = 614,
    ecUncomment       = 615,
    ecToggleComment   = 616,
    ecToggleBlockComment   = 617,

    ecUpperCase       = 620, // apply to the current or previous word
    ecLowerCase       = 621,
    ecToggleCase      = 622,
    ecTitleCase       = 623,
    ecUpperCaseBlock  = 625, // apply to current selection, or current char if no selection
    ecLowerCaseBlock  = 626,
    ecToggleCaseBlock = 627,

    ecString          = 630,  //Insert a whole string
    ecZoomOut         = 631,  //Increase Font Size
    ecZoomIn          = 632,  //Decrease Font Size

    ecLineBreakAtBegin  = 651, //add a line break at the begin of the line
    ecLineBreakAtEnd    = 652,

    //### Code Folding ###
    ecCollapse          = 700,
    ecUncollapse        = 701,
    ecCollapseLevel     = 702,
    ecUncollapseLevel   = 703,
    ecCollapseAll       = 704,
    ecUncollapseAll     = 705,
    //### End Code Folding ###

    ecUserFirst = 1001, // Start of user-defined commands



};

class KeyError: public BaseError {
public:
    explicit KeyError(const QString& reason);
};

class EditKeyStroke
{
public:
    explicit EditKeyStroke();
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
