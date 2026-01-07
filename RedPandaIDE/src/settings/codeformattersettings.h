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
#ifndef CODE_FORMATTER_SETTINGS_H
#define CODE_FORMATTER_SETTINGS_H
#include "basesettings.h"

#define SETTING_CODE_FORMATTER "CodeFormatter"

class CodeFormatterSettings: public BaseSettings {
public:
    explicit CodeFormatterSettings(SettingsPersistor* persistor);
    QStringList getArguments();
    int braceStyle() const;
    void setBraceStyle(int newBraceStyle);

    int indentStyle() const;
    void setIndentStyle(int newIndentStyle);
    int tabWidth() const;
    void setTabWidth(int newTabWidth);
    bool attachNamespaces() const;
    void setAttachNamespaces(bool newAttachNamespaces);
    bool attachClasses() const;
    void setAttachClasses(bool newAttachClasses);
    bool attachInlines() const;
    void setAttachInlines(bool newAttachInlines);
    bool attachExternC() const;
    void setAttachExternC(bool newAttachExternC);
    bool attachClosingWhile() const;
    void setAttachClosingWhile(bool newAttachClosingWhile);
    bool indentClasses() const;
    void setIndentClasses(bool newIndentClasses);
    bool indentModifiers() const;
    void setIndentModifiers(bool newIndentModifiers);
    bool indentCases() const;
    void setIndentCases(bool newIndentCases);
    bool indentNamespaces() const;
    void setIndentNamespaces(bool newIndentNamespaces);
    int indentContinuation() const;
    void setIndentContinuation(int newIndentContinuation);
    bool indentLabels() const;
    void setIndentLabels(bool newIndentLabels);
    bool indentPreprocBlock() const;
    void setIndentPreprocBlock(bool newIndentPreprocBlock);
    bool indentPreprocCond() const;
    void setIndentPreprocCond(bool newIndentPreprocCond);
    bool indentPreprocDefine() const;
    void setIndentPreprocDefine(bool newIndentPreprocDefine);
    bool indentCol1Comments() const;
    void setIndentCol1Comments(bool newIndentCol1Comments);
    int minConditionalIndent() const;
    void setMinConditionalIndent(int newMinConditionalIndent);
    int maxContinuationIndent() const;
    void setMaxContinuationIndent(int newMaxContinuationIndent);
    bool breakBlocks() const;
    void setBreakBlocks(bool newBreakBlocks);
    bool breakBlocksAll() const;
    void setBreakBlocksAll(bool newBreakBlocksAll);
    bool padOper() const;
    void setPadOper(bool newPadOper);
    bool padComma() const;
    void setPadComma(bool newPadComma);
    bool padParen() const;
    void setPadParen(bool newPadParen);
    bool padParenOut() const;
    void setPadParenOut(bool newPadParenOut);
    bool padFirstParenOut() const;
    void setPadFirstParenOut(bool newPadFirstParenOut);
    bool padParenIn() const;
    void setPadParenIn(bool newPadParenIn);
    bool padHeader() const;
    void setPadHeader(bool newPadHeader);
    bool unpadParen() const;
    void setUnpadParen(bool newUnpadParen);
    bool deleteEmptyLines() const;
    void setDeleteEmptyLines(bool newDeleteEmptyLines);
    bool fillEmptyLines() const;
    void setFillEmptyLines(bool newFillEmptyLines);
    int alignPointerStyle() const;
    void setAlignPointerStyle(int newAlignPointerStyle);
    int alignReferenceStyle() const;
    void setAlignReferenceStyle(int newAlignReferenceStyle);
    bool breakClosingBraces() const;
    void setBreakClosingBraces(bool newBreakClosingBraces);
    bool breakElseIf() const;
    void setBreakElseIf(bool newBreakElseIf);
    bool breakOneLineHeaders() const;
    void setBreakOneLineHeaders(bool newBreakOneLineHeaders);
    bool addBraces() const;
    void setAddBraces(bool newAddBraces);
    bool addOneLineBraces() const;
    void setAddOneLineBraces(bool newAddOneLineBraces);
    bool removeBraces() const;
    void setRemoveBraces(bool newRemoveBraces);
    bool breakReturnTypeDecl() const;
    void setBreakReturnTypeDecl(bool newBreakReturnTypeDecl);
    bool attachReturnType() const;
    void setAttachReturnType(bool newAttachReturnType);
    bool attachReturnTypeDecl() const;
    void setAttachReturnTypeDecl(bool newAttachReturnTypeDecl);
    bool keepOneLineBlocks() const;
    void setKeepOneLineBlocks(bool newKeepOneLineBlocks);
    bool keepOneLineStatements() const;
    void setKeepOneLineStatements(bool newKeepOneLineStatements);
    bool convertTabs() const;
    void setConvertTabs(bool newConvertTabs);
    bool closeTemplates() const;
    void setCloseTemplates(bool newCloseTemplates);
    bool removeCommentPrefix() const;
    void setRemoveCommentPrefix(bool newRemoveCommentPrefix);
    int maxCodeLength() const;
    void setMaxCodeLength(int newMaxCodeLength);
    bool breakAfterLogical() const;
    void setBreakAfterLogical(bool newBreakAfterLogical);

