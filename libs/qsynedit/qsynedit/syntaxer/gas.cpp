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
#include "gas.h"
#include "../constants.h"
#include <qt_utils/utils.h>
#include <QDebug>

namespace  QSynedit {

const QSet<QString> GASSyntaxer::Directives {
#if defined(ARCH_X86_64) || defined(ARCH_X86)
    "section","global","extern","segment",
    "db","dw","dd","dq","dt","do","dy","dz",
    "resb","resw","resd","resq","rest","reso","resy","resz",
    "equ","times","byte","word","dword","qword","tword",
    "xmmword","ymmword","zmmword","fword","tbyte","oword","ptr",
#endif
    ".lcomm",".largecomm","value"
    ".intel_style",".att_syntax",
    ".intel_mnemonic",".att_mnemonic",
    ".tfloat",".hfloat",".bfloat16",
#ifdef Q_OS_WIN
    ".seh_proc",".seh_endprologue",".seh_handler",
    ".seh_eh",".seh_32",".seh_no32",".seh_endproc",
    ".seh_setframe",".seh_stackalloc",".seh_pushreg",
    ".seh_savereg",".seh_savemm",".seh_savexmm",
    ".seh_pushframe",".seh_scope",
#else // Unix
    ".cfi_sections",".cfi_startproc",".cfi_endproc",
    ".cfi_personality",".cfi_personality_id",".cfi_fde_data",
    ".cfi_lsda",".cfi_inline_lsda",".cfi_def_cfa",
    ".cfi_def_cfa_register",".cfi_def_cfa_offset",".cfi_adjust_cfa_offset",
    ".cfi_offset",".cfi_val_offset",".cfi_rel_offset",
    ".cfi_register",".cfi_restore",".cfi_undefined",
    ".cfi_same_value",".cfi_remember_state",".cfi_restore_state",
    ".cfi_return_column",".cfi_signal_frame",".cfi_window_save",
    ".cfi_escape",".cfi_val_encoded_addr",
#endif
    ".abort",".align",".altmacro",".ascii",
    ".asciz",".attach",".balign",".bss",
    ".bundle",".byte",".comm",".data",
    ".dc",".dcb",".ds",".def", ".desc",
    ".dim","double",".eject",".else",
    ".elseif",".end",".endef","endfunc",
    ".endif",".equ",".equiv",".eqy",
    ".err",".error",".exitm",".extern",
    ".fail",".file",".fill", ".float",
    ".func",".globl",".global",".gnu",".hidden",
    ".hword",".ident",".if", ".incbin",
    ".inclue", ".int", ".internal", ".intel_syntax",".irp",
    ".irpc",".lcomm",".lflags",".line",".linkonce",
    ".list", ".ln", ".loc",".local",".macro",
    ".mri",".noaltmacro",".nolist",".nop",".nops",
    ".octa",".offset",".org",".p2align",".popsection",
    ".previous",".print",".protected",".psize",
    ".purgem",".pushsection",".quad",".reloc",
    ".rept", ".sbttl", ".scl", ".section",
    ".set", ".short", ".single", ".size",
    ".skip", ".sleb128", ".space_size", ".stabd",
    ".stabn", ".stabs", ".string", ".string8", ".string16",
    ".struct", ".subsection", ".symver", ".tag", ".text",
    ".title", ".tls", ".type", ".uleb128", ".val",".version",
    ".vtable", ".warning",".weak",".weakref",".word",
    ".zero",
};

GASSyntaxer::GASSyntaxer(): ASMSyntaxer(),
    mDirectiveSyntaxLine{-1},
    mSyntaxMode{SyntaxMode::ATT},
    mPrefixRegisterNames{true},
    mThisLineHasSyntaxDirective{false}
{
}

bool GASSyntaxer::isCommentStartChar(QChar ch)
{
    return (ch == "#");
}

void GASSyntaxer::procNull()
{
    if ( (mLineNumber == mThisLineHasSyntaxDirective)
            && !mThisLineHasSyntaxDirective) {
        mSyntaxMode = SyntaxMode::ATT;
        setPrefixRegisterNames(true);
        mDirectiveSyntaxLine = -1;
    }
    ASMSyntaxer::procNull();
}

GASSyntaxer::SyntaxMode GASSyntaxer::syntaxMode() const
{
    return mSyntaxMode;
}

void GASSyntaxer::setSyntaxMode(SyntaxMode newSyntaxMode)
{
    mSyntaxMode = newSyntaxMode;
}

bool GASSyntaxer::prefixRegisterNames() const
{
    return mPrefixRegisterNames;
}

bool GASSyntaxer::isDirective(const QString &ident)
{
    return Directives.contains(ident);
}

void GASSyntaxer::handleDirective(int line, const QString &directive)
{
    //directive is already in lower case
    if (directive == ".intel_syntax") {
        mThisLineHasSyntaxDirective = true;
        mSyntaxMode = SyntaxMode::Intel;
        setPrefixRegisterNames(false);
        mDirectiveSyntaxLine = line;
    } else if (directive == ".att_syntax") {
        mThisLineHasSyntaxDirective = true;
        mSyntaxMode = SyntaxMode::ATT;
        setPrefixRegisterNames(true);
        mDirectiveSyntaxLine = line;
    }
}

void GASSyntaxer::handleIdent(int line, const QString &ident)
{
    if (mDirectiveSyntaxLine == line) {
        if (ident == "prefix") {
            setPrefixRegisterNames(true);
        } else if (ident == "noprefix") {
            setPrefixRegisterNames(false);
        }
    }
}

void GASSyntaxer::setPrefixRegisterNames(bool prefix)
{
    if (prefix != mPrefixRegisterNames) {
        mPrefixRegisterNames = prefix;
        resetKeywordsCache();
    }
}

void GASSyntaxer::setLine(int lineNumber, const QString &newLine, size_t lineSeq)
{
    Q_UNUSED(lineSeq);
    mThisLineHasSyntaxDirective = false;
    ASMSyntaxer::setLine(lineNumber, newLine, lineSeq);
}

QString GASSyntaxer::languageName()
{
    return "GNU Assembly";
}

ProgrammingLanguage GASSyntaxer::language()
{
    return ProgrammingLanguage::GNU_Assembly;
}

QSet<QString> GASSyntaxer::keywords()
{
    if (mPrefixRegisterNames) {
        if (mPrefixedKeywordCache.isEmpty()) {
            mPrefixedKeywordCache = InstructionNames;
            mPrefixedKeywordCache += Directives;
            mPrefixedKeywordCache += PrefixedRegisters;
        }
        return mPrefixedKeywordCache;
    }
    if (mNonprefixedKeywordCache.isEmpty()) {
        mNonprefixedKeywordCache = InstructionNames;
        mNonprefixedKeywordCache += Directives;
        mNonprefixedKeywordCache += Registers;
    }
    return mNonprefixedKeywordCache;
}

}
