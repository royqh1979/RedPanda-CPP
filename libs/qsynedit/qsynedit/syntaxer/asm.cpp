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
#include <QDebug>

namespace  QSynedit {

QSet<QString> ASMSyntaxer::InstructionNames;
QMap<QString,QString> ASMSyntaxer::Instructions;

const QSet<QString> ASMSyntaxer::Registers {
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
};

const QSet<QString> ASMSyntaxer::ATTRegisters {
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
};

const QSet<QString> ASMSyntaxer::Directives {
    "section","global","extern","segment",
    "db","dw","dd","dq","dt","do","dy","dz",
    "resb","resw","resd","resq","rest","reso","resy","resz",
    "equ","times","word","dword","byte","tword"
};

const QSet<QString> ASMSyntaxer::ATTDirectives {
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
    ".seh_pushreg",".seh_setframe",
    ".seh_stackalloc",".seh_endprologue",
    ".set", ".short", ".single", ".size",
    ".skip", ".sleb128", ".space_size", ".stabd",
    ".stabn", ".stabs", ".string", ".string8", ".string16",
    ".struct", ".subsection", ".symver", ".tag", ".text",
    ".title", ".tls", ".type", ".uleb128", ".val",".version",
    ".vtable", ".warning",".weak",".weakref",".word",
    ".zero",".2byte",".4byte",".8byte"
};

ASMSyntaxer::ASMSyntaxer(bool isATT):
    mATT(isATT)
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
        if (mLine[mRun] > 32)
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

bool ASMSyntaxer::isIdentStartChar(const QChar &ch)
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
        Instructions.insert("bswap",QObject::tr("byte swap."));
        Instructions.insert("bswapl",QObject::tr("byte swap."));
        Instructions.insert("bswapq",QObject::tr("byte swap."));
        Instructions.insert("cbtw",QObject::tr("convert %1 to %2.").arg(QObject::tr("byte"),QObject::tr("word")));
        Instructions.insert("cbw",QObject::tr("convert %1 to %2.").arg(QObject::tr("byte"),QObject::tr("word")));
        Instructions.insert("cltd",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("double word"),"%eax",QObject::tr("quad word"),"%edx:%eax"));
        Instructions.insert("cdq",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("double word"),"%eax",QObject::tr("quad word"),"%edx:%eax"));
        Instructions.insert("cltq",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("double word"),"%eax",QObject::tr("quad word"),"%rax"));
        Instructions.insert("cdqe",QObject::tr("convert %1 in %2 to %3 in %4.").arg(QObject::tr("double word"),"%eax",QObject::tr("quad word"),"%rax"));
        Instructions.insert("cmove",QObject::tr("Conditional move if equal"));
        Instructions.insert("cmovz",QObject::tr("Conditional move if zero."));
        Instructions.insert("cmovne",QObject::tr("Conditional move if not equal."));
        Instructions.insert("cmovnz",QObject::tr("Conditional move if not equal."));
        Instructions.insert("cmova",QObject::tr("Conditional move if above."));
        Instructions.insert("cmovbe",QObject::tr("Conditional move if not below or equal."));
        Instructions.insert("cmovae",QObject::tr("Conditional move if above or equal."));
        Instructions.insert("cmovnb",QObject::tr("Conditional move if not below."));
        Instructions.insert("cmovb",QObject::tr("Conditional move if below."));
        Instructions.insert("cmovnae",QObject::tr("Conditional move if not above or equal."));
        Instructions.insert("cmovbe",QObject::tr("Conditional move if below or equal."));
        Instructions.insert("cmovna",QObject::tr("Conditional move if not above."));
        Instructions.insert("cmovg",QObject::tr("Conditional move if greater."));
        Instructions.insert("cmovnle",QObject::tr("Conditional move if not less or equal."));
        Instructions.insert("cmovge",QObject::tr("Conditional move if greater or equal."));
        Instructions.insert("cmovnl",QObject::tr("Conditional move if not less."));
        Instructions.insert("cmovl",QObject::tr("Conditional move if less."));
        Instructions.insert("cmovnge",QObject::tr("Conditional move if not greater or equal."));
        Instructions.insert("cmovle",QObject::tr("Conditional move if less or equal."));
        Instructions.insert("cmovng",QObject::tr("Conditional move if not greater."));
        Instructions.insert("cmovc",QObject::tr("Conditional move if carry."));
        Instructions.insert("cmovnc",QObject::tr("Conditional move if not carry."));
        Instructions.insert("cmovo",QObject::tr("Conditional move if overflow."));
        Instructions.insert("cmovno",QObject::tr("Conditional move if not overflow."));
        Instructions.insert("cmovs",QObject::tr("Conditional move if sign (negative)."));
        Instructions.insert("cmovns",QObject::tr("Conditional move if not sign (non-negative)."));
        Instructions.insert("cmovp",QObject::tr("Conditional move if parity."));
        Instructions.insert("cmovpe",QObject::tr("Conditional move if parity even."));
        Instructions.insert("cmovnp",QObject::tr("Conditional move if not parity."));
        Instructions.insert("cmovpo",QObject::tr("Conditional move if parity odd."));
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
        Instructions.insert("seta",QObject::tr("set byte if above."));
        Instructions.insert("setae",QObject::tr("set byte if above or equal."));
        Instructions.insert("setb",QObject::tr("set byte if below."));
        Instructions.insert("setbe",QObject::tr("set byte if below or equal."));
        Instructions.insert("setc",QObject::tr("set byte if carry."));
        Instructions.insert("sete",QObject::tr("set byte if equal."));
        Instructions.insert("setg",QObject::tr("set byte if greater."));
        Instructions.insert("setge",QObject::tr("set byte if greater or equal."));
        Instructions.insert("setl",QObject::tr("set byte if less."));
        Instructions.insert("setle",QObject::tr("set byte if less or equal."));
        Instructions.insert("setna",QObject::tr("set byte if not above."));
        Instructions.insert("setnae",QObject::tr("set byte if not above or equal."));
        Instructions.insert("setnb",QObject::tr("set byte if not below."));
        Instructions.insert("setnbe",QObject::tr("set byte if not below or equal."));
        Instructions.insert("setnc",QObject::tr("set byte if not carry."));
        Instructions.insert("setne",QObject::tr("set byte if not equal."));
        Instructions.insert("setng",QObject::tr("set byte if not greater."));
        Instructions.insert("setnge",QObject::tr("set byte if not greater or equal."));
        Instructions.insert("setnl",QObject::tr("set byte if not less."));
        Instructions.insert("setnle",QObject::tr("set byte if not less or equal."));
        Instructions.insert("setno",QObject::tr("set byte if not overflow."));
        Instructions.insert("setnp",QObject::tr("set byte if not parity."));
        Instructions.insert("setns",QObject::tr("set byte if not sign (non-negative)."));
        Instructions.insert("setnz",QObject::tr("set byte if not zero."));
        Instructions.insert("seto",QObject::tr("set byte if overflow."));
        Instructions.insert("setp",QObject::tr("set byte if parity."));
        Instructions.insert("setpe",QObject::tr("set byte if parity even."));
        Instructions.insert("setpo",QObject::tr("set byte if parity odd."));
        Instructions.insert("sets",QObject::tr("set byte if sign (negative)."));
        Instructions.insert("setz",QObject::tr("set byte if zero."));
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
        Instructions.insert("ja",QObject::tr("jump if above."));
        Instructions.insert("jae",QObject::tr("jump if above or equal."));
        Instructions.insert("jb",QObject::tr("jump if below."));
        Instructions.insert("jbe",QObject::tr("jump if below or equal."));
        Instructions.insert("jc",QObject::tr("jump if carry."));
        Instructions.insert("jcxz",QObject::tr("jump register %cx zero"));
        Instructions.insert("je",QObject::tr("jump if equal."));
        Instructions.insert("jecxz",QObject::tr("jump register %ecx zero"));
        Instructions.insert("jg",QObject::tr("jump if greater."));
        Instructions.insert("jge",QObject::tr("jump if greater or equal."));
        Instructions.insert("jl",QObject::tr("jump if less."));
        Instructions.insert("jle",QObject::tr("jump if less or equal."));
        Instructions.insert("jmp",QObject::tr("jump."));
        Instructions.insert("jnae",QObject::tr("jump if not above or equal."));
        Instructions.insert("jnb",QObject::tr("jump if not below."));
        Instructions.insert("jnbe",QObject::tr("jump if not below or equal."));
        Instructions.insert("jnc",QObject::tr("jump if not carry."));
        Instructions.insert("jne",QObject::tr("jump if not equal."));
        Instructions.insert("jng",QObject::tr("jump if not greater."));
        Instructions.insert("jnge",QObject::tr("jump if not greater or equal."));
        Instructions.insert("jnl",QObject::tr("jump if not less."));
        Instructions.insert("jnle",QObject::tr("jump if not less or equal."));
        Instructions.insert("jno",QObject::tr("jump if not overflow."));
        Instructions.insert("jnp",QObject::tr("jump if not parity."));
        Instructions.insert("jns",QObject::tr("jump if not sign (non-negative)."));
        Instructions.insert("jnz",QObject::tr("jump if not zero."));
        Instructions.insert("jo",QObject::tr("jump if overflow."));
        Instructions.insert("jp",QObject::tr("jump if parity."));
        Instructions.insert("jpe",QObject::tr("jump if parity even."));
        Instructions.insert("jpo",QObject::tr("jump if parity odd."));
        Instructions.insert("js",QObject::tr("jump if sign (negative)."));
        Instructions.insert("jz",QObject::tr("jump if zero."));
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
        if (mATT) {
            SymbolProc();
        } else
            CommentProc();
        break;
    case '#':
        CommentProc();
        break;
    case '.':
        if (isIdentChar(mLine[mRun+1])) {
            mRun++;
            IdentProc(IdentPrefix::Period);
        } else
            SymbolProc();
        break;
    case '%':
        if (isIdentChar(mLine[mRun+1])) {
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
        } else if (isIdentChar(mLine[mRun])) {
            IdentProc(IdentPrefix::None);
        } else if (mLine[mRun]<=32) {
            SpaceProc();
        } else {
            UnknownProc();
        }
    }
}

void ASMSyntaxer::setLine(const QString &newLine, int lineNumber)
{
    mLineString = newLine;
    mLine = mLineString.data();
    mLineNumber = lineNumber;
    mRun = 0;
    next();
}

bool ASMSyntaxer::getTokenFinished() const
{
    return true;
}

bool ASMSyntaxer::isLastLineCommentNotFinished(int /*state*/) const
{
    return true;
}

bool ASMSyntaxer::isLastLineStringNotFinished(int /*state*/) const
{
    return true;
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