    bool breakReturnType() const;
    void setBreakReturnType(bool newBreakReturnType);

    bool breakMaxCodeLength() const;
    void setBreakMaxCodeLength(bool newBreakMaxCodeLength);

    bool indentSwitches() const;
    void setIndentSwitches(bool newIndentSwitches);

    bool indentAfterParens() const;
    void setIndentAfterParens(bool newIndentAfterParens);

    bool squeezeLines() const;
    void setSqueezeLines(bool newSqueezeLines);

    int squeezeLinesNumber() const;
    void setSqueezeLinesNumber(int newSqueezeLinesNumber);

    bool squeezeWhitespace() const;
    void setSqueezeWhitespace(bool newSqueezeWhitespace);

private:
    int mBraceStyle;
    int mIndentStyle;
    int mTabWidth;
    bool mAttachNamespaces;
    bool mAttachClasses;
    bool mAttachInlines;
    bool mAttachExternC;
    bool mAttachClosingWhile;
    bool mIndentClasses;
    bool mIndentModifiers;
    bool mIndentSwitches;
    bool mIndentCases;
    bool mIndentNamespaces;
    bool mIndentAfterParens;
    int mIndentContinuation;
    bool mIndentLabels;
    bool mIndentPreprocBlock;
    bool mIndentPreprocCond;
    bool mIndentPreprocDefine;
    bool mIndentCol1Comments;
    int mMinConditionalIndent;
    int mMaxContinuationIndent;
    bool mBreakBlocks;
    bool mBreakBlocksAll;
    bool mPadOper;
    bool mPadComma;
    bool mPadParen;
    bool mPadParenOut;
    bool mPadFirstParenOut;
    bool mPadParenIn;
    bool mPadHeader;
    bool mUnpadParen;
    bool mDeleteEmptyLines;
    bool mFillEmptyLines;
    bool mSqueezeLines;
    int mSqueezeLinesNumber;
    bool mSqueezeWhitespace;
    int mAlignPointerStyle;
    int mAlignReferenceStyle;
    bool mBreakClosingBraces;
    bool mBreakElseIf;
    bool mBreakOneLineHeaders;
    bool mAddBraces;
    bool mAddOneLineBraces;
    bool mRemoveBraces;
    bool mBreakReturnType;
    bool mBreakReturnTypeDecl;
    bool mAttachReturnType;
    bool mAttachReturnTypeDecl;
    bool mKeepOneLineBlocks;
    bool mKeepOneLineStatements;
    bool mConvertTabs;
    bool mCloseTemplates;
    bool mRemoveCommentPrefix;
    bool mBreakMaxCodeLength;
    int mMaxCodeLength;
    bool mBreakAfterLogical;
    // _Base interface
protected:
    void doSave() override;
    void doLoad() override;
};


#endif
//CODE_FORMATTER_SETTINGS_H
