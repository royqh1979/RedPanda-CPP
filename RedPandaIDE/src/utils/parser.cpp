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
#include "parser.h"
#include "../parser/cppparser.h"
#include "../settings/compilersetsettings.h"
#include "../settings.h"
#include "../mainwindow.h"

void resetCppParser(std::shared_ptr<CppParser> parser, int compilerSetIndex)
{
    if (!parser)
        return;
    // Configure parser
    parser->resetParser();
    parser->setEnabled(true);
    parser->setParseGlobalHeaders(true);
    parser->setParseLocalHeaders(true);

    // Set options depending on the current compiler set
    if (compilerSetIndex<0) {
        compilerSetIndex=pSettings->compilerSets().defaultIndex();
    }
    PCompilerSet compilerSet = pSettings->compilerSets().getSet(compilerSetIndex);
#ifdef ENABLE_SDCC
    if (compilerSet && compilerSet->compilerType()==CompilerType::SDCC)
        parser->setLanguage(ParserLanguage::SDCC);
#endif
    parser->clearIncludePaths();
    bool isCpp = parser->language()==ParserLanguage::CPlusPlus;
    if (compilerSet) {
        if (isCpp) {
            foreach  (const QString& file,compilerSet->CppIncludeDirs()) {
                parser->addIncludePath(file);
            }
        }
        foreach  (const QString& file,compilerSet->CIncludeDirs()) {
            parser->addIncludePath(file);
        }
        if (isCpp) {
            foreach  (const QString& file,compilerSet->defaultCppIncludeDirs()) {
                parser->addIncludePath(file);
            }
        }
        foreach  (const QString& file,compilerSet->defaultCIncludeDirs()) {
            parser->addIncludePath(file);
        }
        // Set defines
        foreach (const QString &define, compilerSet->defines(parser->language()==ParserLanguage::CPlusPlus)) {
            parser->addHardDefineByLine(define);
        }
//        // add a Red Pand C++ 's own macro
//        parser->addHardDefineByLine("#define EGE_FOR_AUTO_CODE_COMPLETETION_ONLY");
        // add C/C++ default macro
        parser->addHardDefineByLine("#define __FILE__  1");
        parser->addHardDefineByLine("#define __LINE__  1");
        parser->addHardDefineByLine("#define __DATE__  1");
        parser->addHardDefineByLine("#define __TIME__  1");
    }
    parser->parseHardDefines();
    pMainWindow->disconnect(parser.get(),
                            &CppParser::parseStarted,
                            pMainWindow,
                            &MainWindow::onParseStarted);
    pMainWindow->disconnect(parser.get(),
                            &CppParser::progress,
                            pMainWindow,
                            &MainWindow::onParserProgress);
    pMainWindow->disconnect(parser.get(),
                            &CppParser::parseFinished,
                            pMainWindow,
                            &MainWindow::onParseFinished);
    pMainWindow->connect(parser.get(),
                            &CppParser::parseStarted,
                            pMainWindow,
                            &MainWindow::onParseStarted);
    pMainWindow->connect(parser.get(),
                            &CppParser::progress,
                            pMainWindow,
                            &MainWindow::onParserProgress);
    pMainWindow->connect(parser.get(),
                            &CppParser::parseFinished,
                            pMainWindow,
                            &MainWindow::onParseFinished);
}
