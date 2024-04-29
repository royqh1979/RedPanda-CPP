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
#include "asm.h"
#include "../constants.h"
#include <qt_utils/utils.h>
#include <QDebug>

namespace  QSynedit {

QSet<QString> ASMSyntaxer::InstructionNames;
QMap<QString,QString> ASMSyntaxer::Instructions;

const QSet<QString> ASMSyntaxer::Registers {
#if defined(ARCH_X86_64) || defined(ARCH_X86)
    "ah","al","ax","eax",
    "bh","bl","bx","ebx",
    "ch","cl","cx","ecx",
    "dh","dl","dx","edx",
    "spl","sp","esp",
    "bpl","bp","ebp",
    "sil","si","esi",
    "dil","di","edi",
    "r8b","r8w","r8d",
    "r9b","r9w","r9d",
    "r10b","r10w","r10d",
    "r11b","r11w","r11d",
    "r12b","r12w","r12d",
    "r13b","r13w","r13d",
    "r14b","r14w","r14d",
    "r15b","r15w","r15d",
    "rax","rbx","rcx","rdx","rsp","rbp","rsi","rdi",
    "r8","r9","r10","r11","r12","r13","r14","r15",
    "ip","eip","rip",
    "flags","eflags","rflags",
    "cs","ds","ss","es","fs","gs",
    "st0","st1","st2","st3","st4","st5","st6","st7",
    "xmm0","xmm1","xmm2","xmm3",
    "xmm4","xmm5","xmm6","xmm7",
    "xmm8","xmm9","xmm10","xmm11",
    "xmm12","xmm13","xmm14","xmm15",
#endif
};

const QSet<QString> ASMSyntaxer::ATTRegisters {
#if defined(ARCH_X86_64) || defined(ARCH_X86)
    "%ah","%al","%ax","%eax",
    "%bh","%bl","%bx","%ebx",
    "%ch","%cl","%cx","%ecx",
    "%dh","%dl","%dx","%edx",
    "%spl","%sp","%esp",
    "%bpl","%bp","%ebp",
    "%sil","%si","%esi",
    "%dil","%di","%edi",
    "%r8b","%r8w","%r8d",
    "%r9b","%r9w","%r9d",
    "%r10b","%r10w","%r10d",
    "%r11b","%r11w","%r11d",
    "%r12b","%r12w","%r12d",
    "%r13b","%r13w","%r13d",
    "%r14b","%r14w","%r14d",
    "%r15b","%r15w","%r15d",
    "%rax","%rbx","%rcx","%rdx","%rsp","%rbp","%rsi","%rdi",
    "%r8","%r9","%r10","%r11","%r12","%r13","%r14","%r15",
    "%ip","%eip","%rip",
    "%flags","%eflags","%rflags",
    "%cs","%ds","%ss","%es","%fs","%gs",
    "%st0","%st1","%st2","%st3","%st4","%st5","%st6","%st7",
    "%xmm0","%xmm1","%xmm2","%xmm3",
    "%xmm4","%xmm5","%xmm6","%xmm7",
    "%xmm8","%xmm9","%xmm10","%xmm11",
    "%xmm12","%xmm13","%xmm14","%xmm15",
#endif
};

const QSet<QString> ASMSyntaxer::Directives {
#if defined(ARCH_X86_64) || defined(ARCH_X86)
    "section","global","extern","segment",
    "db","dw","dd","dq","dt","do","dy","dz",
    "resb","resw","resd","resq","rest","reso","resy","resz",
    "equ","times","byte","word","dword","qword","tword",
    "xmmword","ymmword","zmmword","fword","tbyte","oword","ptr",
#endif
};

const QSet<QString> ASMSyntaxer::ATTDirectives {
#if defined(ARCH_X86_64) || defined(ARCH_X86)
    ".lcomm",".largecomm","value"
    ".intel_style",".att_syntax",
    ".intel_mnemonic",".att_mnemonic",
    ".tfloat",".hfloat",".bfloat16",
#endif
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
    ".zero",".2byte",".4byte",".8byte"
};

ASMSyntaxer::ASMSyntaxer(bool isATT, bool isCppMixed):
    mATT{isATT},
    mCppMixed{isCppMixed}
{
    initData();
    mNumberAttribute = std::make_shared<TokenAttribute>(SYNS_AttrNumber, TokenType::Number);
    addAttribute(mNumberAttribute);
    mDirectiveAttribute = std::make_shared<TokenAttribute>(SYNS_AttrVariable, TokenType::Keyword);
    addAttribute(mDirectiveAttribute);
    mLabelAttribute = std::make_shared<TokenAttribute>(SYNS_AttrFunction, TokenType::Keyword);
    addAttribute(mLabelAttribute);
    mRegisterAttribute = std::make_shared<TokenAttribute>(SYNS_AttrClass, TokenType::Keyword);
    addAttribute(mRegisterAttribute);
}

const PTokenAttribute &ASMSyntaxer::numberAttribute() const
{
    return mNumberAttribute;
}

const PTokenAttribute &ASMSyntaxer::registerAttribute() const
{
    return mRegisterAttribute;
}

bool ASMSyntaxer::isATT() const
{
    return mATT;
}

void ASMSyntaxer::setATT(bool newATT)
{
    if (mATT!=newATT) {
        mATT = newATT;
        mKeywordsCache.clear();
    }
}

void ASMSyntaxer::CommentProc()
{
    mTokenID = TokenId::Comment;
    do {
        mRun++;
    } while (! (mLine[mRun]==0 || mLine[mRun] == '\r' || mLine[mRun]=='\n'));
}

void ASMSyntaxer::CRProc()
{
    mTokenID = TokenId::Space;
    mRun++;
    if (mLine[mRun] == '\n')
        mRun++;
}

void ASMSyntaxer::GreaterProc()
{
    mRun++;
    mTokenID = TokenId::Symbol;
    if (mLine[mRun] == '=')
        mRun++;
}

void ASMSyntaxer::IdentProc(IdentPrefix prefix)
{
    int start = mRun;
    while (isIdentChar(mLine[mRun])) {
        mRun++;
    }
    QString s = mLineString.mid(start,mRun-start).toLower();
    switch(prefix) {
    case IdentPrefix::Percent:
        mTokenID = TokenId::Register;
        break;
    case IdentPrefix::Period:
        if (mLine[mRun]==':')
            mTokenID = TokenId::Label;
        else
            mTokenID = TokenId::Directive;
        break;
    default:
        if (Instructions.contains(s))
            mTokenID = TokenId::Instruction;
        else if (Registers.contains(s))
            mTokenID = TokenId::Register;
        else if (Directives.contains(s))
            mTokenID = TokenId::Directive;
        else if (mLine[mRun]==':')
            mTokenID = TokenId::Label;
        else
            mTokenID = TokenId::Identifier;
    }
}

void ASMSyntaxer::LFProc()
{
    mTokenID = TokenId::Space;
    mRun++;
}

void ASMSyntaxer::LowerProc()
{
    mRun++;
    mTokenID = TokenId::Symbol;
    if (mLine[mRun]=='=' || mLine[mRun]== '>')
        mRun++;
}

void ASMSyntaxer::NullProc()
{
    mTokenID = TokenId::Null;
}

void ASMSyntaxer::NumberProc()
{
    mRun++;
    mTokenID = TokenId::Number;
    while (true) {
        QChar ch = mLine[mRun];
        if (!((ch>='0' && ch<='9') || (ch=='.') || (ch >= 'a' && ch<='f')
              || (ch=='h') || (ch >= 'A' && ch<='F') || (ch == 'H')
              || (ch == 'x')))
            break;
        mRun++;
    }
}

void ASMSyntaxer::SingleQuoteStringProc()
{
    mTokenID = TokenId::String;
    if ((mRun+2 < mLineString.size()) && (mLine[mRun + 1] == '\'') && (mLine[mRun + 2] == '\''))
        mRun += 2;
    while (true) {
        if (mLine[mRun] == 0 || mLine[mRun] == '\r' || mLine[mRun] == '\n' || mLine[mRun] == '\'')
            break;
        mRun++;
    }
    if (mLine[mRun]!=0)
        mRun++;
}

void ASMSyntaxer::SlashProc()
{
    mRun++;
    if (mLine[mRun] == '/') {
      mTokenID = TokenId::Comment;
      while (true) {
        mRun++;
        if (mLine[mRun] == 0 || mLine[mRun] == '\r' || mLine[mRun] == '\n')
            break;
      }
    } else
        mTokenID = TokenId::Symbol;
}

void ASMSyntaxer::SpaceProc()
{
    mTokenID = TokenId::Space;
    while (true) {
        mRun++;
        if (mLine[mRun] == 0 || mLine[mRun] == '\r' || mLine[mRun] == '\n')
            break;
        if (!isLexicalSpace(mLine[mRun]))
            break;
    }
    if (mRun>=mStringLen)
        mHasTrailingSpaces = true;
}

void ASMSyntaxer::StringProc()
{
    mTokenID = TokenId::String;
    if ((mRun+2 < mLineString.size()) && (mLine[mRun + 1] == '\"') && (mLine[mRun + 2] == '\"'))
        mRun += 2;
    else
        mRun+=1;
    while (true) {
        if (mLine[mRun] == 0 || mLine[mRun] == '\r' || mLine[mRun] == '\n')
            break;
        if (mLine[mRun] == '\"')
            break;
        mRun += 1;
    }
    if (mLine[mRun]!=0)
        mRun++;
}

void ASMSyntaxer::SymbolProc()
{
    mRun++;
    mTokenID = TokenId::Symbol;
}

void ASMSyntaxer::UnknownProc()
{
    mRun++;
    mTokenID = TokenId::Unknown;
}

bool ASMSyntaxer::isIdentStartChar(const QChar &ch) const
{
    if (ch == '_') {
        return true;
    }
    if ((ch>='a') && (ch <= 'z')) {
        return true;
    }
    if ((ch>='A') && (ch <= 'Z')) {
        return true;
    }
    return false;
}

void ASMSyntaxer::initData()
{
    if (Instructions.isEmpty()) {
        // https://docs.oracle.com/cd/E19120-01/open.solaris/817-5477/ennbz/index.html
        //Data Transfer Instruction
#if defined(ARCH_X86_64) || defined(ARCH_X86)
        Instructions.insert("bswap",QObject::tr("byte swap."));
        Instructions.insert("bswapl",QObject::tr("byte swap."));
        Instructions.insert("bswapq",QObject::tr("byte swap."));
        Instructions.insert("cbtw",QObject::tr("convert %1 to %2.").arg(QObject::tr("byte"),QObject::tr("word")));
        Instructions.insert("cbw",QObject::tr("convert %1 to %2.").arg(QObject::tr("byte"),QObject::tr("word")));
        Instructions.insert("cltd",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("double word"),"%eax",QObject::tr("quad word"),"%edx:%eax"));
        Instructions.insert("cdq",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("double word"),"%eax",QObject::tr("quad word"),"%edx:%eax"));
        Instructions.insert("cltq",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("double word"),"%eax",QObject::tr("quad word"),"%rax"));
        Instructions.insert("cdqe",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("double word"),"%eax",QObject::tr("quad word"),"%rax"));

        Instructions.insert("cmove",QObject::tr("Conditional move %1").arg(QObject::tr("if equal")));
        Instructions.insert("cmovz",QObject::tr("Conditional move %1").arg(QObject::tr("if zero.")));
        Instructions.insert("cmovne",QObject::tr("Conditional move %1").arg(QObject::tr("if not equal.")));
        Instructions.insert("cmovnz",QObject::tr("Conditional move %1").arg(QObject::tr("if not zero.")));

        Instructions.insert("cmovs",QObject::tr("Conditional move %1").arg(QObject::tr("if sign (negative).")));
        Instructions.insert("cmovns",QObject::tr("Conditional move %1").arg(QObject::tr("if not sign (non-negative).")));

        Instructions.insert("cmovg",QObject::tr("Conditional move %1").arg(QObject::tr("if greater(signed >).")));
        Instructions.insert("cmovnle",QObject::tr("Conditional move %1").arg(QObject::tr("if not less or equal(signed >).")));
        Instructions.insert("cmovge",QObject::tr("Conditional move %1").arg(QObject::tr("if greater or equal(signed >=).")));
        Instructions.insert("cmovnl",QObject::tr("Conditional move %1").arg(QObject::tr("if not less(signed >=).")));
        Instructions.insert("cmovl",QObject::tr("Conditional move %1").arg(QObject::tr("if less(signed <).")));
        Instructions.insert("cmovnge",QObject::tr("Conditional move %1").arg(QObject::tr("if not greater or equal(signed <).")));
        Instructions.insert("cmovle",QObject::tr("Conditional move %1").arg(QObject::tr("if less or equal(signed <=).")));
        Instructions.insert("cmovng",QObject::tr("Conditional move %1").arg(QObject::tr("if not greater(signed <=).")));

        Instructions.insert("cmova",QObject::tr("Conditional move %1").arg(QObject::tr("if above(unsigned >).")));
        Instructions.insert("cmovnbe",QObject::tr("Conditional move %1").arg(QObject::tr("if not below or equal(unsigned >).")));
        Instructions.insert("cmovae",QObject::tr("Conditional move %1").arg(QObject::tr("if above or equal(unsigned >=).")));
        Instructions.insert("cmovnb",QObject::tr("Conditional move %1").arg(QObject::tr("if not below(unsigned >=).")));
        Instructions.insert("cmovb",QObject::tr("Conditional move %1").arg(QObject::tr("if below(unsigned <).")));
        Instructions.insert("cmovnae",QObject::tr("Conditional move %1").arg(QObject::tr("if not above or equal(unsigned <).")));
        Instructions.insert("cmovbe",QObject::tr("Conditional move %1").arg(QObject::tr("if below or equal(unsigned <=).")));
        Instructions.insert("cmovna",QObject::tr("Conditional move %1").arg(QObject::tr("if not above(unsigned <=).")));

        Instructions.insert("cmovc",QObject::tr("Conditional move %1").arg(QObject::tr("if carry.")));
        Instructions.insert("cmovnc",QObject::tr("Conditional move %1").arg(QObject::tr("if not carry.")));
        Instructions.insert("cmovo",QObject::tr("Conditional move %1").arg(QObject::tr("if overflow.")));
        Instructions.insert("cmovno",QObject::tr("Conditional move %1").arg(QObject::tr("if not overflow.")));
        Instructions.insert("cmovp",QObject::tr("Conditional move %1").arg(QObject::tr("if parity.")));
        Instructions.insert("cmovpe",QObject::tr("Conditional move %1").arg(QObject::tr("if parity even.")));
        Instructions.insert("cmovnp",QObject::tr("Conditional move %1").arg(QObject::tr("if not parity.")));
        Instructions.insert("cmovpo",QObject::tr("Conditional move %1").arg(QObject::tr("if parity odd.")));

        Instructions.insert("cmpxchg",QObject::tr("Compare and exchange."));
        Instructions.insert("cmpxchg8b",QObject::tr("Compare and exchange 8 bytes."));
        Instructions.insert("cqto",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("quad word"),"%rax",QObject::tr("oct word"),"%rdx:%rax"));
        Instructions.insert("cqo",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("quad word"),"%rax",QObject::tr("oct word"),"%rdx:%rax"));
        Instructions.insert("cwtd",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("word"),"%ax",QObject::tr("double word"),"%dx:%ax"));
        Instructions.insert("cwd",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("word"),"%ax",QObject::tr("double word"),"%dx:%ax"));
        Instructions.insert("cwtl",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("word"),"%ax",QObject::tr("double word"),"%eax"));
        Instructions.insert("cwde",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("word"),"%ax",QObject::tr("double word"),"%eax"));
        Instructions.insert("mov",QObject::tr("move data between immediate values, general purpose registers, segment registers, and memory."));
        Instructions.insert("movb",QObject::tr("move %1 data between immediate values, general purpose registers, segment registers, and memory.").arg(QObject::tr("byte")));
        Instructions.insert("movw",QObject::tr("Move %1.").arg(QObject::tr("word")));
        Instructions.insert("movl",QObject::tr("Move %1.").arg(QObject::tr("double word")));
        Instructions.insert("movq",QObject::tr("Move %1.").arg(QObject::tr("quad word")));
        Instructions.insert("movabs",QObject::tr("move immediate value to register."));
        Instructions.insert("movabsb",QObject::tr("move immediate %1 value to register.").arg(QObject::tr("byte")));
        Instructions.insert("movabsw",QObject::tr("move immediate %1 value to register.").arg(QObject::tr("word")));
        Instructions.insert("movabsl",QObject::tr("move immediate %1 value to register.").arg(QObject::tr("double word")));
        Instructions.insert("movabsq",QObject::tr("move immediate %1 value to register.").arg(QObject::tr("quad word")));
        Instructions.insert("movabsa",QObject::tr("move immediate value to register %al/%ax/%eax/%rax."));
        Instructions.insert("movabsba",QObject::tr("move immediate %1 value to register %2.").arg(QObject::tr("byte"),"%al"));
        Instructions.insert("movabswa",QObject::tr("move immediate %1 value to register %2.").arg(QObject::tr("word"),"%ax"));
        Instructions.insert("movabsla",QObject::tr("move immediate %1 value to register %2.").arg(QObject::tr("double word"),"%eax"));
        Instructions.insert("movabsqa",QObject::tr("move immediate %1 value to register %2.").arg(QObject::tr("quad word"),"%rax"));
        Instructions.insert("movsx",QObject::tr("Move and sign extension.")); //intel
        Instructions.insert("movsbw",QObject::tr("Move sign-extended %1 to %2.").arg(QObject::tr("byte"),QObject::tr("word")));
        Instructions.insert("movsbl",QObject::tr("Move sign-extended %1 to %2.").arg(QObject::tr("byte"),QObject::tr("double word")));
        Instructions.insert("movswl",QObject::tr("Move sign-extended %1 to %2.").arg(QObject::tr("word"),QObject::tr("double word")));
        Instructions.insert("movsbq",QObject::tr("Move sign-extended %1 to %2.").arg(QObject::tr("byte"),QObject::tr("quad word")));
        Instructions.insert("movswq",QObject::tr("Move sign-extended %1 to %2.").arg(QObject::tr("word"),QObject::tr("quad word")));
        Instructions.insert("movslq",QObject::tr("Move sign-extended %1 to %2.").arg(QObject::tr("double word"),QObject::tr("quad word")));
        Instructions.insert("movzx",QObject::tr("Move with zero extension.")); //intel
        Instructions.insert("movzbw",QObject::tr("Move zero-extended %1 to %2.").arg(QObject::tr("byte"),QObject::tr("word")));
        Instructions.insert("movzbl",QObject::tr("Move zero-extended %1 to %2.").arg(QObject::tr("byte"),QObject::tr("double word")));
        Instructions.insert("movzwl",QObject::tr("Move zero-extended %1 to %2.").arg(QObject::tr("word"),QObject::tr("double word")));
        Instructions.insert("movzbq",QObject::tr("Move zero-extended %1 to %2.").arg(QObject::tr("byte"),QObject::tr("quad word")));
        Instructions.insert("movzwq",QObject::tr("Move zero-extended %1 to %2.").arg(QObject::tr("word"),QObject::tr("quad word")));
        Instructions.insert("pop",QObject::tr("Pop stack."));
        Instructions.insert("popw",QObject::tr("Pop %1 off stack.").arg(QObject::tr("word")));
        Instructions.insert("popl",QObject::tr("Pop %1 off stack.").arg(QObject::tr("double word")));
        Instructions.insert("popq",QObject::tr("Pop %1 off stack.").arg(QObject::tr("quad word")));
        Instructions.insert("popa",QObject::tr("Pop general-purpose registers from stack."));
        Instructions.insert("popaw",QObject::tr("Pop general-purpose registers from stack."));
        Instructions.insert("popad",QObject::tr("Pop general-purpose registers from stack."));
        Instructions.insert("push",QObject::tr("Push stack."));
        Instructions.insert("pushw",QObject::tr("Push %1 onto stack.").arg(QObject::tr("word")));
        Instructions.insert("pushl",QObject::tr("Push %1 onto stack.").arg(QObject::tr("double word")));
        Instructions.insert("pushq",QObject::tr("Push %1 onto stack.").arg(QObject::tr("quad word")));
        Instructions.insert("pusha",QObject::tr("Push general-purpose registers onto stack."));
        Instructions.insert("pushaw",QObject::tr("Push general-purpose registers onto stack."));
        Instructions.insert("pushal",QObject::tr("Push general-purpose registers onto stack."));
        Instructions.insert("xadd",QObject::tr("Exchange and add %1.").arg("integer"));
        Instructions.insert("xaddb",QObject::tr("Exchange and add %1.").arg("byte"));
        Instructions.insert("xaddw",QObject::tr("Exchange and add %1.").arg("word"));
        Instructions.insert("xaddl",QObject::tr("Exchange and add %1.").arg("double word"));
        Instructions.insert("xaddq",QObject::tr("Exchange and add %1.").arg("quad word"));
        Instructions.insert("xchg",QObject::tr("Exchange %1.").arg("integer"));
        Instructions.insert("xchgb",QObject::tr("Exchange %1.").arg("byte"));
        Instructions.insert("xchgw",QObject::tr("Exchange %1.").arg("word"));
        Instructions.insert("xchgl",QObject::tr("Exchange %1.").arg("double word"));
        Instructions.insert("xchgq",QObject::tr("Exchange %1.").arg("quad word"));
        //Binary Arithmetic Instructions
        Instructions.insert("adcx",QObject::tr("add unsigned %1 with carry.").arg(QObject::tr("integer"))); //intel
