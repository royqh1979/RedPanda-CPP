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
#include "../Constants.h"
#include <QDebug>

namespace  QSynedit {

const QSet<QString> ASMHighlighter::Registers {
    "ah","al","ax","eax",
    "bh","bl","bx","ebx",
    "ch","cl","cx","ecx",
    "dh","dl","dx","edx",
    "rax","rbx","rcx","rdx","rsi","rdi","rbp",
    "rsp","r8","r9","r10","r11","r12","r13","r14","r15",
    "r8h","r8l","r8w","r8d",
    "r9h","r9l","r9w","r9d",
    "r10h","r10l","r10w","r10d",
    "r11h","r11l","r11w","r11d",
    "r12h","r12l","r12w","r12d",
    "r13h","r13l","r13w","r13d",
    "r14h","r14l","r14w","r14d",
    "r15h","r15l","r15w","r15d"
};

const QSet<QString> ASMHighlighter::Keywords {
    "movb","movw","movl","movq",
    "leab","leaw","leal","leaq",
    "incb","incw","incl","incq",
    "decb","decw","decl","decq",
    "addb","addw","addl","addq",
    "subb","subw","subl","subq",
    "imulb","imulw","imull","imulq",
    "divb","divw","divl","divq",
    "xorb","xorw","xorl","xorq",
    "orb","orw","orl","orq",
    "andb","andw","andl","andq",
    "salb","salw","sall","salq",
    "shlb","shlw","shll","shlq",
    "sarb","sarw","sarl","sarq",
    "shrb","shrw","shrl","shrq",
    "cmpb","cmpw","cmpl","cmpq",
    "testb","testw","testl","testq",
    "pushq","popq",
    "cmove", "cmovz", "cmovne", "cmovnz",
    "cmovs", "cmovns", "cmovg", "cmovge",
    "cmovl", "cmovle", "cmova", "cmovae",
    "cmovb", "cmovbe", "cmovnbe","cmovnb",
    "cmovnae","cmovna",
    "aaa","aad","aam","adc","add","and","arpl","bound","bsf","bsr","bswap","bt","btc","btr","bts",
    "call","cbw","cdq","clc","cld","cli","clts","cmc","cmp","cmps","cmpsb","cmpsd","cmpsw",
    "cmpxchg","cwd","cwde","daa","das","dec","div","emms","enter","f2xm1","fabs","fadd","faddp","fbld",
    "fbstp","fchs","fclex","fcmovb","fcmovbe","fcmove","fcmovnb","fcmovnbe","fcmovne","fcmovnu",
    "fcmovu","fcom","fcomi","fcomip","fcomp","fcompp","fcos","fdecstp","fdiv","fdivp","fdivr",
    "fdivrp","femms","ffree","fiadd","ficom","ficomp","fidiv","fidivr","fild","fimul","fincstp",
    "finit","fist","fistp","fisub","fisubr","fld","fld1","fldcw","fldenv","fldl2e","fldl2t","fldlg2",
    "fldln2","fldpi","fldz","fmul","fmulp","fnclex","fninit","fnop","fnsave","fnstcw","fnstenv",
    "fnstsw","fpatan","fprem1","fptan","frndint","frstor","fsave","fscale","fsin","fsincos",
    "fsqrt","fst","fstcw","fstenv","fstp","fstsw","fsub","fsubp","fsubr","fsubrp","ftst",
    "fucom","fucomi","fucomip","fucomp","fucompp","fwait","fxch","fxtract","fyl2xp1","hlt","idiv",
    "imul","in","inc","ins","insb","insd","insw","int","into","invd","invlpg","iret","iretd","iretw",
    "ja","jae","jb","jbe","jc","jcxz","je","jecxz","jg","jge","jl","jle","jmp","jna","jnae","jnb","jnbe","jnc",
    "jne","jng","jnge","jnl","jnle","jno","jnp","jns","jnz","jo","jp","jpe","jpo","js","jz","lahf","lar","lds",
    "lea","leave","les","lfs","lgdt","lgs","lidt","lldt","lmsw","lock","lods","lodsb","lodsd","lodsw",
    "loop","loope","loopne","loopnz","loopz","lsl","lss","ltr","mov","movd","movq"," movs","movsb",
    "movsd","movsw","movsx","movzx","mul","neg","nop","not","or","out","outs","outsb","outsd","outsw",
    "packssdw","packsswb","packuswb","paddb","paddd","paddsb","paddsw","paddusb","paddusw",
    "paddw","pand","pandn","pavgusb","pcmpeqb","pcmpeqd","pcmpeqw","pcmpgtb","pcmpgtd","pcmpgtw",
    "pf2id","pfacc","pfadd","pfcmpeq","pfcmpge","pfcmpgt","pfmax","pfmin","pfmul","pfrcp",
    "pfrcpit1","pfrcpit2","pfrsqit1","pfrsqrt","pfsub","pfsubr","pi2fd","pmaddwd","pmulhrw",
    "pmulhw","pmullw","pop","popa","popad","popaw","popf","popfd","popfw","por","prefetch","prefetchw",
    "pslld","psllq","psllw","psrad","psraw","psrld","psrlq","psrlw","psubb","psubd","psubsb",
    "psubsw","psubusb","psubusw","psubw","punpckhbw","punpckhdq","punpckhwd","punpcklbw",
    "punpckldq","punpcklwd","push","pusha","pushad","pushaw","pushf","pushfd","pushfw","pxor",
    "rcl","rcr","rep","repe","repne","repnz","repz","ret","rol","ror","sahf","sal","sar","sbb","scas",
    "scasb","scasd","scasw","seta","setae","setb","setbe","setc","sete","setg","setge","setl","setle",
    "setna","setnae","setnb","setnbe","setnc","setne","setng","setnge","setnl","setnle","setno",
    "setnp","setns","setnz","seto","setp","setpo","sets","setz","sgdt","shl","shld","shr","shrd","sidt",
    "sldt","smsw","stc","std","sti","stos","stosb","stosd","stosw","str","sub","test","verr","verw",
    "wait","wbinvd","xadd","xchg","xlat","xlatb","xor"
};



ASMHighlighter::ASMHighlighter()
{
    mNumberAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrNumber, TokenType::Number);
    addAttribute(mNumberAttribute);
    mDirectiveAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrVariable, TokenType::Keyword);
    addAttribute(mDirectiveAttribute);
    mLabelAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrFunction, TokenType::Keyword);
    addAttribute(mLabelAttribute);
    mRegisterAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrClass, TokenType::Keyword);
    addAttribute(mRegisterAttribute);
}

