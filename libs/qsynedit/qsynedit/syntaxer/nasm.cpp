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
#include "nasm.h"
#include "../constants.h"
#include <qt_utils/utils.h>
#include <QDebug>

namespace  QSynedit {

const QSet<QString> NASMSyntaxer::Directives {
    "section","global","extern","segment",
    "absolute","common","static",
    "prefix","gprefix","lprefix","gpostfix","lpostfix","postfix",
    "db","dw","dd","dq","dt","do","dy","dz",
    "resb","resw","resd","resq","rest","reso","resy","resz",
    "equ","times","incbin","dup",
    "byte","word","dword","qword","tword",
    "xmmword","ymmword","zmmword","fword","tbyte","oword","ptr",
    "strict","seg","wrt",
    "bits","use16","use32",
    "defalut","rel","abs","bnd","nobnd",
    "cpu","float"
};

const QSet<QString> NASMSyntaxer::PreprocessorDirectives {
    "%define","%xdefine","%undef","%assign","%defstr",
    "%deftok","%strcat","%strlen","%substr",
    "%abs","%b2hs","%chr","%cond","%count",
    "%eval","%find","%findi","%hex","%hs2b","%is",
    "%map","%null","%num","%ord","%pathsearch","%realpath",
    "%sel","%str","%strcat","%strlen","%substr",
    "%tok","%macro","%rotate","%unmacro","%unimacro",
    "%exitmacro","%if","%else","%elif","%endif",
    "%ifdef","%ifdefalias","%ifmacro","%ifctx",
    "%ifidn", "%ifidni", "%ifid", "%ifnum", "%ifstr",
    "%iftoken","%ifempty","%ifdirectvie","%ifusable","%ifusing",
    "%iffile","%ifenv","%rep","%include","%depend",
    "%use","%push","%pop","%repl","%arg","%stacksize","%local",
    "%error","%fatal","%warning","%note","%pragma","%line","%clear"
};



NASMSyntaxer::NASMSyntaxer():ASMSyntaxer()
{
}

bool NASMSyntaxer::isDirective(const QString &ident)
{
    return Directives.contains(ident);
}

bool NASMSyntaxer::isPreprocessDirective(const QString &ident)
{
    return PreprocessorDirectives.contains(ident);
}

QString NASMSyntaxer::languageName()
{
    return "NASM";
}

ProgrammingLanguage NASMSyntaxer::language()
{
    return ProgrammingLanguage::NetwideAssembly;
}

QSet<QString> NASMSyntaxer::keywords()
{
    if (mKeywordCache.isEmpty()) {
        mKeywordCache = InstructionNames;
        mKeywordCache += Directives;
        mKeywordCache += Registers;
    }
    return mKeywordCache;
}

}
