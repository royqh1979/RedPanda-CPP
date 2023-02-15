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

const QSet<QString> ASMSyntaxer::Instructions {
    "aaa","aad","aam","aas","adc","adcx","add",
    "addb","addw","addl","addq", "addpd","addps",
    "addsd","addss","addsubpd","addsubps","adox","aesdec","aesdec128kl","aesdec256kl","aesdeclast","aesdecwide128kl",
    "aesdecwide256kl","aesenc","aesenc128kl","aesenc256kl","aesenclast","aesencwide128kl","aesencwide256kl","aesimc","aeskeygenassist","and",
    "andn","andnpd","andnps","andpd","andps","andb","andw","andl","andq","arpl","bextr","blendpd","blendps","blendvpd",
    "blendvps","blsi","blsmsk","blsr","bndcl","bndcn","bndcu","bndldx","bndmk","bndmov",
    "bndstx","bound","bsf","bsr","bswap","bt","btc","btr","bts","bzhi",
    "call","cbw","cdq","cdqe","clac","clc","cld","cldemote","clflush","clflushopt",
    "cli","clrssbsy","clts","clwb","cmc","cmova","cmovae","cmovb","cmovbe","cmovc",
    "cmove","cmovg","cmovge","cmovl","cmovle","cmovna","cmovnae","cmovnb","cmovnbe","cmovnc",
    "cmovne","cmovng","cmovnge","cmovnl","cmovnle","cmovno","cmovnp","cmovns","cmovnz","cmovo",
    "cmovp","cmovpe","cmovpo","cmovs","cmovz","cmp","cmpb","cmpw","cmpl","cmpq",
    "cmppd","cmpps","cmps","cmpsb", "cmpsd","cmpsq","cmpss","cmpsw","cmpxchg","cmpxchg16b","cmpxchg8b","comisd","comiss","cpuid",
    "cqo","crc32","cvtdq2pd","cvtdq2ps","cvtpd2dq","cvtpd2pi","cvtpd2ps","cvtpi2pd","cvtpi2ps","cvtps2dq",
    "cvtps2pd","cvtps2pi","cvtsd2si","cvtsd2ss","cvtsi2sd","cvtsi2ss","cvtss2sd","cvtss2si","cvttpd2dq","cvttpd2pi",
    "cvttps2dq","cvttps2pi","cvttsd2si","cvttss2si","cwd","cwde","daa","das","dec","div",
    "divpd","divps","divsd","divss","dppd","dpps","emms","encodekey128","encodekey256","endbr32",
    "endbr64","enter","extractps","f2xm1","fabs","fadd","faddp","fbld","fbstp","fchs",
    "fclex","fcmovb","fcmove","fcmovbe","fcmovu","fcmovnb","fcmovne","fcmovnbe","fcmovnu","fcom",
    "fcomi","fcomip","fcomp","fcompp","fcos","fdecstp","fdiv","fdivp","fdivr","fdivrp",
    "ffree","fiadd","ficom","ficomp","fidiv","fidivr","fild","fimul","fincstp","finit",
    "fist","fistp","fisttp","fisub","fisubr","fld","fld1","fldcw","fldenv","fldl2e",
    "fldl2t","fldlg2","fldln2","fldpi","fldz","fmul","fmulp","fnclex","fninit","fnop",
    "fnsave","fnstcw","fnstenv","fnstsw","fpatan","fprem","fprem1","fptan","frndint","frstor",
    "fsave","fscale","fsin","fsincos","fsqrt","fst","fstcw","fstenv","fstp","fstsw",
    "fsub","fsubp","fsubr","fsubrp","ftst","fucom","fucomi","fucomip","fucomp","fucompp",
    "fwait","fxam","fxch","fxrstor","fxsave","fxtract","fyl2x","fyl2xp1","gf2p8affineinvqb","gf2p8affineqb",
    "gf2p8mulb","haddpd","haddps","hlt","hreset","hsubpd","hsubps","idiv","idivb","idivw","idivl","idivq","imul",
    "imulb","imulw","imull","imulq","in",  "inc","incsspd","incsspq","ins","insb","insd","insertps","insw","int n","int1",
    "int3","into","invd","invlpg","invpcid","iret","iretd","iretq","jmp","ja",
    "jae","jb","jbe","jc","jcxz","je","jecxz","jg","jge","jl",
    "jle","jna","jnae","jnb","jnbe","jnc","jne","jng","jnge","jnl",
    "jnle","jno","jnp","jns","jnz","jo","jp","jpe","jpo","jrcxz",
    "js","jz","kaddb","kaddd","kaddq","kaddw","kandb","kandd","kandnb","kandnd",
    "kandnq","kandnw","kandq","kandw","kmovb","kmovd","kmovq","kmovw","knotb","knotd",
    "knotq","knotw","korb","kord","korq","kortestb","kortestd","kortestq","kortestw","korw",
    "kshiftlb","kshiftld","kshiftlq","kshiftlw","kshiftrb","kshiftrd","kshiftrq","kshiftrw","ktestb","ktestd",
    "ktestq","ktestw","kunpckbw","kunpckdq","kunpckwd","kxnorb","kxnord","kxnorq","kxnorw","kxorb",
    "kxord","kxorq","kxorw","lahf","lar","lddqu","ldmxcsr","lds","lea","leaq","leave",
    "les","lfence","lfs","lgdt","lgs","lidt","lldt","lmsw","loadiwkey","lock",
    "lods","lodsb","lodsd","lodsq","lodsw","loop","loope","loopne","lsl","lss",
    "ltr","lzcnt","maskmovdqu","maskmovq","maxpd","maxps","maxsd","maxss","mfence","minpd",
    "minps","minsd","minss","monitor","mov","movapd","movaps","movbe","movd","movddup",
    "movdir64b","movdiri","movdq2q","movdqa","movdqu","movhlps","movhpd","movhps","movlhps","movlpd",
    "movlps","movmskpd","movmskps","movntdq","movntdqa","movnti","movntpd","movntps","movntq","movq",
    "movq2dq","movs","movsb","movsd","movshdup","movsldup","movsq","movss","movsw","movsx",
    "movsxd","movupd","movups","movzx", "movb","movs","movl",
    "mpsadbw","mul","mulpd","mulps","mulsd","mulss",
    "mulx","mwait","neg","nop","not","or","orb","orw","orl","orq","orpd","orps","out","outs",
    "outsb","outsd","outsw","pabsb","pabsd","pabsq","pabsw","packssdw","packsswb","packusdw",
    "packuswb","paddb","paddd","paddq","paddsb","paddsw","paddusb","paddusw","paddw","palignr",
    "pand","pandn","pause","pavgb","pavgw","pblendvb","pblendw","pclmulqdq","pcmpeqb","pcmpeqd",
    "pcmpeqq","pcmpeqw","pcmpestri","pcmpestrm","pcmpgtb","pcmpgtd","pcmpgtq","pcmpgtw","pcmpistri","pcmpistrm",
    "pconfig","pdep","pext","pextrb","pextrd","pextrq","pextrw","phaddd","phaddsw","phaddw",
    "phminposuw","phsubd","phsubsw","phsubw","pinsrb","pinsrd","pinsrq","pinsrw","pmaddubsw","pmaddwd",
    "pmaxsb","pmaxsd","pmaxsq","pmaxsw","pmaxub","pmaxud","pmaxuq","pmaxuw","pminsb","pminsd",
    "pminsq","pminsw","pminub","pminud","pminuq","pminuw","pmovmskb","pmovsx","pmovzx","pmuldq",
    "pmulhrsw","pmulhuw","pmulhw","pmulld","pmullq","pmullw","pmuludq","pop","popa","popad",
    "popcnt","popf","popfd","popfq","popq","por","prefetchw","prefetchh","psadbw","pshufb","pshufd",
    "pshufhw","pshuflw","pshufw","psignb","psignd","psignw","pslld","pslldq","psllq","psllw",
    "psrad","psraq","psraw","psrld","psrldq","psrlq","psrlw","psubb","psubd","psubq",
    "psubsb","psubsw","psubusb","psubusw","psubw","ptest","ptwrite","punpckhbw","punpckhdq","punpckhqdq",
    "punpckhwd","punpcklbw","punpckldq","punpcklqdq","punpcklwd","push","pusha","pushad","pushf","pushfd",
    "pushfq","pushq","pxor","rcl","rcpps","rcpss","rcr","rdfsbase","rdgsbase","rdmsr","rdpid",
    "rdpkru","rdpmc","rdrand","rdseed","rdsspd","rdsspq","rdtsc","rdtscp","rep","repe",
    "repne","repnz","repz","ret","rol","ror","rorx","roundpd","roundps","roundsd",
    "roundss","rsm","rsqrtps","rsqrtss","rstorssp","sahf","sal","salb","salw","sall","salq","sar","sarb","sarw","sarl","sarq","sarx","saveprevssp",
    "sbb","scas","scasb","scasd","scasw","serialize","setssbsy","seta","setae","setb",
    "setbe","setc","sete","setg","setge","setl","setle","setna","setnae","setnb",
    "setnbe","setnc","setne","setng","setnge","setnl","setnle","setno","setnp","setns",
    "setnz","seto","setp","setpe","setpo","sets","setz","sfence","sgdt","sha1msg1",
    "sha1msg2","sha1nexte","sha1rnds4","sha256msg1","sha256msg2","sha256rnds2","shl","shld","shlx","shr",
    "shrd","shrx","shufpd","shufps","sidt","sldt","smsw","sqrtpd","sqrtps","sqrtsd",
    "sqrtss","stac","stc","std","sti","stmxcsr","stos","stosb","stosd","stosq",
    "stosw","str","sub","subpd","subps","subsd","subss","swapgs","syscall","sysenter",
    "sysexit","sysret","test","tpause","tzcnt","ucomisd","ucomiss","ud","umonitor","umwait",
    "unpckhpd","unpckhps","unpcklpd","unpcklps","valignd","valignq","vblendmpd","vblendmps","vbroadcast","vcompresspd",
    "vcompressps","vcompressw","vcvtne2ps2bf16","vcvtneps2bf16","vcvtpd2qq","vcvtpd2udq","vcvtpd2uqq","vcvtph2ps","vcvtps2ph","vcvtps2qq",
    "vcvtps2udq","vcvtps2uqq","vcvtqq2pd","vcvtqq2ps","vcvtsd2usi","vcvtss2usi","vcvttpd2qq","vcvttpd2udq","vcvttpd2uqq","vcvttps2qq",
    "vcvttps2udq","vcvttps2uqq","vcvttsd2usi","vcvttss2usi","vcvtudq2pd","vcvtudq2ps","vcvtuqq2pd","vcvtuqq2ps","vcvtusi2sd","vcvtusi2ss",
    "vdbpsadbw","vdpbf16ps","verr","verw","vexpandpd","vexpandps","vextractf128","vextractf32x4","vextractf32x8","vextractf64x2",
    "vextractf64x4","vextracti128","vextracti32x4","vextracti32x8","vextracti64x2","vextracti64x4","vfixupimmpd","vfixupimmps","vfixupimmsd","vfixupimmss",
    "vfmadd132pd","vfmadd132ps","vfmadd132sd","vfmadd132ss","vfmadd213pd","vfmadd213ps","vfmadd213sd","vfmadd213ss","vfmadd231pd","vfmadd231ps",
    "vfmadd231sd","vfmadd231ss","vfmaddsub132pd","vfmaddsub132ps","vfmaddsub213pd","vfmaddsub213ps","vfmaddsub231pd","vfmaddsub231ps","vfmsub132pd","vfmsub132ps",
    "vfmsub132sd","vfmsub132ss","vfmsub213pd","vfmsub213ps","vfmsub213sd","vfmsub213ss","vfmsub231pd","vfmsub231ps","vfmsub231sd","vfmsub231ss",
    "vfmsubadd132pd","vfmsubadd132ps","vfmsubadd213pd","vfmsubadd213ps","vfmsubadd231pd","vfmsubadd231ps","vfnmadd132pd","vfnmadd132ps","vfnmadd132sd","vfnmadd132ss",
    "vfnmadd213pd","vfnmadd213ps","vfnmadd213sd","vfnmadd213ss","vfnmadd231pd","vfnmadd231ps","vfnmadd231sd","vfnmadd231ss","vfnmsub132pd","vfnmsub132ps",
    "vfnmsub132sd","vfnmsub132ss","vfnmsub213pd","vfnmsub213ps","vfnmsub213sd","vfnmsub213ss","vfnmsub231pd","vfnmsub231ps","vfnmsub231sd","vfnmsub231ss",
    "vfpclasspd","vfpclassps","vfpclasssd","vfpclassss","vgatherdpd","vgatherdps","vgatherqpd","vgatherqps","vgetexppd","vgetexpps",
    "vgetexpsd","vgetexpss","vgetmantpd","vgetmantps","vgetmantsd","vgetmantss","vinsertf128","vinsertf32x4","vinsertf32x8","vinsertf64x2",
    "vinsertf64x4","vinserti128","vinserti32x4","vinserti32x8","vinserti64x2","vinserti64x4","vmaskmov","vmovdqa32","vmovdqa64","vmovdqu16",
    "vmovdqu32","vmovdqu64","vmovdqu8","vp2intersectd","vp2intersectq","vpblendd","vpblendmb","vpblendmd","vpblendmq","vpblendmw",
    "vpbroadcast","vpbroadcastb","vpbroadcastd","vpbroadcastm","vpbroadcastq","vpbroadcastw","vpcmpb","vpcmpd","vpcmpq","vpcmpub",
    "vpcmpud","vpcmpuq","vpcmpuw","vpcmpw","vpcompressb","vpcompressd","vpcompressq","vpconflictd","vpconflictq","vpdpbusd",
    "vpdpbusds","vpdpwssd","vpdpwssds","vperm2f128","vperm2i128","vpermb","vpermd","vpermi2b","vpermi2d","vpermi2pd",
    "vpermi2ps","vpermi2q","vpermi2w","vpermilpd","vpermilps","vpermpd","vpermps","vpermq","vpermt2b","vpermt2d",
    "vpermt2pd","vpermt2ps","vpermt2q","vpermt2w","vpermw","vpexpandb","vpexpandd","vpexpandq","vpexpandw","vpgatherdd",
    "vpgatherdq","vpgatherqd","vpgatherqq","vplzcntd","vplzcntq","vpmadd52huq","vpmadd52luq","vpmaskmov","vpmovb2m","vpmovd2m",
    "vpmovdb","vpmovdw","vpmovm2b","vpmovm2d","vpmovm2q","vpmovm2w","vpmovq2m","vpmovqb","vpmovqd","vpmovqw",
    "vpmovsdb","vpmovsdw","vpmovsqb","vpmovsqd","vpmovsqw","vpmovswb","vpmovusdb","vpmovusdw","vpmovusqb","vpmovusqd",
    "vpmovusqw","vpmovuswb","vpmovw2m","vpmovwb","vpmultishiftqb","vpopcnt","vprold","vprolq","vprolvd","vprolvq",
    "vprord","vprorq","vprorvd","vprorvq","vpscatterdd","vpscatterdq","vpscatterqd","vpscatterqq","vpshld","vpshldv",
    "vpshrd","vpshrdv","vpshufbitqmb","vpsllvd","vpsllvq","vpsllvw","vpsravd","vpsravq","vpsravw","vpsrlvd",
    "vpsrlvq","vpsrlvw","vpternlogd","vpternlogq","vptestmb","vptestmd","vptestmq","vptestmw","vptestnmb","vptestnmd",
    "vptestnmq","vptestnmw","vrangepd","vrangeps","vrangesd","vrangess","vrcp14pd","vrcp14ps","vrcp14sd","vrcp14ss",
    "vreducepd","vreduceps","vreducesd","vreducess","vrndscalepd","vrndscaleps","vrndscalesd","vrndscaless","vrsqrt14pd","vrsqrt14ps",
    "vrsqrt14sd","vrsqrt14ss","vscalefpd","vscalefps","vscalefsd","vscalefss","vscatterdpd","vscatterdps","vscatterqpd","vscatterqps",
    "vshuff32x4","vshuff64x2","vshufi32x4","vshufi64x2","vtestpd","vtestps","vzeroall","vzeroupper","wait","wbinvd",
    "wbnoinvd","wrfsbase","wrgsbase","wrmsr","wrpkru","wrssd","wrssq","wrussd","wrussq","xabort",
    "xacquire","xadd","xbegin","xchg","xend","xgetbv","xlat","xlatb","xor","xorb","xorw","xorl","xorq","xorpd",
    "xorps","xrelease","xrstor","xrstors","xsave","xsavec","xsaveopt","xsaves","xsetbv","xtest",
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
    mATT = newATT;
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

QSet<QString> ASMSyntaxer::keywords() const
{
    QSet<QString> result=Instructions;
    if (!isATT()) {
        result.unite(Directives);
        result.unite(Registers);
    } else {
        result.unite(ATTDirectives);
        result.unite(ATTRegisters);
    }
    return result;
}

const PTokenAttribute &ASMSyntaxer::directiveAttribute() const
{
    return mDirectiveAttribute;
}

const PTokenAttribute &ASMSyntaxer::labelAttribute() const
{
    return mLabelAttribute;
}
}