const PHighlighterAttribute &ASMHighlighter::numberAttribute() const
{
    return mNumberAttribute;
}

const PHighlighterAttribute &ASMHighlighter::registerAttribute() const
{
    return mRegisterAttribute;
}

void ASMHighlighter::CommentProc()
{
    mTokenID = TokenId::Comment;
    do {
        mRun++;
    } while (! (mLine[mRun]==0 || mLine[mRun] == '\r' || mLine[mRun]=='\n'));
}

void ASMHighlighter::CRProc()
{
    mTokenID = TokenId::Space;
    mRun++;
    if (mLine[mRun] == '\n')
        mRun++;
}

void ASMHighlighter::GreaterProc()
{
    mRun++;
    mTokenID = TokenId::Symbol;
    if (mLine[mRun] == '=')
        mRun++;
}

void ASMHighlighter::IdentProc(IdentPrefix prefix)
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
        if (Keywords.contains(s))
            mTokenID = TokenId::Instruction;
        else if (Registers.contains(s))
            mTokenID = TokenId::Register;
        else if (mLine[mRun]==':')
            mTokenID = TokenId::Label;
        else
            mTokenID = TokenId::Identifier;
    }
}

void ASMHighlighter::LFProc()
{
    mTokenID = TokenId::Space;
    mRun++;
}

void ASMHighlighter::LowerProc()
{
    mRun++;
    mTokenID = TokenId::Symbol;
    if (mLine[mRun]=='=' || mLine[mRun]== '>')
        mRun++;
}

void ASMHighlighter::NullProc()
{
    mTokenID = TokenId::Null;
}

void ASMHighlighter::NumberProc()
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

void ASMHighlighter::SingleQuoteStringProc()
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

void ASMHighlighter::SlashProc()
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

void ASMHighlighter::SpaceProc()
{
    mTokenID = TokenId::Space;
    while (true) {
        mRun++;
        if (mLine[mRun] == 0 || mLine[mRun] == '\r' || mLine[mRun] == '\n')
            break;
        if (mLine[mRun] > 32)
            break;
    }
}

void ASMHighlighter::StringProc()
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

void ASMHighlighter::SymbolProc()
{
    mRun++;
    mTokenID = TokenId::Symbol;
}

void ASMHighlighter::UnknownProc()
{
    mRun++;
    mTokenID = TokenId::Unknown;
}

bool ASMHighlighter::isIdentStartChar(const QChar &ch)
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

bool ASMHighlighter::eol() const
{
    return mTokenID == TokenId::Null;
}

QString ASMHighlighter::languageName()
{
    return "asm";
}

HighlighterLanguage ASMHighlighter::language()
{
    return HighlighterLanguage::Asssembly;
}

QString ASMHighlighter::getToken() const
{
    return mLineString.mid(mTokenPos,mRun-mTokenPos);
}

const PHighlighterAttribute &ASMHighlighter::getTokenAttribute() const
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

int ASMHighlighter::getTokenPos()
{
    return mTokenPos;
}

void ASMHighlighter::next()
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
    case '#':
    case ';':
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

void ASMHighlighter::setLine(const QString &newLine, int lineNumber)
{
    mLineString = newLine;
    mLine = mLineString.data();
    mLineNumber = lineNumber;
    mRun = 0;
    next();
}

bool ASMHighlighter::getTokenFinished() const
{
    return true;
}

bool ASMHighlighter::isLastLineCommentNotFinished(int /*state*/) const
{
    return true;
}

bool ASMHighlighter::isLastLineStringNotFinished(int /*state*/) const
{
    return true;
}

HighlighterState ASMHighlighter::getState() const
{
    return HighlighterState();
}

void ASMHighlighter::setState(const HighlighterState&)
{

}

void ASMHighlighter::resetState()
{

}

QSet<QString> ASMHighlighter::keywords() const
{
    return Keywords;
}

const PHighlighterAttribute &ASMHighlighter::directiveAttribute() const
{
    return mDirectiveAttribute;
}

const PHighlighterAttribute &ASMHighlighter::labelAttribute() const
{
    return mLabelAttribute;
}
}