//        Instructions.insert("adcxb",QObject::tr("add unsigned %1 with carry.").arg(QObject::tr("byte")));
//        Instructions.insert("adcxw",QObject::tr("add unsigned %1 with carry.").arg(QObject::tr("word")));
//        Instructions.insert("adcxl",QObject::tr("add unsigned %1 with carry.").arg(QObject::tr("double word")));
//        Instructions.insert("adcxq",QObject::tr("add unsigned %1 with carry.").arg(QObject::tr("quad word")));
        Instructions.insert("ado",QObject::tr("add unsigned %1 with overflow.").arg(QObject::tr("integer"))); //intel
//        Instructions.insert("adob",QObject::tr("add unsigned %1 with overflow.").arg(QObject::tr("byte")));
//        Instructions.insert("adow",QObject::tr("add unsigned %1 with overflow.").arg(QObject::tr("word")));
//        Instructions.insert("adol",QObject::tr("add unsigned %1 with overflow.").arg(QObject::tr("double word")));
//        Instructions.insert("adoq",QObject::tr("add unsigned %1 with overflow.").arg(QObject::tr("quad word")));
        Instructions.insert("adc",QObject::tr("add %1 with carry.").arg(QObject::tr("integer")));
        Instructions.insert("adcb",QObject::tr("add %1 with carry.").arg(QObject::tr("byte")));
        Instructions.insert("adcw",QObject::tr("add %1 with carry.").arg(QObject::tr("word")));
        Instructions.insert("adcl",QObject::tr("add %1 with carry.").arg(QObject::tr("double word")));
        Instructions.insert("adcq",QObject::tr("add %1 with carry.").arg(QObject::tr("quad word")));
        Instructions.insert("add",QObject::tr("add %1.").arg(QObject::tr("integer")));
        Instructions.insert("addb",QObject::tr("add %1.").arg(QObject::tr("byte")));
        Instructions.insert("addw",QObject::tr("add %1.").arg(QObject::tr("word")));
        Instructions.insert("addl",QObject::tr("add %1.").arg(QObject::tr("double word")));
        Instructions.insert("addq",QObject::tr("add %1.").arg(QObject::tr("quad word")));
        Instructions.insert("cmp",QObject::tr("compare."));
        Instructions.insert("cmpb",QObject::tr("compare %1.").arg(QObject::tr("byte")));
        Instructions.insert("cmpw",QObject::tr("compare %1.").arg(QObject::tr("word")));
        Instructions.insert("cmpl",QObject::tr("compare %1.").arg(QObject::tr("double word")));
        Instructions.insert("cmpq",QObject::tr("compare %1.").arg(QObject::tr("quad word")));
        Instructions.insert("dec",QObject::tr("decrement by 1."));
        Instructions.insert("decb",QObject::tr("decrement %1 by 1.").arg(QObject::tr("byte")));
        Instructions.insert("decw",QObject::tr("decrement %1 by 1.").arg(QObject::tr("word")));
        Instructions.insert("decl",QObject::tr("decrement %1 by 1.").arg(QObject::tr("double word")));
        Instructions.insert("decq",QObject::tr("decrement %1 by 1.").arg(QObject::tr("quad word")));
        Instructions.insert("div",QObject::tr("unsigned %1 divide.").arg(QObject::tr("integer")));
        Instructions.insert("divb",QObject::tr("unsigned %1 divide.").arg(QObject::tr("byte")));
        Instructions.insert("divw",QObject::tr("unsigned %1 divide.").arg(QObject::tr("word")));
        Instructions.insert("divl",QObject::tr("unsigned %1 divide.").arg(QObject::tr("double word")));
        Instructions.insert("divq",QObject::tr("unsigned %1 divide.").arg(QObject::tr("quad word")));
        Instructions.insert("idiv",QObject::tr("signed %1 divide.").arg(QObject::tr("integer")));
        Instructions.insert("idivb",QObject::tr("signed %1 divide.").arg(QObject::tr("byte")));
        Instructions.insert("idivw",QObject::tr("signed %1 divide.").arg(QObject::tr("word")));
        Instructions.insert("idivl",QObject::tr("signed %1 divide.").arg(QObject::tr("double word")));
        Instructions.insert("idivq",QObject::tr("signed %1 divide.").arg(QObject::tr("quad word")));
        Instructions.insert("imul",QObject::tr("signed %1 multiply.").arg(QObject::tr("integer")));
        Instructions.insert("imulb",QObject::tr("signed %1 multiply.").arg(QObject::tr("byte")));
        Instructions.insert("imulw",QObject::tr("signed %1 multiply.").arg(QObject::tr("word")));
        Instructions.insert("imull",QObject::tr("signed %1 multiply.").arg(QObject::tr("double word")));
        Instructions.insert("imulq",QObject::tr("signed %1 multiply.").arg(QObject::tr("quad word")));
        Instructions.insert("inc",QObject::tr("increment by 1."));
        Instructions.insert("incb",QObject::tr("increment %1 by 1.").arg(QObject::tr("byte")));
        Instructions.insert("incw",QObject::tr("increment %1 by 1.").arg(QObject::tr("word")));
        Instructions.insert("incl",QObject::tr("increment %1 by 1.").arg(QObject::tr("double word")));
        Instructions.insert("incq",QObject::tr("increment %1 by 1.").arg(QObject::tr("quad word")));
        Instructions.insert("mul",QObject::tr("unsigned %1 multiply.").arg(QObject::tr("integer")));
        Instructions.insert("mulb",QObject::tr("unsigned %1 multiply.").arg(QObject::tr("byte")));
        Instructions.insert("mulw",QObject::tr("unsigned %1 multiply.").arg(QObject::tr("word")));
        Instructions.insert("mull",QObject::tr("unsigned %1 multiply.").arg(QObject::tr("double word")));
        Instructions.insert("mulq",QObject::tr("unsigned %1 multiply.").arg(QObject::tr("quad word")));
        Instructions.insert("neg",QObject::tr("Two's complement negation."));
        Instructions.insert("negb",QObject::tr("Replace the value of the %1 with its two's complement").arg("byte"));
        Instructions.insert("negw",QObject::tr("Replace the value of the %1 with its two's complement").arg("word"));
        Instructions.insert("negl",QObject::tr("Replace the value of the %1 with its two's complement").arg("double word"));
        Instructions.insert("negq",QObject::tr("Replace the value of the %1 with its two's complement").arg("quad word"));
        Instructions.insert("sbb",QObject::tr("subtract %1 with borrow.").arg(QObject::tr("integer")));
        Instructions.insert("sbbb",QObject::tr("subtract %1 with borrow.").arg(QObject::tr("byte")));
        Instructions.insert("sbbw",QObject::tr("subtract %1 with borrow.").arg(QObject::tr("word")));
        Instructions.insert("sbbl",QObject::tr("subtract %1 with borrow.").arg(QObject::tr("double word")));
        Instructions.insert("sbbq",QObject::tr("subtract %1 with borrow.").arg(QObject::tr("quad word")));
        Instructions.insert("sub",QObject::tr("subtract %1.").arg(QObject::tr("integer")));
        Instructions.insert("subb",QObject::tr("subtract %1.").arg(QObject::tr("byte")));
        Instructions.insert("subw",QObject::tr("subtract %1.").arg(QObject::tr("word")));
        Instructions.insert("subl",QObject::tr("subtract %1.").arg(QObject::tr("double word")));
        Instructions.insert("subq",QObject::tr("subtract %1.").arg(QObject::tr("quad word")));
        //Decimal Arithmetic Instructions
        Instructions.insert("aaa",QObject::tr("ascii adjust after addition."));
        Instructions.insert("aad",QObject::tr("ascii adjust before division."));
        Instructions.insert("aam",QObject::tr("ascii adjust after multiplication."));
        Instructions.insert("aas",QObject::tr("ascii adjust after subtraction."));
        Instructions.insert("daa",QObject::tr("decimal adjust after addition."));
        Instructions.insert("das",QObject::tr("decimal adjust after subtraction."));
        //Logical Instructions
        Instructions.insert("and",QObject::tr("bitwise logical AND."));
        Instructions.insert("andb",QObject::tr("bitwise logical AND on %1 values.").arg(QObject::tr("byte")));
        Instructions.insert("andw",QObject::tr("bitwise logical AND on %1 values.").arg(QObject::tr("word")));
        Instructions.insert("andl",QObject::tr("bitwise logical AND on %1 values.").arg(QObject::tr("double word")));
        Instructions.insert("andq",QObject::tr("bitwise logical AND on %1 values.").arg(QObject::tr("quad word")));
        Instructions.insert("not",QObject::tr("bitwise logical NOT."));
        Instructions.insert("notb",QObject::tr("bitwise logical NOT on %1 value.").arg(QObject::tr("byte")));
        Instructions.insert("notw",QObject::tr("bitwise logical NOT on %1 value.").arg(QObject::tr("word")));
        Instructions.insert("notl",QObject::tr("bitwise logical NOT on %1 value.").arg(QObject::tr("double word")));
        Instructions.insert("notq",QObject::tr("bitwise logical NOT on %1 value.").arg(QObject::tr("quad word")));
        Instructions.insert("or",QObject::tr("bitwise logical OR."));
        Instructions.insert("orb",QObject::tr("bitwise logical OR on %1 values.").arg(QObject::tr("byte")));
        Instructions.insert("orw",QObject::tr("bitwise logical OR on %1 values.").arg(QObject::tr("word")));
        Instructions.insert("orl",QObject::tr("bitwise logical OR on %1 values.").arg(QObject::tr("double word")));
        Instructions.insert("orq",QObject::tr("bitwise logical OR on %1 values.").arg(QObject::tr("quad word")));
        Instructions.insert("xor",QObject::tr("bitwise logical XOR."));
        Instructions.insert("xorb",QObject::tr("bitwise logical XOR on %1 values.").arg(QObject::tr("byte")));
        Instructions.insert("xorw",QObject::tr("bitwise logical XOR on %1 values.").arg(QObject::tr("word")));
        Instructions.insert("xorl",QObject::tr("bitwise logical XOR on %1 values.").arg(QObject::tr("double word")));
        Instructions.insert("xorq",QObject::tr("bitwise logical XOR on %1 values.").arg(QObject::tr("quad word")));
        //Shift and Rotate Instructions
        Instructions.insert("rcl",QObject::tr("rotate %1 through carry left.").arg(QObject::tr("integer")));
        Instructions.insert("rclb",QObject::tr("rotate %1 through carry left.").arg(QObject::tr("byte")));
        Instructions.insert("rclw",QObject::tr("rotate %1 through carry left.").arg(QObject::tr("word")));
        Instructions.insert("rcll",QObject::tr("rotate %1 through carry left.").arg(QObject::tr("double word")));
        Instructions.insert("rclq",QObject::tr("rotate %1 through carry left.").arg(QObject::tr("quad word")));
        Instructions.insert("rcr",QObject::tr("rotate %1 through carry right.").arg(QObject::tr("integer")));
        Instructions.insert("rcrb",QObject::tr("rotate %1 through carry right.").arg(QObject::tr("byte")));
        Instructions.insert("rcrw",QObject::tr("rotate %1 through carry right.").arg(QObject::tr("word")));
        Instructions.insert("rcrl",QObject::tr("rotate %1 through carry right.").arg(QObject::tr("double word")));
        Instructions.insert("rcrq",QObject::tr("rotate %1 through carry right.").arg(QObject::tr("quad word")));
        Instructions.insert("rol",QObject::tr("rotate %1 left.").arg(QObject::tr("integer")));
        Instructions.insert("rolb",QObject::tr("rotate %1 left.").arg(QObject::tr("byte")));
        Instructions.insert("rolw",QObject::tr("rotate %1 left.").arg(QObject::tr("word")));
        Instructions.insert("roll",QObject::tr("rotate %1 left.").arg(QObject::tr("double word")));
        Instructions.insert("rolq",QObject::tr("rotate %1 left.").arg(QObject::tr("quad word")));
        Instructions.insert("ror",QObject::tr("rotate %1 right.").arg(QObject::tr("integer")));
        Instructions.insert("rorb",QObject::tr("rotate %1 right.").arg(QObject::tr("byte")));
        Instructions.insert("rorw",QObject::tr("rotate %1 right.").arg(QObject::tr("word")));
        Instructions.insert("rorl",QObject::tr("rotate %1 right.").arg(QObject::tr("double word")));
        Instructions.insert("rorq",QObject::tr("rotate %1 right.").arg(QObject::tr("quad word")));
        Instructions.insert("sal",QObject::tr("shift %1 arithmetic left.").arg(QObject::tr("integer")));
        Instructions.insert("salb",QObject::tr("shift %1 arithmetic left.").arg(QObject::tr("byte")));
        Instructions.insert("salw",QObject::tr("shift %1 arithmetic left.").arg(QObject::tr("word")));
        Instructions.insert("sall",QObject::tr("shift %1 arithmetic left.").arg(QObject::tr("double word")));
        Instructions.insert("salq",QObject::tr("shift %1 arithmetic left.").arg(QObject::tr("quad word")));
        Instructions.insert("sar",QObject::tr("shift %1 arithmetic right.").arg(QObject::tr("integer")));
        Instructions.insert("sarb",QObject::tr("shift %1 arithmetic right.").arg(QObject::tr("byte")));
        Instructions.insert("sarw",QObject::tr("shift %1 arithmetic right.").arg(QObject::tr("word")));
        Instructions.insert("sarl",QObject::tr("shift %1 arithmetic right.").arg(QObject::tr("double word")));
        Instructions.insert("sarq",QObject::tr("shift %1 arithmetic right.").arg(QObject::tr("quad word")));
        Instructions.insert("shl",QObject::tr("shift %1 logical left.").arg(QObject::tr("integer")));
        Instructions.insert("shlb",QObject::tr("shift %1 logical left.").arg(QObject::tr("byte")));
        Instructions.insert("shlw",QObject::tr("shift %1 logical left.").arg(QObject::tr("word")));
        Instructions.insert("shll",QObject::tr("shift %1 logical left.").arg(QObject::tr("double word")));
        Instructions.insert("shlq",QObject::tr("shift %1 logical left.").arg(QObject::tr("quad word")));
        Instructions.insert("shr",QObject::tr("shift %1 logical right.").arg(QObject::tr("integer")));
        Instructions.insert("shrb",QObject::tr("shift %1 logical right.").arg(QObject::tr("byte")));
        Instructions.insert("shrw",QObject::tr("shift %1 logical right.").arg(QObject::tr("word")));
        Instructions.insert("shrl",QObject::tr("shift %1 logical right.").arg(QObject::tr("double word")));
        Instructions.insert("shrq",QObject::tr("shift %1 logical right.").arg(QObject::tr("quad word")));
        Instructions.insert("shld",QObject::tr("shift %1 left double.").arg(QObject::tr("integer")));
        Instructions.insert("shldb",QObject::tr("shift %1 left double.").arg(QObject::tr("byte")));
        Instructions.insert("shldw",QObject::tr("shift %1 left double.").arg(QObject::tr("word")));
        Instructions.insert("shldl",QObject::tr("shift %1 left double.").arg(QObject::tr("double word")));
        Instructions.insert("shldq",QObject::tr("shift %1 left double.").arg(QObject::tr("quad word")));
        Instructions.insert("shrd",QObject::tr("shift %1 right double.").arg(QObject::tr("integer")));
        Instructions.insert("shrdb",QObject::tr("shift %1 right double.").arg(QObject::tr("byte")));
        Instructions.insert("shrdw",QObject::tr("shift %1 right double.").arg(QObject::tr("word")));
        Instructions.insert("shrdl",QObject::tr("shift %1 right double.").arg(QObject::tr("double word")));
        Instructions.insert("shrdq",QObject::tr("shift %1 right double.").arg(QObject::tr("quad word")));
        //Bit and Byte Instructions
        Instructions.insert("bsf",QObject::tr("bit scan forward."));
        Instructions.insert("bsfw",QObject::tr("bit scan forward in the %1 operand.").arg(QObject::tr("word")));
        Instructions.insert("bsfl",QObject::tr("bit scan forward in the %1 operand.").arg(QObject::tr("double word")));
        Instructions.insert("bsfq",QObject::tr("bit scan forward in the %1 operand.").arg(QObject::tr("quad word")));
        Instructions.insert("bsr",QObject::tr("bit scan reserve."));
        Instructions.insert("bsrw",QObject::tr("bit scan reserve in the %1 operand.").arg(QObject::tr("word")));
        Instructions.insert("bsrl",QObject::tr("bit scan reserve in the %1 operand.").arg(QObject::tr("double word")));
        Instructions.insert("bsrq",QObject::tr("bit scan reserve in the %1 operand.").arg(QObject::tr("quad word")));
        Instructions.insert("bt",QObject::tr("bit test."));
        Instructions.insert("btw",QObject::tr("bit test in the %1 operand.").arg(QObject::tr("word")));
        Instructions.insert("btl",QObject::tr("bit test in the %1 operand.").arg(QObject::tr("double word")));
        Instructions.insert("btq",QObject::tr("bit test in the %1 operand.").arg(QObject::tr("quad word")));
        Instructions.insert("btc",QObject::tr("bit test and complement."));
        Instructions.insert("btcw",QObject::tr("bit test and complement in the %1 operand.").arg(QObject::tr("word")));
        Instructions.insert("btcl",QObject::tr("bit test and complement in the %1 operand.").arg(QObject::tr("double word")));
        Instructions.insert("btcq",QObject::tr("bit test and complement in the %1 operand.").arg(QObject::tr("quad word")));
        Instructions.insert("btr",QObject::tr("bit test and reset."));
        Instructions.insert("btrw",QObject::tr("bit test and reset in the %1 operand.").arg(QObject::tr("word")));
        Instructions.insert("btrl",QObject::tr("bit test and reset in the %1 operand.").arg(QObject::tr("double word")));
        Instructions.insert("btrq",QObject::tr("bit test and reset in the %1 operand.").arg(QObject::tr("quad word")));
        Instructions.insert("bts",QObject::tr("bit test and set."));
        Instructions.insert("btsw",QObject::tr("bit test and set in the %1 operand.").arg(QObject::tr("word")));
        Instructions.insert("btsl",QObject::tr("bit test and set in the %1 operand.").arg(QObject::tr("double word")));
        Instructions.insert("btsq",QObject::tr("bit test and set in the %1 operand.").arg(QObject::tr("quad word")));

        Instructions.insert("sete",QObject::tr("set byte %1").arg(QObject::tr("if equal.")));
        Instructions.insert("setz",QObject::tr("set byte %1").arg(QObject::tr("if zero.")));
        Instructions.insert("setne",QObject::tr("set byte %1").arg(QObject::tr("if not equal.")));
        Instructions.insert("setnz",QObject::tr("set byte %1").arg(QObject::tr("if not zero.")));

        Instructions.insert("sets",QObject::tr("set byte %1").arg(QObject::tr("if sign (negative).")));
        Instructions.insert("setns",QObject::tr("set byte %1").arg(QObject::tr("if not sign (non-negative).")));

        Instructions.insert("setg",QObject::tr("set byte %1").arg(QObject::tr("if greater(signed >).")));
        Instructions.insert("setnle",QObject::tr("set byte %1").arg(QObject::tr("if not less or equal(signed >).")));
        Instructions.insert("setge",QObject::tr("set byte %1").arg(QObject::tr("if greater or equal(signed >=).")));
        Instructions.insert("setnl",QObject::tr("set byte %1").arg(QObject::tr("if not less(signed >=).")));
        Instructions.insert("setl",QObject::tr("set byte %1").arg(QObject::tr("if less(siged <).")));
        Instructions.insert("setnge",QObject::tr("set byte %1").arg(QObject::tr("if not greater or equal(siged <).")));
        Instructions.insert("setle",QObject::tr("set byte %1").arg(QObject::tr("if less or equal(siged <=).")));
        Instructions.insert("setng",QObject::tr("set byte %1").arg(QObject::tr("if not greater(siged <=).")));

        Instructions.insert("seta",QObject::tr("set byte %1").arg(QObject::tr("if above(unsigned >).")));
        Instructions.insert("setnbe",QObject::tr("set byte %1").arg(QObject::tr("if not below or equal(unsigned >).")));
        Instructions.insert("setae",QObject::tr("set byte %1").arg(QObject::tr("if above or equal(unsigned >=).")));
        Instructions.insert("setnb",QObject::tr("set byte %1").arg(QObject::tr("if not below(unsigned >=).")));
        Instructions.insert("setb",QObject::tr("set byte %1").arg(QObject::tr("if below(unsigned <).")));
        Instructions.insert("setnae",QObject::tr("set byte %1").arg(QObject::tr("if not above or equal(unsigned <).")));
        Instructions.insert("setbe",QObject::tr("set byte %1").arg(QObject::tr("if below or equal(unsigned <=).")));
        Instructions.insert("setna",QObject::tr("set byte %1").arg(QObject::tr("if not above(unsigned <=).")));

        Instructions.insert("setc",QObject::tr("set byte %1").arg(QObject::tr("if carry.")));
        Instructions.insert("setnc",QObject::tr("set byte %1").arg(QObject::tr("if not carry.")));
        Instructions.insert("setno",QObject::tr("set byte %1").arg(QObject::tr("if not overflow.")));
        Instructions.insert("setnp",QObject::tr("set byte %1").arg(QObject::tr("if not parity.")));
        Instructions.insert("seto",QObject::tr("set byte %1").arg(QObject::tr("if overflow.")));
        Instructions.insert("setp",QObject::tr("set byte %1").arg(QObject::tr("if parity.")));
        Instructions.insert("setpe",QObject::tr("set byte %1").arg(QObject::tr("if parity even.")));
        Instructions.insert("setpo",QObject::tr("set byte %1").arg(QObject::tr("if parity odd.")));

        Instructions.insert("test",QObject::tr("logical compare."));
        Instructions.insert("testb",QObject::tr("logical compare %1.").arg(QObject::tr("byte")));
        Instructions.insert("testw",QObject::tr("logical compare %1.").arg(QObject::tr("word")));
        Instructions.insert("testl",QObject::tr("logical compare %1.").arg(QObject::tr("double word")));
        Instructions.insert("testq",QObject::tr("logical compare %1.").arg(QObject::tr("quad word")));

        //Control Transfer Instructions
        Instructions.insert("bound",QObject::tr("detect value out of range."));
        Instructions.insert("boundw",QObject::tr("detect %1 value out of range.").arg(QObject::tr("word")));
        Instructions.insert("boundl",QObject::tr("detect %1 value out of range.").arg(QObject::tr("double word")));
        Instructions.insert("call",QObject::tr("call procedure."));
        Instructions.insert("enter",QObject::tr("high-level procedure entry."));
        Instructions.insert("int",QObject::tr("software interrupt."));
        Instructions.insert("into",QObject::tr("interrupt on overflow."));
        Instructions.insert("iret",QObject::tr("return from interrupt."));

        Instructions.insert("jmp",QObject::tr("jump."));
        Instructions.insert("je",QObject::tr("jump %1").arg(QObject::tr("if equal.")));
        Instructions.insert("jz",QObject::tr("jump %1").arg(QObject::tr("if zero.")));
        Instructions.insert("jne",QObject::tr("jump %1").arg(QObject::tr("if not equal.")));
        Instructions.insert("jnz",QObject::tr("jump %1").arg(QObject::tr("if not zero.")));

        Instructions.insert("js",QObject::tr("jump %1").arg(QObject::tr("if sign (negative).")));
        Instructions.insert("jns",QObject::tr("jump %1").arg(QObject::tr("if not sign (non-negative).")));

        Instructions.insert("jg",QObject::tr("jump %1").arg(QObject::tr("if greater(signed >).")));
        Instructions.insert("jnle",QObject::tr("jump %1").arg(QObject::tr("if not less or equal(signed >).")));
        Instructions.insert("jge",QObject::tr("jump %1").arg(QObject::tr("if greater or equal(signed >=).")));
        Instructions.insert("jnl",QObject::tr("jump %1").arg(QObject::tr("if not less(signed >=).")));
        Instructions.insert("jl",QObject::tr("jump %1").arg(QObject::tr("if less(signed <).")));
        Instructions.insert("jnge",QObject::tr("jump %1").arg(QObject::tr("if not greater or equal(signed <).")));
        Instructions.insert("jle",QObject::tr("jump %1").arg(QObject::tr("if less or equal(signed <=).")));
        Instructions.insert("jng",QObject::tr("jump %1").arg(QObject::tr("if not greater(signed <=).")));

        Instructions.insert("ja",QObject::tr("jump %1").arg(QObject::tr("if above(unsigned >).")));
        Instructions.insert("jnbe",QObject::tr("jump %1").arg(QObject::tr("if not below or equal(unsigned >).")));
        Instructions.insert("jae",QObject::tr("jump %1").arg(QObject::tr("if above or equal(unsigned >=).")));
        Instructions.insert("jnb",QObject::tr("jump %1").arg(QObject::tr("if not below(unsigned >=).")));
        Instructions.insert("jb",QObject::tr("jump %1").arg(QObject::tr("if below(unsigned <).")));
        Instructions.insert("jnae",QObject::tr("jump %1").arg(QObject::tr("if not above or equal(unsigned <).")));
        Instructions.insert("jbe",QObject::tr("jump %1").arg(QObject::tr("if below or equal(unsigned <=).")));
        Instructions.insert("jna",QObject::tr("jump %1").arg(QObject::tr("if not above(unsigned <=).")));

        Instructions.insert("jc",QObject::tr("jump %1").arg(QObject::tr("if carry.")));
        Instructions.insert("jcxz",QObject::tr("jump %1").arg(QObject::tr("register %cx zero")));
        Instructions.insert("jecxz",QObject::tr("jump %1").arg(QObject::tr("register %ecx zero")));
        Instructions.insert("jnc",QObject::tr("jump %1").arg(QObject::tr("if not carry.")));
        Instructions.insert("jno",QObject::tr("jump %1").arg(QObject::tr("if not overflow.")));
        Instructions.insert("jnp",QObject::tr("jump %1").arg(QObject::tr("if not parity.")));
        Instructions.insert("jo",QObject::tr("jump %1").arg(QObject::tr("if overflow.")));
        Instructions.insert("jp",QObject::tr("jump %1").arg(QObject::tr("if parity.")));
        Instructions.insert("jpe",QObject::tr("jump %1").arg(QObject::tr("if parity even.")));
        Instructions.insert("jpo",QObject::tr("jump %1").arg(QObject::tr("if parity odd.")));

        Instructions.insert("lcall",QObject::tr("call far procedure."));
        Instructions.insert("leave",QObject::tr("high-level procedure exit."));
        Instructions.insert("loop",QObject::tr("loop with %ecx counter"));
        Instructions.insert("loope",QObject::tr("loop with %ecx and equal"));
        Instructions.insert("loopne",QObject::tr("loop with %ecx and not equal"));
        Instructions.insert("loopnz",QObject::tr("loop with %ecx and not zero"));
        Instructions.insert("loopz",QObject::tr("loop with %ecx and zero"));
        Instructions.insert("lret",QObject::tr("return from far procedure."));
        Instructions.insert("ret",QObject::tr("return."));

        //String Instructions
        Instructions.insert("coms",QObject::tr("compare string."));
        Instructions.insert("cpmsb",QObject::tr("compare %1 string.").arg(QObject::tr("byte")));
        Instructions.insert("cmpsw",QObject::tr("compare %1 string.").arg(QObject::tr("word")));
        Instructions.insert("cmpsl",QObject::tr("compare %1 string.").arg(QObject::tr("double word")));
        Instructions.insert("cmpsq",QObject::tr("compare %1 string.").arg(QObject::tr("quad word")));
        Instructions.insert("lods",QObject::tr("load string."));
        Instructions.insert("lodsb",QObject::tr("load %1 string.").arg(QObject::tr("byte")));
        Instructions.insert("lodsw",QObject::tr("load %1 string.").arg(QObject::tr("word")));
        Instructions.insert("lodsl",QObject::tr("load %1 string.").arg(QObject::tr("double word")));
        Instructions.insert("lodsq",QObject::tr("load %1 string.").arg(QObject::tr("quad word")));
        Instructions.insert("movs",QObject::tr("move string."));
        Instructions.insert("movsb",QObject::tr("move %1 string.").arg(QObject::tr("byte")));
        Instructions.insert("movsw",QObject::tr("move %1 string.").arg(QObject::tr("word")));
        Instructions.insert("movsl",QObject::tr("move %1 string.").arg(QObject::tr("double word")));
        Instructions.insert("movsq",QObject::tr("move %1 string.").arg(QObject::tr("quad word")));
        Instructions.insert("rep",QObject::tr("repeat while %ecx not zero"));
        Instructions.insert("repnz",QObject::tr("repeat while not equal."));
        Instructions.insert("repnz",QObject::tr("repeat while not zero."));
        Instructions.insert("repz",QObject::tr("repeat while equal."));
        Instructions.insert("repz",QObject::tr("repeat while zero."));
        Instructions.insert("scas",QObject::tr("scan string."));
        Instructions.insert("scasb",QObject::tr("scan %1 string.").arg(QObject::tr("byte")));
        Instructions.insert("scasw",QObject::tr("scan %1 string.").arg(QObject::tr("word")));
        Instructions.insert("scasl",QObject::tr("scan %1 string.").arg(QObject::tr("double word")));
        Instructions.insert("scasq",QObject::tr("scan %1 string.").arg(QObject::tr("quad word")));
        Instructions.insert("stos",QObject::tr("store string."));
        Instructions.insert("stosb",QObject::tr("store %1 string.").arg(QObject::tr("byte")));
        Instructions.insert("stosw",QObject::tr("store %1 string.").arg(QObject::tr("word")));
        Instructions.insert("stosl",QObject::tr("store %1 string.").arg(QObject::tr("double word")));
        Instructions.insert("stosq",QObject::tr("store %1 string.").arg(QObject::tr("quad word")));
        //I/O Instructions
        Instructions.insert("in",QObject::tr("read from a port."));
        Instructions.insert("ins",QObject::tr("input string from a port."));
        Instructions.insert("insb",QObject::tr("input byte string from port."));
        Instructions.insert("insl",QObject::tr("input double word string from port."));
        Instructions.insert("insw",QObject::tr("input word string from port."));
        Instructions.insert("out",QObject::tr("write to a port."));
        Instructions.insert("outs",QObject::tr("output string to port."));
        Instructions.insert("outsb",QObject::tr("output byte string to port."));
        Instructions.insert("outsl",QObject::tr("output double word string to port."));
        Instructions.insert("outsw",QObject::tr("output word string to port."));
        //Flag Control (EFLAG) Instructions
        Instructions.insert("clc",QObject::tr("clear carry flag."));
        Instructions.insert("cld",QObject::tr("clear direction flag."));
        Instructions.insert("cli",QObject::tr("clear interrupt flag."));
        Instructions.insert("cmc",QObject::tr("complement carry flag."));
        Instructions.insert("lahf",QObject::tr("load flags into %ah register"));
        Instructions.insert("popfw",QObject::tr("pop %eflags from stack"));
        Instructions.insert("popf{lq}",QObject::tr("pop %eflags from stack"));
        Instructions.insert("pushfw",QObject::tr("push %eflags onto stack"));
        Instructions.insert("pushf{lq}",QObject::tr("push %eflags onto stack"));
        Instructions.insert("sahf",QObject::tr("store %ah register into flags"));
        Instructions.insert("stc",QObject::tr("set carry flag."));
        Instructions.insert("std",QObject::tr("set direction flag."));
        Instructions.insert("sti",QObject::tr("set interrupt flag."));
        //Segment Register Instructions
        Instructions.insert("lds",QObject::tr("load far pointer using %ds"));
        Instructions.insert("les",QObject::tr("load far pointer using %es"));
        Instructions.insert("lfs",QObject::tr("load far pointer using %fs"));
        Instructions.insert("lgs",QObject::tr("load far pointer using %gs"));
        Instructions.insert("lss",QObject::tr("load far pointer using %ss"));

        Instructions.insert("cpuid",QObject::tr("processor identification."));
        Instructions.insert("lea",QObject::tr("load effective address."));
        Instructions.insert("leaw",QObject::tr("load effective address."));
        Instructions.insert("leal",QObject::tr("load effective address."));
        Instructions.insert("leaq",QObject::tr("load effective address."));
        Instructions.insert("nop",QObject::tr("no operation."));
        Instructions.insert("ud2",QObject::tr("undefined instruction."));
        Instructions.insert("xlat",QObject::tr("table lookup translation."));
        Instructions.insert("xlatb",QObject::tr("table lookup translation."));

        //Floating-Point Instructions
        //Data Transfer Instructions (Floating Point)
        Instructions.insert("fbld",QObject::tr("load bcd."));
        Instructions.insert("fbstp",QObject::tr("store bcd and pop."));
        Instructions.insert("fcmovb",QObject::tr("floating-point conditional move if below."));
        Instructions.insert("fcmovbe",QObject::tr("floating-point conditional move if below or equal."));
        Instructions.insert("fcmove",QObject::tr("floating-point conditional move if equal."));
        Instructions.insert("fcmovnb",QObject::tr("floating-point conditional move if not below."));
        Instructions.insert("fcmovnbe",QObject::tr("floating-point conditional move if not below or equal."));
        Instructions.insert("fcmovne",QObject::tr("floating-point conditional move if not equal."));
        Instructions.insert("fcmovnu",QObject::tr("floating-point conditional move if unordered."));
        Instructions.insert("fcmovu",QObject::tr("floating-point conditional move if unordered."));
        Instructions.insert("fild",QObject::tr("load integer."));
        Instructions.insert("fist",QObject::tr("store integer."));
        Instructions.insert("fistp",QObject::tr("store integer and pop."));
        Instructions.insert("fld",QObject::tr("load floating-point value."));
        Instructions.insert("fst",QObject::tr("store floating-point value."));
        Instructions.insert("fstp",QObject::tr("store floating-point value and pop."));
        Instructions.insert("fxch",QObject::tr("exchange registers ."));
        //Basic Arithmetic Instructions (Floating-Point)
        Instructions.insert("fabs",QObject::tr("absolute value."));
        Instructions.insert("fadd",QObject::tr("add floating-point."));
        Instructions.insert("faddp",QObject::tr("add floating-point and pop."));
        Instructions.insert("fchs",QObject::tr("change sign."));
        Instructions.insert("fdiv",QObject::tr("divide floating-point."));
        Instructions.insert("fdivp",QObject::tr("divide floating-point and pop."));
        Instructions.insert("fdivr",QObject::tr("divide floating-point reverse."));
        Instructions.insert("fdivrp",QObject::tr("divide floating-point reverse and pop."));
        Instructions.insert("fiadd",QObject::tr("add integer."));
        Instructions.insert("fidiv",QObject::tr("divide integer."));
        Instructions.insert("fidivr",QObject::tr("divide integer reverse."));
        Instructions.insert("fimul",QObject::tr("multiply integer."));
        Instructions.insert("fisub",QObject::tr("subtract integer."));
        Instructions.insert("fisubr",QObject::tr("subtract integer reverse."));
        Instructions.insert("fmul",QObject::tr("multiply floating-point."));
        Instructions.insert("fmulp",QObject::tr("multiply floating-point and pop."));
        Instructions.insert("fprem",QObject::tr("partial remainder."));
        Instructions.insert("fprem1",QObject::tr("ieee partial remainder."));
        Instructions.insert("frndint",QObject::tr("round to integer."));
        Instructions.insert("fscale",QObject::tr("scale by power of two."));
        Instructions.insert("fsqrt",QObject::tr("square root."));
        Instructions.insert("fsub",QObject::tr("subtract floating-point."));
        Instructions.insert("fsubp",QObject::tr("subtract floating-point and pop."));
        Instructions.insert("fsubr",QObject::tr("subtract floating-point reverse."));
        Instructions.insert("fsubrp",QObject::tr("subtract floating-point reverse and pop."));
        Instructions.insert("fxtract",QObject::tr("extract exponent and significand ."));
        //Comparison Instructions (Floating-Point)
        Instructions.insert("fcom",QObject::tr("compare floating-point."));
        Instructions.insert("fcomi",QObject::tr("compare floating-point and set %eflags."));
        Instructions.insert("fcomip",QObject::tr("compare floating-point, set %eflags, and pop."));
        Instructions.insert("fcomp",QObject::tr("compare floating-point and pop."));
        Instructions.insert("fcompp",QObject::tr("compare floating-point and pop twice."));
        Instructions.insert("ficom",QObject::tr("compare integer."));
        Instructions.insert("ficomp",QObject::tr("compare integer and pop."));
        Instructions.insert("ftst",QObject::tr("test floating-point (compare with 0.0)."));
        Instructions.insert("fucom",QObject::tr("unordered compare floating-point."));
        Instructions.insert("fucomi",QObject::tr("unordered compare floating-point and set %eflags."));
        Instructions.insert("fucomip",QObject::tr("unordered compare floating-point, set %eflags, and pop."));
        Instructions.insert("fucomp",QObject::tr("unordered compare floating-point and pop."));
        Instructions.insert("fucompp",QObject::tr("compare floating-point and pop twice."));
        Instructions.insert("fxam",QObject::tr("examine floating-point ."));
        Instructions.insert("transcendental",QObject::tr("instructions (floating-point)	."));
        Instructions.insert("the",QObject::tr("transcendental instructions perform trigonometric and logarithmic operations on floating-point operands.	."));
        Instructions.insert("table",QObject::tr("3–16 transcendental instructions (floating-point)."));
        Instructions.insert("solaris",QObject::tr("mnemonic 	description."));
        Instructions.insert("f2xm1",QObject::tr("computes 2x-1."));
        Instructions.insert("fcos",QObject::tr("cosine."));
        Instructions.insert("fpatan",QObject::tr("partial arctangent."));
        Instructions.insert("fptan",QObject::tr("partial tangent."));
        Instructions.insert("fsin",QObject::tr("sine."));
        Instructions.insert("fsincos",QObject::tr("sine and cosine."));
        Instructions.insert("fyl2x",QObject::tr("computes y * log2x."));
        Instructions.insert("fyl2xp1",QObject::tr("computes y * log2(x+1)."));
        //Load Constants (Floating-Point) Instructions
        Instructions.insert("fld1",QObject::tr("load +1.0."));
        Instructions.insert("fldl2e",QObject::tr("load log2e."));
        Instructions.insert("fldl2t",QObject::tr("load log210."));
        Instructions.insert("fldlg2",QObject::tr("load log102."));
        Instructions.insert("fldln2",QObject::tr("load loge2."));
        Instructions.insert("fldpi",QObject::tr("load π."));
        Instructions.insert("fldz",QObject::tr("load +0.0 ."));
        //Control Instructions (Floating-Point)
        Instructions.insert("fclex",QObject::tr("clear floating-point exception flags after checking for error conditions."));
        Instructions.insert("fdecstp",QObject::tr("decrement floating-point register stack pointer."));
        Instructions.insert("ffree",QObject::tr("free floating-point register."));
        Instructions.insert("fincstp",QObject::tr("increment floating-point register stack pointer."));
        Instructions.insert("finit",QObject::tr("initialize floating-point unit after checking error conditions."));
        Instructions.insert("fldcw",QObject::tr("load floating-point unit control word."));
        Instructions.insert("fldenv",QObject::tr("load floating-point unit environment."));
        Instructions.insert("fnclex",QObject::tr("clear floating-point exception flags without checking for error conditions."));
        Instructions.insert("fninit",QObject::tr("initialize floating-point unit without checking error conditions."));
        Instructions.insert("fnop",QObject::tr("floating-point no operation."));
        Instructions.insert("fnsave",QObject::tr("save floating-point unit state without checking error conditions."));
        Instructions.insert("fnstcw",QObject::tr("store floating-point unit control word without checking error conditions."));
        Instructions.insert("fnstenv",QObject::tr("store floating-point unit environment without checking error conditions."));
        Instructions.insert("fnstsw",QObject::tr("store floating-point unit status word without checking error conditions."));
        Instructions.insert("frstor",QObject::tr("restore floating-point unit state."));
        Instructions.insert("fsave",QObject::tr("save floating-point unit state after checking error conditions."));
        Instructions.insert("fstcw",QObject::tr("store floating-point unit control word after checking error conditions."));
        Instructions.insert("fstenv",QObject::tr("store floating-point unit environment after checking error conditions."));
        Instructions.insert("fstsw",QObject::tr("store floating-point unit status word after checking error conditions."));
        Instructions.insert("fwait",QObject::tr("wait for floating-point unit."));
        Instructions.insert("wait",QObject::tr("wait for floating-point unit."));

        //SIMD State Management Instructions
        Instructions.insert("fxrstor",QObject::tr("restore floating-point unit and simd state."));
        Instructions.insert("fxsave",QObject::tr("save floating-point unit and simd state."));

        //MMX Instructions
        //Data Transfer Instructions (MMX)
        Instructions.insert("movd",QObject::tr("Move %1.").arg(QObject::tr("double word")));
        //Conversion Instructions (MMX)
        Instructions.insert("packssdw",QObject::tr("pack doublewords into words with signed saturation."));
        Instructions.insert("packsswb",QObject::tr("pack words into bytes with signed saturation."));
        Instructions.insert("packuswb",QObject::tr("pack words into bytes with unsigned saturation."));
        Instructions.insert("punpckhbw",QObject::tr("unpack high-order bytes."));
        Instructions.insert("punpckhdq",QObject::tr("unpack high-order doublewords."));
        Instructions.insert("punpckhwd",QObject::tr("unpack high-order words."));
        Instructions.insert("punpcklbw",QObject::tr("unpack low-order bytes."));
        Instructions.insert("punpckldq",QObject::tr("unpack low-order doublewords."));
        Instructions.insert("punpcklwd",QObject::tr("unpack low-order words."));
        //Packed Arithmetic Instructions (MMX)
        Instructions.insert("paddb",QObject::tr("add packed byte integers."));
        Instructions.insert("paddd",QObject::tr("add packed doubleword integers."));
        Instructions.insert("paddsb",QObject::tr("add packed signed byte integers with signed saturation."));
        Instructions.insert("paddsw",QObject::tr("add packed signed word integers with signed saturation."));
        Instructions.insert("paddusb",QObject::tr("add packed unsigned byte integers with unsigned saturation."));
        Instructions.insert("paddusw",QObject::tr("add packed unsigned word integers with unsigned saturation."));
        Instructions.insert("paddw",QObject::tr("add packed word integers."));
        Instructions.insert("pmaddwd",QObject::tr("multiply and add packed word integers."));
        Instructions.insert("pmulhw",QObject::tr("multiply packed signed word integers and store high result."));
        Instructions.insert("pmullw",QObject::tr("multiply packed signed word integers and store low result."));
        Instructions.insert("psubb",QObject::tr("subtract packed byte integers."));
        Instructions.insert("psubd",QObject::tr("subtract packed doubleword integers."));
        Instructions.insert("psubsb",QObject::tr("subtract packed signed byte integers with signed saturation."));
        Instructions.insert("psubsw",QObject::tr("subtract packed signed word integers with signed saturation."));
        Instructions.insert("psubusb",QObject::tr("subtract packed unsigned byte integers with unsigned saturation."));
        Instructions.insert("psubusw",QObject::tr("subtract packed unsigned word integers with unsigned saturation."));
        Instructions.insert("psubw",QObject::tr("subtract packed word integers."));
        //Comparison Instructions (MMX)
        Instructions.insert("pcmpeqb",QObject::tr("compare packed bytes for equal."));
        Instructions.insert("pcmpeqd",QObject::tr("compare packed doublewords for equal."));
        Instructions.insert("pcmpeqw",QObject::tr("compare packed words for equal."));
        Instructions.insert("pcmpgtb",QObject::tr("compare packed signed byte integers for greater than."));
        Instructions.insert("pcmpgtd",QObject::tr("compare packed signed doubleword integers for greater than."));
        Instructions.insert("pcmpgtw",QObject::tr("compare packed signed word integers for greater than."));
        //Logical Instructions (MMX)
        Instructions.insert("pand",QObject::tr("bitwise logical and."));
        Instructions.insert("pandn",QObject::tr("bitwise logical and not."));
        Instructions.insert("por",QObject::tr("bitwise logical or."));
        Instructions.insert("pxor",QObject::tr("bitwise logical xor."));
        //Shift and Rotate Instructions (MMX)
        Instructions.insert("pslld",QObject::tr("shift packed doublewords left logical."));
        Instructions.insert("psllq",QObject::tr("shift packed quadword left logical."));
        Instructions.insert("psllw",QObject::tr("shift packed words left logical."));
        Instructions.insert("psrad",QObject::tr("shift packed doublewords right arithmetic."));
        Instructions.insert("psraw",QObject::tr("shift packed words right arithmetic."));
        Instructions.insert("psrld",QObject::tr("shift packed doublewords right logical."));
        Instructions.insert("psrlq",QObject::tr("shift packed quadword right logical."));
        Instructions.insert("psrlw",QObject::tr("shift packed words right logical."));
        //State Management Instructions (MMX)
        Instructions.insert("emms",QObject::tr("empty mmx state."));

        //SSE Instructions
        //SIMD Single-Precision Floating-Point Instructions (SSE)
        //Data Transfer Instructions (SSE)
        Instructions.insert("solaris",QObject::tr("mnemonic 	description."));
        Instructions.insert("movaps",QObject::tr("move four aligned packed single-precision floating-point values between xmm registers or memory."));
        Instructions.insert("movhlps",QObject::tr("move two packed single-precision floating-point values from the high quadword of an xmm register to the low quadword of another xmm register."));
        Instructions.insert("movhps",QObject::tr("move two packed single-precision floating-point values to or from the high quadword of an xmm register or memory."));
        Instructions.insert("movlhps",QObject::tr("move two packed single-precision floating-point values from the low quadword of an xmm register to the high quadword of another xmm register."));
        Instructions.insert("movlps",QObject::tr("move two packed single-precision floating-point values to or from the low quadword of an xmm register or memory."));
        Instructions.insert("movmskps",QObject::tr("extract sign mask from four packed single-precision floating-point values."));
        Instructions.insert("movss",QObject::tr("move scalar single-precision floating-point value between xmm registers or memory."));
        Instructions.insert("movups",QObject::tr("move four unaligned packed single-precision floating-point values between xmm registers or memory."));
        //Packed Arithmetic Instructions (SSE)
        Instructions.insert("addps",QObject::tr("add packed single-precision floating-point values."));
        Instructions.insert("addss",QObject::tr("add scalar single-precision floating-point values."));
        Instructions.insert("divps",QObject::tr("divide packed single-precision floating-point values."));
        Instructions.insert("divss",QObject::tr("divide scalar single-precision floating-point values."));
        Instructions.insert("maxps",QObject::tr("return maximum packed single-precision floating-point values."));
        Instructions.insert("maxss",QObject::tr("return maximum scalar single-precision floating-point values."));
        Instructions.insert("minps",QObject::tr("return minimum packed single-precision floating-point values."));
        Instructions.insert("minss",QObject::tr("return minimum scalar single-precision floating-point values.."));
        Instructions.insert("mulps",QObject::tr("multiply packed single-precision floating-point values."));
        Instructions.insert("mulss",QObject::tr("multiply scalar single-precision floating-point values."));
        Instructions.insert("rcpps",QObject::tr("compute reciprocals of packed single-precision floating-point values."));
        Instructions.insert("rcpss",QObject::tr("compute reciprocal of scalar single-precision floating-point values."));
        Instructions.insert("rsqrtps",QObject::tr("compute reciprocals of square roots of packed single-precision floating-point values."));
        Instructions.insert("rsqrtss",QObject::tr("compute reciprocal of square root of scalar single-precision floating-point values."));
        Instructions.insert("sqrtps",QObject::tr("compute square roots of packed single-precision floating-point values."));
        Instructions.insert("sqrtss",QObject::tr("compute square root of scalar single-precision floating-point values."));
        Instructions.insert("subps",QObject::tr("subtract packed single-precision floating-point values."));
        Instructions.insert("subss",QObject::tr("subtract scalar single-precision floating-point values."));
        //Comparison Instructions (SSE)
        Instructions.insert("cmpps",QObject::tr("compare packed single-precision floating-point values."));
        Instructions.insert("cmpss",QObject::tr("compare scalar single-precision floating-point values."));
        Instructions.insert("comiss",QObject::tr("perform ordered comparison of scalar single-precision floating-point values and set flags in eflags register."));
        Instructions.insert("ucomiss",QObject::tr("perform unordered comparison of scalar single-precision floating-point values and set flags in eflags register."));
        //Logical Instructions (SSE)
        Instructions.insert("andnps",QObject::tr("perform bitwise logical and not of packed single-precision floating-point values."));
        Instructions.insert("andps",QObject::tr("perform bitwise logical and of packed single-precision floating-point values."));
        Instructions.insert("orps",QObject::tr("perform bitwise logical or of packed single-precision floating-point values."));
        Instructions.insert("xorps",QObject::tr("perform bitwise logical xor of packed single-precision floating-point values."));
        //Shuffle and Unpack Instructions (SSE)
        Instructions.insert("shufps",QObject::tr("shuffles values in packed single-precision floating-point operands."));
        Instructions.insert("unpckhps",QObject::tr("unpacks and interleaves the two high-order values from two single-precision floating-point operands."));
        Instructions.insert("unpcklps",QObject::tr("unpacks and interleaves the two low-order values from two single-precision floating-point operands."));
        //Conversion Instructions (SSE)
        Instructions.insert("cvtpi2ps",QObject::tr("convert packed doubleword integers to packed single-precision floating-point values."));
        Instructions.insert("cvtps2pi",QObject::tr("convert packed single-precision floating-point values to packed doubleword integers."));
        Instructions.insert("cvtsi2ss",QObject::tr("convert doubleword integer to scalar single-precision floating-point value."));
        Instructions.insert("cvtss2si",QObject::tr("convert scalar single-precision floating-point value to a doubleword integer."));
        Instructions.insert("cvttps2pi",QObject::tr("convert with truncation packed single-precision floating-point values to packed doubleword integers."));
        Instructions.insert("cvttss2si",QObject::tr("convert with truncation scalar single-precision floating-point value to scalar doubleword integer."));
        //MXCSR State Management Instructions (SSE)
        Instructions.insert("ldmxcsr",QObject::tr("load %mxcsr register."));
        Instructions.insert("stmxcsr",QObject::tr("save %mxcsr register state."));
        //64–Bit SIMD Integer Instructions (SSE)
        Instructions.insert("pavgb",QObject::tr("compute average of packed unsigned byte integers."));
        Instructions.insert("pavgw",QObject::tr("compute average of packed unsigned byte integers."));
        Instructions.insert("pextrw",QObject::tr("extract word."));
        Instructions.insert("pinsrw",QObject::tr("insert word."));
        Instructions.insert("pmaxsw",QObject::tr("maximum of packed signed word integers."));
        Instructions.insert("pmaxub",QObject::tr("maximum of packed unsigned byte integers."));
        Instructions.insert("pminsw",QObject::tr("minimum of packed signed word integers."));
        Instructions.insert("pminub",QObject::tr("minimum of packed unsigned byte integers."));
        Instructions.insert("pmovmskb",QObject::tr("move byte mask."));
        Instructions.insert("pmulhuw",QObject::tr("multiply packed unsigned integers and store high result."));
        Instructions.insert("psadbw",QObject::tr("compute sum of absolute differences."));
        Instructions.insert("pshufw",QObject::tr("shuffle packed integer word in mmx register."));
        //Miscellaneous Instructions (SSE)
        Instructions.insert("maskmovq",QObject::tr("non-temporal store of selected bytes from an mmx register into memory."));
        Instructions.insert("movntps",QObject::tr("non-temporal store of four packed single-precision floating-point values from an xmm register into memory."));
        Instructions.insert("movntq",QObject::tr("non-temporal store of quadword from an mmx register into memory."));
        Instructions.insert("prefetchnta",QObject::tr("prefetch data into non-temporal cache structure and into a location close to the processor."));
        Instructions.insert("prefetcht0",QObject::tr("prefetch data into all levels of the cache hierarchy."));
        Instructions.insert("prefetcht1",QObject::tr("prefetch data into level 2 cache and higher."));
        Instructions.insert("prefetcht2",QObject::tr("prefetch data into level 2 cache and higher."));
        Instructions.insert("sfence",QObject::tr("serialize store operations."));

        //SSE2 Instructions
        //SSE2 Packed and Scalar Double-Precision Floating-Point Instructions
        //SSE2 Data Movement Instructions
        Instructions.insert("movapd",QObject::tr("move two aligned packed double-precision floating-point values between xmm registers and memory."));
        Instructions.insert("movhpd",QObject::tr("move high packed double-precision floating-point value to or from the high quadword of an xmm register and memory."));
        Instructions.insert("movlpd",QObject::tr("move low packed single-precision floating-point value to or from the low quadword of an xmm register and memory."));
        Instructions.insert("movmskpd",QObject::tr("extract sign mask from two packed double-precision floating-point values."));
        Instructions.insert("movsd",QObject::tr("move scalar double-precision floating-point value between xmm registers and memory.."));
        Instructions.insert("movupd",QObject::tr("move two unaligned packed double-precision floating-point values between xmm registers and memory."));
        //SSE2 Packed Arithmetic Instructions
        Instructions.insert("addpd",QObject::tr("add packed double-precision floating-point values."));
        Instructions.insert("addsd",QObject::tr("add scalar double-precision floating-point values."));
        Instructions.insert("divpd",QObject::tr("divide packed double-precision floating-point values."));
        Instructions.insert("divsd",QObject::tr("divide scalar double-precision floating-point values."));
        Instructions.insert("maxpd",QObject::tr("return maximum packed double-precision floating-point values."));
        Instructions.insert("maxsd",QObject::tr("return maximum scalar double-precision floating-point value."));
        Instructions.insert("minpd",QObject::tr("return minimum packed double-precision floating-point values."));
        Instructions.insert("minsd",QObject::tr("return minimum scalar double-precision floating-point value."));
        Instructions.insert("mulpd",QObject::tr("multiply packed double-precision floating-point values."));
        Instructions.insert("mulsd",QObject::tr("multiply scalar double-precision floating-point values."));
        Instructions.insert("sqrtpd",QObject::tr("compute packed square roots of packed double-precision floating-point values."));
        Instructions.insert("sqrtsd",QObject::tr("compute scalar square root of scalar double-precision floating-point value."));
        Instructions.insert("subpd",QObject::tr("subtract packed double-precision floating-point values."));
        Instructions.insert("subsd",QObject::tr("subtract scalar double-precision floating-point values."));
        //SSE2 Logical Instructions
        Instructions.insert("andnpd",QObject::tr("perform bitwise logical and not of packed double-precision floating-point values."));
        Instructions.insert("andpd",QObject::tr("perform bitwise logical and of packed double-precision floating-point values."));
        Instructions.insert("orpd",QObject::tr("perform bitwise logical or of packed double-precision floating-point values."));
        Instructions.insert("xorpd",QObject::tr("perform bitwise logical xor of packed double-precision floating-point values."));
        //SSE2 Compare Instructions
        Instructions.insert("cmppd",QObject::tr("compare packed double-precision floating-point values."));
        Instructions.insert("cmpsd",QObject::tr("compare scalar double-precision floating-point values."));
        Instructions.insert("comisd",QObject::tr("perform ordered comparison of scalar double-precision floating-point values and set flags in eflags register."));
        Instructions.insert("ucomisd",QObject::tr("perform unordered comparison of scalar double-precision floating-point values and set flags in eflags register."));
        //SSE2 Shuffle and Unpack Instructions
        Instructions.insert("shufpd",QObject::tr("shuffle values in packed double-precision floating-point operands."));
        Instructions.insert("unpckhpd",QObject::tr("unpack and interleave the high values from two packed double-precision floating-point operands."));
        Instructions.insert("unpcklpd",QObject::tr("unpack and interleave the low values from two packed double-precision floating-point operands."));
        //SSE2 Conversion Instructions
        Instructions.insert("cvtdq2pd",QObject::tr("convert packed doubleword integers to packed double-precision floating-point values."));
        Instructions.insert("cvtpd2dq",QObject::tr("convert packed double-precision floating-point values to packed doubleword integers."));
        Instructions.insert("cvtpd2pi",QObject::tr("convert packed double-precision floating-point values to packed doubleword integers."));
        Instructions.insert("cvtpd2ps",QObject::tr("convert packed double-precision floating-point values to packed single-precision floating-point values."));
        Instructions.insert("cvtpi2pd",QObject::tr("convert packed doubleword integers to packed double-precision floating-point values."));
        Instructions.insert("cvtps2pd",QObject::tr("convert packed single-precision floating-point values to packed double-precision floating-point values."));
        Instructions.insert("cvtsd2si",QObject::tr("convert scalar double-precision floating-point values to a doubleword integer."));
        Instructions.insert("cvtsd2ss",QObject::tr("convert scalar double-precision floating-point values to scalar single-precision floating-point values."));
        Instructions.insert("cvtsi2sd",QObject::tr("convert doubleword integer to scalar double-precision floating-point value."));
        Instructions.insert("cvtss2sd",QObject::tr("convert scalar single-precision floating-point values to scalar double-precision floating-point values."));
        Instructions.insert("cvttpd2dq",QObject::tr("convert with truncation packed double-precision floating-point values to packed doubleword integers."));
        Instructions.insert("cvttpd2pi",QObject::tr("convert with truncation packed double-precision floating-point values to packed doubleword integers."));
        Instructions.insert("cvttsd2si",QObject::tr("convert with truncation scalar double-precision floating-point values to scalar doubleword integers."));
        //SSE2 Packed Single-Precision Floating-Point Instructions
        Instructions.insert("cvtdq2ps",QObject::tr("convert packed doubleword integers to packed single-precision floating-point values."));
        Instructions.insert("cvtps2dq",QObject::tr("convert packed single-precision floating-point values to packed doubleword integers."));
        Instructions.insert("cvttps2dq",QObject::tr("convert with truncation packed single-precision floating-point values to packed doubleword integers."));
        //SSE2 128–Bit SIMD Integer Instructions
        Instructions.insert("movdq2q",QObject::tr("move quadword integer from xmm to mmx registers."));
        Instructions.insert("movdqa",QObject::tr("move aligned double quadword."));
        Instructions.insert("movdqu",QObject::tr("move unaligned double quadword."));
        Instructions.insert("movq2dq",QObject::tr("move quadword integer from mmx to xmm registers."));
        Instructions.insert("paddq",QObject::tr("add packed quadword integers."));
        Instructions.insert("pmuludq",QObject::tr("multiply packed unsigned doubleword integers."));
        Instructions.insert("pshufd",QObject::tr("shuffle packed doublewords."));
        Instructions.insert("pshufhw",QObject::tr("shuffle packed high words."));
        Instructions.insert("pshuflw",QObject::tr("shuffle packed low words."));
        Instructions.insert("pslldq",QObject::tr("shift double quadword left logical."));
        Instructions.insert("psrldq",QObject::tr("shift double quadword right logical."));
        Instructions.insert("psubq",QObject::tr("subtract packed quadword integers."));
        Instructions.insert("punpckhqdq",QObject::tr("unpack high quadwords."));
        Instructions.insert("punpcklqdq",QObject::tr("unpack low quadwords."));
        //SSE2 Miscellaneous Instructions
        Instructions.insert("clflush",QObject::tr("flushes and invalidates a memory operand and its associated cache line from all levels of the processor's cache hierarchy."));
        Instructions.insert("lfence",QObject::tr("serializes load operations."));
        Instructions.insert("maskmovdqu",QObject::tr("non-temporal store of selected bytes from an xmm register into memory."));
        Instructions.insert("mfence",QObject::tr("serializes load and store operations."));
        Instructions.insert("movntdq",QObject::tr("non-temporal store of double quadword from an xmm register into memory."));
        Instructions.insert("movnti",QObject::tr("non-temporal store of a doubleword from a general-purpose register into memory."));
        Instructions.insert("movntpd",QObject::tr("non-temporal store of two packed double-precision floating-point values from an xmm register into memory."));
        Instructions.insert("pause",QObject::tr("improves the performance of spin-wait loops."));

        //AVX/AVX2 Instructions
        Instructions.insert("vaddpd",QObject::tr("Add Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vaddps",QObject::tr("Add Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vaddsd",QObject::tr("Add Scalar Double-Precision Floating-Point Values."));
        Instructions.insert("vaddss",QObject::tr("Add Scalar Single-Precision Floating-Point Values."));
        Instructions.insert("vaddsubpd",QObject::tr("Packed Double-FP Add/Subtract."));
        Instructions.insert("vaddsubps",QObject::tr("Packed Single-FP Add/Subtract."));
        Instructions.insert("vandnpd",QObject::tr("Bitwise Logical AND NOT of Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vandnps",QObject::tr("Bitwise Logical AND NOT of Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vandpd",QObject::tr("Bitwise Logical AND of Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vandps",QObject::tr("Bitwise Logical AND of Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vblendpd",QObject::tr("Blend Packed Double Precision Floating-Point Values."));
        Instructions.insert("vblendps",QObject::tr("Blend Packed Single Precision Floating-Point Values."));
        Instructions.insert("vblendvpd",QObject::tr("Variable Blend Packed Double Precision Floating-Point Values."));
        Instructions.insert("vblendvps",QObject::tr("Variable Blend Packed Single Precision Floating-Point Values."));
        Instructions.insert("vcmpeq_ospd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpeq_uqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpeq_uspd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpeqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpfalse_ospd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpfalsepd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpge_oqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpgepd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpgt_oqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpgtpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmple_oqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmplepd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmplt_oqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpltpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpneq_oqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpneq_ospd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpneq_uspd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpneqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpnge_uqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpngepd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpngt_uqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpngtpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpnle_uqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpnlepd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpnlt_uqpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpnltpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpord_spd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpordpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmppd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmptrue_uspd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmptruepd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpunord_spd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpunordpd",QObject::tr("Compare Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcmpeq_osps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpeq_uqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpeq_usps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpeqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpfalse_osps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpfalseps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpge_oqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpgeps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpgt_oqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpgtps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmple_oqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpleps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmplt_oqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpltps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpneq_oqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpneq_osps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpneq_usps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpneqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpnge_uqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpngeps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpngt_uqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpngtps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpnle_uqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpnleps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpnlt_uqps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpnltps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpord_sps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpordps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmptrue_usps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmptrueps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpunord_sps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpunordps",QObject::tr("Compare Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcmpeq_ossd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpeq_uqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpeq_ussd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpeqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpfalse_ossd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpfalsesd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpge_oqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpgesd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpgt_oqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpgtsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmple_oqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmplesd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmplt_oqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpltsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpneq_oqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpneq_ossd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpneq_ussd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpneqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpnge_uqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpngesd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpngt_uqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpngtsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpnle_uqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpnlesd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpnlt_uqsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpnltsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpord_ssd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpordsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmptrue_ussd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmptruesd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpunord_ssd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpunordsd",QObject::tr("Compare Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcmpeq_osss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpeq_uqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpeq_usss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpeqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpfalse_osss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpfalsess",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpge_oqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpgess",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpgt_oqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpgtss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmple_oqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpless",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmplt_oqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpltss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpneq_oqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpneq_osss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpneq_usss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpneqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpnge_uqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpngess",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpngt_uqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpngtss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpnle_uqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpnless",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpnlt_uqss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpnltss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpord_sss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpordss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmptrue_usss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmptruess",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpunord_sss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcmpunordss",QObject::tr("Compare Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcomisd",QObject::tr("Compare Scalar Ordered Double-Precision Floating-Point Values and Set EFLAGS."));
        Instructions.insert("vcomiss",QObject::tr("Compare Scalar Ordered Single-Precision Floating-Point Values and Set EFLAGS."));
        Instructions.insert("vcvtdq2pd",QObject::tr("Convert Packed Doubleword Integers to Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vcvtdq2ps",QObject::tr("Convert Packed Doubleword Integers to Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcvtpd2dq",QObject::tr("Convert Packed Double-Precision Floating-Point Values to Packed Doubleword Integers."));
        Instructions.insert("vcvtpd2dqx",QObject::tr("Convert Packed Double-Precision Floating-Point Values to Packed Doubleword Integers."));
        Instructions.insert("vcvtpd2dqy",QObject::tr("Convert Packed Double-Precision Floating-Point Values to Packed Doubleword Integers."));
        Instructions.insert("vcvtpd2ps",QObject::tr("Convert Packed Double-Precision Floating-Point Values to Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcvtpd2psx",QObject::tr("Convert Packed Double-Precision Floating-Point Values to Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcvtpd2psy",QObject::tr("Convert Packed Double-Precision Floating-Point Values to Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vcvtps2dq",QObject::tr("Convert Packed Single-Precision Floating-Point Values to Packed Signed Doubleword Integer Values."));
        Instructions.insert("vcvtps2pd",QObject::tr("Convert Packed Single-Precision Floating-Point Values to Packed Double-Precision Floating-Point."));
        Instructions.insert("vcvtsd2si",QObject::tr("Convert Scalar Double-Precision Floating-Point Value to Doubleword Integer."));
        Instructions.insert("vcvtsd2siq",QObject::tr("Convert Scalar Double-Precision Floating-Point Value to quad word Integer."));
        Instructions.insert("vcvtsd2sil",QObject::tr("Convert Scalar Double-Precision Floating-Point Value to double word Integer."));
        Instructions.insert("vcvtsd2ss",QObject::tr("Convert Scalar Double-Precision Floating-Point Value to Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcvtsi2sd",QObject::tr("Convert Doubleword Integer to Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcvtsi2sdq",QObject::tr("Convert quad word Integer to Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcvtsi2sdl",QObject::tr("Convert double word Integer to Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcvtsi2ss",QObject::tr("Convert Doubleword Integer to Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcvtsi2ssq",QObject::tr("Convert quad word Integer to Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcvtsi2ssl",QObject::tr("Convert Doubleword Integer to Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vcvtss2sd",QObject::tr("Convert Scalar Single-Precision Floating-Point Value to Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vcvtss2si",QObject::tr("Convert Scalar Single-Precision Floating-Point Value to Doubleword Integer."));
        Instructions.insert("vcvtss2siq",QObject::tr("Convert Scalar Single-Precision Floating-Point Value to quad word Integer."));
        Instructions.insert("vcvtss2sil",QObject::tr("Convert Scalar Single-Precision Floating-Point Value to Doubleword Integer."));
        Instructions.insert("vcvttpd2dq",QObject::tr("Convert with Truncation Packed Double-Precision Floating-Point Values to Packed Doubleword."));
        Instructions.insert("vcvttpd2dqx",QObject::tr("Convert with Truncation Packed Double-Precision Floating-Point Values to Packed Doubleword."));
        Instructions.insert("vcvttpd2dqy",QObject::tr("Convert with Truncation Packed Double-Precision Floating-Point Values to Packed Doubleword."));
        Instructions.insert("vcvttps2dq",QObject::tr("Convert with Truncation Packed Single-Precision Floating-Point Values to Packed Signed Doubleword."));
        Instructions.insert("vcvttsd2si",QObject::tr("Convert with Truncation Scalar Double-Precision Floating-Point Value to Signed Integer."));
        Instructions.insert("vcvttsd2siq",QObject::tr("Convert with Truncation Scalar Double-Precision Floating-Point Value to Signed quad word Integer."));
        Instructions.insert("vcvttsd2sil",QObject::tr("Convert with Truncation Scalar Double-Precision Floating-Point Value to Signed double word Integer."));
        Instructions.insert("vcvttss2si",QObject::tr("Convert with Truncation Scalar Single-Precision Floating-Point Value to Integer."));
        Instructions.insert("vcvttss2siq",QObject::tr("Convert with Truncation Scalar Single-Precision Floating-Point Value to quad word Integer."));
        Instructions.insert("vcvttss2sil",QObject::tr("Convert with Truncation Scalar Single-Precision Floating-Point Value to double word Integer."));
        Instructions.insert("vdivpd",QObject::tr("Divide Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vdivps",QObject::tr("Divide Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vdivsd",QObject::tr("Divide Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vdivss",QObject::tr("Divide Scalar Single-Precision Floating-Point Values."));
        Instructions.insert("vdppd",QObject::tr("Dot Product of Packed Double Precision Floating-Point Values."));
        Instructions.insert("vdpps",QObject::tr("Dot Product of Packed Single Precision Floating-Point Values."));
        Instructions.insert("vextractps",QObject::tr("Extract Packed Floating-Point Values."));
        Instructions.insert("vhaddpd",QObject::tr("Packed Double-FP Horizontal Add."));
        Instructions.insert("vhaddps",QObject::tr("Packed Single-FP Horizontal Add."));
        Instructions.insert("vhsubpd",QObject::tr("Packed Double-FP Horizontal Subtract."));
        Instructions.insert("vhsubps",QObject::tr("Packed Single-FP Horizontal Subtract."));
        Instructions.insert("vinsertps",QObject::tr("Insert Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vlddqu",QObject::tr("Load Unaligned Integer 128 Bits."));
        Instructions.insert("vldmxcsr",QObject::tr("Load MXCSR Register."));
        Instructions.insert("vmaskmovdqu",QObject::tr("Store Selected Bytes of Double Quadword."));
        Instructions.insert("vmaxpd",QObject::tr("Maximum of Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vmaxps",QObject::tr("Maximum of Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vmaxsd",QObject::tr("Return Maximum Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vmaxss",QObject::tr("Return Maximum Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vminpd",QObject::tr("Minimum of Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vminps",QObject::tr("Minimum of Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vminsd",QObject::tr("Return Minimum Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vminss",QObject::tr("Return Minimum Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vmovapd",QObject::tr("Move Aligned Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vmovaps",QObject::tr("Move Aligned Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vmov",QObject::tr("Move Doubleword and Quadword."));
        Instructions.insert("vmovq",QObject::tr("Move Quadword."));
        Instructions.insert("vmovd",QObject::tr("Move Doubleword."));
        Instructions.insert("vmovddup",QObject::tr("Replicate Double FP Values."));
        Instructions.insert("vmovdqa",QObject::tr("Move Aligned Packed Integer Values."));
        Instructions.insert("vmovdqu",QObject::tr("Move Unaligned Packed Integer Values."));


        Instructions.insert("vmovhlps",QObject::tr("Move Packed Single-Precision Floating-Point Values High to Low."));
        Instructions.insert("vmovhpd",QObject::tr("Move High Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vmovhps",QObject::tr("Move High Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vmovlhps",QObject::tr("Move Packed Single-Precision Floating-Point Values Low to High."));
        Instructions.insert("vmovlpd",QObject::tr("Move Low Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vmovlps",QObject::tr("Move Low Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vmovmskpd",QObject::tr("Extract Packed Double-Precision Floating-Point Sign Mask."));
        Instructions.insert("vmovmskps",QObject::tr("Extract Packed Single-Precision Floating-Point Sign Mask."));
        Instructions.insert("vmovntdq",QObject::tr("Store Packed Integers Using Non-Temporal Hint."));
        Instructions.insert("vmovntdqa",QObject::tr("Load Double Quadword Non-Temporal Aligned Hint."));
        Instructions.insert("vmovntpd",QObject::tr("Store Packed Double-Precision Floating-Point Values Using Non-Temporal Hint."));
        Instructions.insert("vmovntps",QObject::tr("Store Packed Single-Precision Floating-Point Values Using Non-Temporal Hint."));
        Instructions.insert("vmovq",QObject::tr("Move Quadword."));
        Instructions.insert("vmovsd",QObject::tr("Move or Merge Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vmovshdup",QObject::tr("Replicate Single FP Values."));
        Instructions.insert("vmovsldup",QObject::tr("Replicate Single FP Values."));
        Instructions.insert("vmovss",QObject::tr("Move or Merge Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vmovupd",QObject::tr("Move Unaligned Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vmovups",QObject::tr("Move Unaligned Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vmpsadbw",QObject::tr("Compute Multiple Packed Sums of Absolute Difference."));
        Instructions.insert("vmulpd",QObject::tr("Multiply Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vmulps",QObject::tr("Multiply Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vmulsd",QObject::tr("Multiply Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vmulss",QObject::tr("Multiply Scalar Single-Precision Floating-Point Values."));
        Instructions.insert("vorpd",QObject::tr("Bitwise Logical OR of Double-Precision Floating-Point Values."));
        Instructions.insert("vorps",QObject::tr("Bitwise Logical OR of Single-Precision Floating-Point Values."));
        Instructions.insert("vpabsq",QObject::tr("Packed Absolute Quadword Value."));
        Instructions.insert("vpabsw",QObject::tr("Packed Absolute Word Value."));
        Instructions.insert("vpabsb",QObject::tr("Packed Absolute Byte Value."));
        Instructions.insert("vpabsd)",QObject::tr("Packed Absolute Doubleword Value."));
        Instructions.insert("vpackssdw",QObject::tr("Pack with Signed Saturation."));
        Instructions.insert("vpacksswb",QObject::tr("Pack with Signed Saturation."));
        Instructions.insert("vpackusdw",QObject::tr("Pack with Unsigned Saturation."));
        Instructions.insert("vpackuswb",QObject::tr("Pack with Unsigned Saturation."));
        Instructions.insert("vpaddq",QObject::tr("Add Packed Quadword Integers."));
        Instructions.insert("vpaddw",QObject::tr("Add Packed Word Integers."));
        Instructions.insert("vpaddb",QObject::tr("Add Packed Byte Integers."));
        Instructions.insert("vpaddd",QObject::tr("Add Packed Doubleword Integers."));

        Instructions.insert("vpaddsw",QObject::tr("Add Packed Signed Word Integers with Signed Saturation."));
        Instructions.insert("vpaddsb",QObject::tr("Add Packed Signed Byte Integers with Signed Saturation."));

        Instructions.insert("vpaddusw",QObject::tr("Add Packed Unsigned Word Integers with Unsigned Saturation."));
        Instructions.insert("vpaddusb",QObject::tr("Add Packed Unsigned Byte Integers with Unsigned Saturation."));

        Instructions.insert("vpalignr",QObject::tr("Packed Align Right."));
        Instructions.insert("vpand",QObject::tr("Logical AND."));
        Instructions.insert("vpandn",QObject::tr("Logical AND NOT."));
        Instructions.insert("vpavgw",QObject::tr("Average Packed Word Integers."));
        Instructions.insert("vpavgb",QObject::tr("Average Packed Byte Integers."));

        Instructions.insert("vpblendvb",QObject::tr("Variable Blend Packed Bytes."));
        Instructions.insert("vpblendw",QObject::tr("Blend Packed Words."));
        Instructions.insert("vpcmpeqq",QObject::tr("Compare Packed Quadword Integers for Equality."));
        Instructions.insert("vpcmpeqw",QObject::tr("Compare Packed Word Integers for Equality."));
        Instructions.insert("vpcmpeqb",QObject::tr("Compare Packed Byte Integers for Equality."));
        Instructions.insert("vpcmpeqd",QObject::tr("Compare Packed Doubleword Integers for Equality."));

        Instructions.insert("vpcmpestri",QObject::tr("Packed Compare Explicit Length Strings, Return Index."));
        Instructions.insert("vpcmpestrm",QObject::tr("Packed Compare Explicit Length Strings, Return Mask."));
        Instructions.insert("vpcmpgtq",QObject::tr("Compare Packed Quadword Integers for Greater Than."));
        Instructions.insert("vpcmpgtw",QObject::tr("Compare Packed Word Integers for Greater Than."));
        Instructions.insert("vpcmpgtb",QObject::tr("Compare Packed Byte Integers for Greater Than."));
        Instructions.insert("vpcmpgtd",QObject::tr("Compare Packed Doubleword Integers for Greater Than."));

        Instructions.insert("vpcmpistri",QObject::tr("Packed Compare Implicit Length Strings, Return Index."));
        Instructions.insert("vpcmpistrm",QObject::tr("Packed Compare Implicit Length Strings, Return Mask."));
        Instructions.insert("vpextrq",QObject::tr("Extract Quadword."));
        Instructions.insert("vpextrb",QObject::tr("Extract Byte."));
        Instructions.insert("vpextrd",QObject::tr("Extract Doubleword."));


        Instructions.insert("vpextrw",QObject::tr("Extract Word."));
        Instructions.insert("vphaddsw",QObject::tr("Packed Horizontal Add and Saturate."));
        Instructions.insert("vphaddw",QObject::tr("Packed Horizontal Add Word."));
        Instructions.insert("vphaddd",QObject::tr("Packed Horizontal Add Doubleword."));

        Instructions.insert("vphminposuw",QObject::tr("Packed Horizontal Word Minimum."));
        Instructions.insert("vphsubsw",QObject::tr("Packed Horizontal Subtract and Saturate."));
        Instructions.insert("vphsubw",QObject::tr("Packed Horizontal Subtract Word."));
        Instructions.insert("vphsubd",QObject::tr("Packed Horizontal Subtract Doubleword."));
        Instructions.insert("vpinsrq",QObject::tr("Insert Quadword."));
        Instructions.insert("vpinsrb",QObject::tr("Insert Byte."));
        Instructions.insert("vpinsrw",QObject::tr("Insert Word."));
        Instructions.insert("vpinsrd",QObject::tr("Insert Doubleword."));
        Instructions.insert("vpinsrw",QObject::tr("Insert Word."));
        Instructions.insert("vpmaddubsw",QObject::tr("Multiply and Add Packed Signed and Unsigned Bytes."));
        Instructions.insert("vpmaddwd",QObject::tr("Multiply and Add Packed Integers."));
        Instructions.insert("vvpmaxsq",QObject::tr("Maximum of Packed Signed Quadword Integers."));
        Instructions.insert("vvpmaxsw",QObject::tr("Maximum of Packed Signed Word Integers."));
        Instructions.insert("vvpmaxsb",QObject::tr("Maximum of Packed Signed Byte Integers."));
        Instructions.insert("vvpmaxsd",QObject::tr("Maximum of Packed Signed Doubleword Integers."));
        Instructions.insert("vpmaxub",QObject::tr("Maximum of Packed Unsigned Byte Integers."));
        Instructions.insert("vpmaxud",QObject::tr("Maximum of Packed Unsigned Integers."));
        Instructions.insert("vpmaxuw",QObject::tr("Maximum of Packed Word Integers."));
        Instructions.insert("vpminsb",QObject::tr("Minimum of Packed Signed Byte Integers."));
        Instructions.insert("vpminsd",QObject::tr("Minimum of Packed Signed Integers."));
        Instructions.insert("vpminsw",QObject::tr("Minimum of Packed Signed Word Integers."));
        Instructions.insert("vpminub",QObject::tr("Minimum of Packed Unsigned Byte Integers."));
        Instructions.insert("vpminud",QObject::tr("Minimum of Packed Unsigned Integers."));
        Instructions.insert("vpminuw",QObject::tr("Minimum of Packed Word Integers."));
        Instructions.insert("vpmovmskb",QObject::tr("Move Byte Mask."));
        Instructions.insert("vpmovsxbd",QObject::tr("Packed Move byte to double word with Sign Extend."));
        Instructions.insert("vpmovsxbq",QObject::tr("Packed Move byte to quad word with Sign Extend."));
        Instructions.insert("vpmovsxbw",QObject::tr("Packed Move byte to word with Sign Extend."));
        Instructions.insert("vpmovsxdq",QObject::tr("Packed Move double word to quad word with Sign Extend."));
        Instructions.insert("vpmovsxwd)",QObject::tr("Packed Move word to double word with Sign Extend."));
        Instructions.insert("vpmovsxwq)",QObject::tr("Packed Move word to quad word with Sign Extend."));
        Instructions.insert("vpmovzxbd",QObject::tr("Packed Move byte to double word with Zero Extend."));
        Instructions.insert("vpmovzxbq",QObject::tr("Packed Move byte to quad word with Zero Extend."));
        Instructions.insert("vpmovzxbw",QObject::tr("Packed Move byte to word with Zero Extend."));
        Instructions.insert("vpmovzxdq",QObject::tr("Packed Move double word to quad word with Zero Extend."));
        Instructions.insert("vpmovzxwd)",QObject::tr("Packed Move word to double word with Zero Extend."));
        Instructions.insert("vpmovzxwq)",QObject::tr("Packed Move word to quad word with Zero Extend."));
        Instructions.insert("vpmuldq",QObject::tr("Multiply Packed Doubleword Integers."));
        Instructions.insert("vpmulhrsw",QObject::tr("Packed Multiply High with Round and Scale."));
        Instructions.insert("vpmulhuw",QObject::tr("Multiply Packed Unsigned Integers and Store High Result."));
        Instructions.insert("vpmulhw",QObject::tr("Multiply Packed Signed Integers and Store High Result."));
        Instructions.insert("vpmulld",QObject::tr("Multiply Packed Integers and Store Low Result."));
        Instructions.insert("vpmullw",QObject::tr("Multiply Packed Signed Integers and Store Low Result."));
        Instructions.insert("vpmuludq",QObject::tr("Multiply Packed Unsigned Doubleword Integers."));
        Instructions.insert("vpor",QObject::tr("Bitwise Logical Or."));
        Instructions.insert("vpsadbw",QObject::tr("Compute Sum of Absolute Differences."));
        Instructions.insert("vpshufb",QObject::tr("Packed Shuffle Bytes."));
        Instructions.insert("vpshufd",QObject::tr("Shuffle Packed Doublewords."));
        Instructions.insert("vpshufhw",QObject::tr("Shuffle Packed High Words."));
        Instructions.insert("vpshuflw",QObject::tr("Shuffle Packed Low Words."));
        Instructions.insert("vpsignw",QObject::tr("Packed SIGN Word."));
        Instructions.insert("vpsignb",QObject::tr("Packed SIGN Byte."));
        Instructions.insert("vpsignd",QObject::tr("Packed SIGN Doubleword."));

        Instructions.insert("vpslldq",QObject::tr("Shift Double Quadword Left Logical."));
        Instructions.insert("vpsllq",QObject::tr("Bit Shift Left Quadword."));
        Instructions.insert("vpsllw",QObject::tr("Bit Shift Left Word."));
        Instructions.insert("vpslld",QObject::tr("Bit Shift Left Doubleword."));

        Instructions.insert("vpsraw",QObject::tr("Bit Shift Arithmetic Right Word."));
        Instructions.insert("vpsrad",QObject::tr("Bit Shift Arithmetic Right Doubleword."));
        Instructions.insert("vpsrldq",QObject::tr("Shift Double Quadword Right Logical."));
        Instructions.insert("vpsrlq",QObject::tr("Shift Packed Data Right Logical Quadword."));
        Instructions.insert("vpsrlw",QObject::tr("Shift Packed Data Right Logical Word."));
        Instructions.insert("vpsrld",QObject::tr("Shift Packed Data Right Logical Doubleword."));

        Instructions.insert("vpsubq",QObject::tr("Packed Quadword Integer Subtract."));
        Instructions.insert("vpsubw",QObject::tr("Packed Word Integer Subtract."));
        Instructions.insert("vpsubb",QObject::tr("Packed Byte Integer Subtract."));
        Instructions.insert("vpsubd",QObject::tr("Packed Doubleword Integer Subtract."));

        Instructions.insert("vpsubsw",QObject::tr("Subtract Packed Signed Word Integers with Signed Saturation."));
        Instructions.insert("vpsubsb",QObject::tr("Subtract Packed Signed Byte Integers with Signed Saturation."));

        Instructions.insert("vpsubusw",QObject::tr("Subtract Packed Unsigned Word Integers with Unsigned Saturation."));
        Instructions.insert("vpsubusb",QObject::tr("Subtract Packed Unsigned Byte Integers with Unsigned Saturation."));

        Instructions.insert("vptest",QObject::tr("Logical Compare."));
        Instructions.insert("vpunpckhbw",QObject::tr("Unpack High Data."));
        Instructions.insert("vpunpckhdq",QObject::tr("Unpack High Data."));
        Instructions.insert("vpunpckhqdq",QObject::tr("Unpack High Data."));
        Instructions.insert("vpunpckhwd",QObject::tr("Unpack High Data."));
        Instructions.insert("vpunpcklbw",QObject::tr("Unpack Low Data."));
        Instructions.insert("vpunpckldq",QObject::tr("Unpack Low Data."));
        Instructions.insert("vpunpcklqdq",QObject::tr("Unpack Low Data."));
        Instructions.insert("vpunpcklwd",QObject::tr("Unpack Low Data."));

        Instructions.insert("vpxor",QObject::tr("Exclusive Or."));

        Instructions.insert("vrcpps",QObject::tr("Compute Reciprocals of Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vrcpss",QObject::tr("Compute Reciprocal of Scalar Single-Precision Floating-Point Values."));
        Instructions.insert("vroundpd",QObject::tr("Round Packed Double Precision Floating-Point Values."));
        Instructions.insert("vroundps",QObject::tr("Round Packed Single Precision Floating-Point Values."));
        Instructions.insert("vroundsd",QObject::tr("Round Scalar Double Precision Floating-Point Values."));
        Instructions.insert("vroundss",QObject::tr("Round Scalar Single Precision Floating-Point Values."));
        Instructions.insert("vrsqrtps",QObject::tr("Compute Reciprocals of Square Roots of Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vrsqrtss",QObject::tr("Compute Reciprocal of Square Root of Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vshufpd",QObject::tr("Shuffle Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vshufps",QObject::tr("Shuffle Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vsqrtpd",QObject::tr("Square Root of Double-Precision Floating-Point Values."));
        Instructions.insert("vsqrtps",QObject::tr("Square Root of Single-Precision Floating-Point Values."));
        Instructions.insert("vsqrtsd",QObject::tr("Compute Square Root of Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vsqrtss",QObject::tr("Compute Square Root of Scalar Single-Precision Value."));
        Instructions.insert("vstmxcsr",QObject::tr("Store MXCSR Register State."));
        Instructions.insert("vsubpd",QObject::tr("Subtract Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vsubps",QObject::tr("Subtract Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vsubsd",QObject::tr("Subtract Scalar Double-Precision Floating-Point Value."));
        Instructions.insert("vsubss",QObject::tr("Subtract Scalar Single-Precision Floating-Point Value."));
        Instructions.insert("vucomisd",QObject::tr("Unordered Compare Scalar Double-Precision Floating-Point Values and Set EFLAGS."));
        Instructions.insert("vucomiss",QObject::tr("Unordered Compare Scalar Single-Precision Floating-Point Values and Set EFLAGS."));
        Instructions.insert("vunpckhpd",QObject::tr("Unpack and Interleave High Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vunpckhps",QObject::tr("Unpack and Interleave High Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vunpcklpd",QObject::tr("Unpack and Interleave Low Packed Double-Precision Floating-Point Values."));
        Instructions.insert("vunpcklps",QObject::tr("Unpack and Interleave Low Packed Single-Precision Floating-Point Values."));
        Instructions.insert("vbroadcastf128",QObject::tr("Load with Broadcast Floating-Point Data."));
        Instructions.insert("vbroadcastsd",QObject::tr("Load with Broadcast Double-Precision Floating-Point Data."));
        Instructions.insert("vbroadcastss",QObject::tr("Load with Broadcast Single-Precision Floating-Point Data."));
        Instructions.insert("vextractf128",QObject::tr("Extract Packed Floating-Point Values."));
        Instructions.insert("vinsertf128",QObject::tr("Insert Packed Floating-Point Values."));
        Instructions.insert("vmaskmovpd",QObject::tr("Conditional SIMD Packed Double-Precision Loads and Stores."));
        Instructions.insert("vmaskmovps",QObject::tr("Conditional SIMD Packed Single-Precision Loads and Stores."));
        Instructions.insert("vperm2f128",QObject::tr("Permute Floating-Point Values."));
        Instructions.insert("vpermilpd",QObject::tr("Permute Double-Precision Floating-Point Values."));
        Instructions.insert("vpermilps",QObject::tr("Permute Single-Precision Floating-Point Values."));
        Instructions.insert("vtestpd",QObject::tr("Packed Double-Precision Bit Test."));
        Instructions.insert("vtestps",QObject::tr("Packed Single-Precision Bit Test."));
        Instructions.insert("vzeroall",QObject::tr("Zero All YMM Registers."));
        Instructions.insert("vzeroupper",QObject::tr("Zero Upper Bits of YMM Registers."));
        Instructions.insert("vxorpd",QObject::tr("Bitwise Logical XOR for Double-Precision Floating-Point Values."));
        Instructions.insert("vxorps",QObject::tr("Bitwise Logical XOR for Single-Precision Floating-Point Values."));
        Instructions.insert("vpclmulqdq",QObject::tr("Carry-Less Multiplication Quadword, Requires PCLMULQDQ CPUID-flag."));
#endif
        InstructionNames=QSet<QString>(Instructions.keyBegin(),Instructions.keyEnd());
    }
}

bool ASMSyntaxer::eol() const
{
    return mTokenID == TokenId::Null;
}

QString ASMSyntaxer::languageName()
{
    return "asm";
}

ProgrammingLanguage ASMSyntaxer::language()
{
    if (isATT())
        return ProgrammingLanguage::ATTAssembly;
    return ProgrammingLanguage::Assembly;
}

QString ASMSyntaxer::getToken() const
{
    return mLineString.mid(mTokenPos,mRun-mTokenPos);
}

const PTokenAttribute &ASMSyntaxer::getTokenAttribute() const
{
    switch(mTokenID) {
    case TokenId::Comment:
        return mCommentAttribute;
    case TokenId::Identifier:
        return mIdentifierAttribute;
    case TokenId::Instruction:
        return mKeywordAttribute;
    case TokenId::Directive:
        return mDirectiveAttribute;
    case TokenId::Label:
        return mLabelAttribute;
    case TokenId::Register:
        return mRegisterAttribute;
    case TokenId::Number:
        return mNumberAttribute;
    case TokenId::Space:
        return mWhitespaceAttribute;
    case TokenId::String:
        return mStringAttribute;
    case TokenId::Symbol:
        return mSymbolAttribute;
    case TokenId::Unknown:
        return mIdentifierAttribute;
    default:
        return mIdentifierAttribute;
    }
}

int ASMSyntaxer::getTokenPos()
{
    return mTokenPos;
}

void ASMSyntaxer::next()
{
    mTokenPos = mRun;
    switch(mLine[mRun].unicode()) {
    case 0:
        NullProc();
        break;
    case '\n':
        LFProc();
        break;
    case '\r':
        CRProc();
        break;
    case '\"':
        StringProc();
        break;
    case '\'':
        SingleQuoteStringProc();
        break;
    case '>':
        GreaterProc();
        break;
    case '<':
        LowerProc();
        break;
    case '/':
        SlashProc();
        break;
    case ';':
        if (mATT || mCppMixed) {
            SymbolProc();
        } else
            CommentProc();
        break;
    case '#':
        CommentProc();
        break;
    case '.':
        if (isIdentStartChar(mLine[mRun+1])) {
            mRun++;
            IdentProc(IdentPrefix::Period);
        } else
            SymbolProc();
        break;
    case '%':
        if (isIdentStartChar(mLine[mRun+1])) {
            mRun++;
            IdentProc(IdentPrefix::Percent);
        } else
            UnknownProc();
        break;
    case ':':
    case '&':
    case '{':
    case '}':
    case '=':
    case '^':
    case '-':
    case '+':
    case '(':
    case ')':
    case '*':
        SymbolProc();
        break;
    default:
        if (mLine[mRun]>='0' && mLine[mRun]<='9') {
            NumberProc();
        } else if (isIdentStartChar(mLine[mRun])) {
            IdentProc(IdentPrefix::None);
        } else if (isLexicalSpace(mLine[mRun])) {
            SpaceProc();
        } else {
            UnknownProc();
        }
    }
}

void ASMSyntaxer::setLine(const QString &newLine, int lineNumber)
{
    mLineString = newLine;
    mLine = getNullTerminatedStringData(mLineString);
    mLineNumber = lineNumber;
    mRun = 0;
    next();
}

bool ASMSyntaxer::isCommentNotFinished(int /*state*/) const
{
    return false;
}

bool ASMSyntaxer::isStringNotFinished(int /*state*/) const
{
    return false;
}

SyntaxState ASMSyntaxer::getState() const
{
    SyntaxState state;
    state.hasTrailingSpaces = mHasTrailingSpaces;
    return state;
}

void ASMSyntaxer::setState(const SyntaxState&)
{
    mHasTrailingSpaces = false;
}

void ASMSyntaxer::resetState()
{
    mHasTrailingSpaces = false;
}

bool ASMSyntaxer::supportFolding()
{
    return false;
}

bool ASMSyntaxer::needsLineState()
{
    return true;
}

QSet<QString> ASMSyntaxer::keywords()
{
    if (mKeywordsCache.isEmpty()) {
        mKeywordsCache=InstructionNames;
        if (!isATT()) {
            mKeywordsCache.unite(Directives);
            mKeywordsCache.unite(Registers);
        } else {
            mKeywordsCache.unite(ATTDirectives);
            mKeywordsCache.unite(ATTRegisters);
        }
    }
    return mKeywordsCache;
}

const PTokenAttribute &ASMSyntaxer::directiveAttribute() const
{
    return mDirectiveAttribute;
}

const PTokenAttribute &ASMSyntaxer::labelAttribute() const
{
    return mLabelAttribute;
}

QString ASMSyntaxer::commentSymbol()
{
    if (mATT)
        return "#";
    else
        return ";";
}


QString ASMSyntaxer::blockCommentBeginSymbol()
{
    if (mATT)
        return "/*";
    else
        return "";
}

QString ASMSyntaxer::blockCommentEndSymbol()
{
    if (mATT)
        return "*/";
    else
        return "";
}

}
