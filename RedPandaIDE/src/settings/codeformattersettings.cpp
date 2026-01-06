#include "codeformattersettings.h"
#include "../utils.h"

CodeFormatterSettings::CodeFormatterSettings(SettingsPersistor *persistor):
    BaseSettings{persistor,SETTING_CODE_FORMATTER}
{

}

QStringList CodeFormatterSettings::getArguments()
{
    QStringList result;
    //force use english language
    result.append("-I");
    switch(mBraceStyle) {
    case FormatterBraceStyle::fbsDefault:
        break;
    case FormatterBraceStyle::fbsAllman:
        result.append("--style=allman");
        break;
    case FormatterBraceStyle::fbsJava:
        result.append("--style=java");
        break;
    case FormatterBraceStyle::fbsKR:
        result.append("--style=kr");
        break;
    case FormatterBraceStyle::fbsStroustrup:
        result.append("--style=stroustrup");
        break;
    case FormatterBraceStyle::fbsWitesmith:
        result.append("--style=whitesmith");
        break;
    case FormatterBraceStyle::fbsVtk:
        result.append("--style=vtk");
        break;
    case FormatterBraceStyle::fbsRatliff:
        result.append("--style=ratliff");
        break;
    case FormatterBraceStyle::fbsGNU:
        result.append("--style=gnu");
        break;
    case FormatterBraceStyle::fbsLinux:
        result.append("--style=linux");
        break;
    case FormatterBraceStyle::fbsHorstmann:
        result.append("--style=horstmann");
        break;
    case FormatterBraceStyle::fbs1TBS:
        result.append("--style=1tbs");
        break;
    case FormatterBraceStyle::fbsGoogle:
        result.append("--style=google");
        break;
    case FormatterBraceStyle::fbsMozilla:
        result.append("--style=mozilla");
        break;
    case FormatterBraceStyle::fbsWebkit:
        result.append("--style=webkit");
        break;
    case FormatterBraceStyle::fbsPico:
        result.append("--style=pico");
        break;
    case FormatterBraceStyle::fbsLisp:
        result.append("--style=lisp");
        break;
    };
    switch(mIndentStyle) {
    case FormatterIndentType::fitTab:
        result.append(QString("--indent=tab=%1").arg(mTabWidth));
        break;
    case FormatterIndentType::fitSpace:
        result.append(QString("--indent=spaces=%1").arg(mTabWidth));
        break;
    }
    if (mAttachNamespaces)
        result.append("--attach-namespaces");
    if (mAttachClasses)
        result.append("--attach-classes");
    if (mAttachInlines)
        result.append("--attach-inlines");
    if (mAttachExternC)
        result.append("--attach-extern-c");
    if (mAttachClosingWhile)
        result.append("--attach-closing-while");
    if (mIndentClasses)
        result.append("--indent-classes");
    if (mIndentModifiers)
        result.append("--indent-modifiers");
    if (mIndentSwitches)
        result.append("--indent-switches");
    if (mIndentCases)
        result.append("--indent-cases");
    if (mIndentNamespaces)
        result.append("--indent-namespaces");
    if (mIndentAfterParens)
        result.append("--indent-after-parens");
    if (mIndentContinuation!=1)
        result.append(QString("--indent-continuation=%1").arg(mIndentContinuation));
    if (mIndentLabels)
        result.append("--indent-labels");
    if (mIndentPreprocBlock)
        result.append("--indent-preproc-block");
    if (mIndentPreprocCond)
        result.append("--indent-preproc-cond");
    if (mIndentPreprocDefine)
        result.append("--indent-preproc-define");
    if (mIndentCol1Comments)
        result.append("--indent-col1-comments");
    if (mMinConditionalIndent!=2)
        result.append(QString("--min-conditional-indent=%1").arg(mMinConditionalIndent));
    if (mMaxContinuationIndent!=40)
        result.append(QString("--max-continuation-indent=%1").arg(mMaxContinuationIndent));
    if (mBreakBlocks)
        result.append("--break-blocks");
    else if (mBreakBlocksAll)
        result.append("--break-blocks=all");
    if (mPadOper)
        result.append("--pad-oper");
    if (mPadComma)
        result.append("--pad-comma");
    if (mPadParen)
        result.append("--pad-paren");
    if (mPadParenOut)
        result.append("--pad-paren-out");
    if (mPadFirstParenOut)
        result.append("--pad-first-paren-out");
    if (mPadParenIn)
        result.append("--pad-paren-in");
    if (mPadHeader)
        result.append("--pad-header");
    if (mUnpadParen)
        result.append("--unpad-paren");
    if (mDeleteEmptyLines)
        result.append("--delete-empty-lines");
    if (mFillEmptyLines)
        result.append("--fill-empty-lines");
    if (mSqueezeLines)
        result.append(QString("--squeeze-lines=%1").arg(mSqueezeLinesNumber));
    if (mSqueezeWhitespace)
        result.append(QString("--squeeze-ws").arg(mSqueezeLinesNumber));
    switch(mAlignPointerStyle) {
    case FormatterOperatorAlign::foaNone:
        break;
    case FormatterOperatorAlign::foaType:
        result.append("--align-pointer=type");
        break;
    case FormatterOperatorAlign::foaMiddle:
        result.append("--align-pointer=middle");
        break;
    case FormatterOperatorAlign::foaName:
        result.append("--align-pointer=name");
        break;
    }
    switch(mAlignReferenceStyle) {
    case FormatterOperatorAlign::foaNone:
        break;
    case FormatterOperatorAlign::foaType:
        result.append("--align-reference=type");
        break;
    case FormatterOperatorAlign::foaMiddle:
        result.append("--align-reference=middle");
        break;
    case FormatterOperatorAlign::foaName:
        result.append("--align-reference=name");
        break;
    }

    if (mBreakClosingBraces)
        result.append("--break-closing-braces");
    if (mBreakElseIf)
        result.append("--break-elseifs");
    if (mBreakOneLineHeaders)
        result.append("--break-one-line-headers");
    if (mAddBraces)
        result.append("--add-braces");
    if (mAddOneLineBraces)
        result.append("--add-one-line-braces");
    if (mRemoveBraces)
        result.append("--remove-braces");
    if (mBreakReturnType)
        result.append("--break-return-type");
    if (mBreakReturnTypeDecl)
        result.append("--break-return-type-decl");
    if (mAttachReturnType)
        result.append("--attach-return-type");
    if (mAttachReturnTypeDecl)
        result.append("--attach-return-type-decl");
    if (mKeepOneLineBlocks)
        result.append("--keep-one-line-blocks");
    if (mKeepOneLineStatements)
        result.append("--keep-one-line-statements");
    if (mConvertTabs)
        result.append("--convert-tabs");
    if (mCloseTemplates)
        result.append("--close-templates");
    if (mRemoveCommentPrefix)
        result.append("--remove-comment-prefix");
    if (mBreakMaxCodeLength) {
        result.append(QString("--max-code-length=%1").arg(mMaxCodeLength));
        if (mBreakAfterLogical)
            result.append("--break-after-logical");
    }
    return result;
}

int CodeFormatterSettings::indentStyle() const
{
    return mIndentStyle;
}

void CodeFormatterSettings::setIndentStyle(int newIndentStyle)
{
    mIndentStyle = newIndentStyle;
}

int CodeFormatterSettings::tabWidth() const
{
    return mTabWidth;
}

void CodeFormatterSettings::setTabWidth(int newTabWidth)
{
    mTabWidth = newTabWidth;
}

bool CodeFormatterSettings::attachNamespaces() const
{
    return mAttachNamespaces;
}

void CodeFormatterSettings::setAttachNamespaces(bool newAttachNamespaces)
{
    mAttachNamespaces = newAttachNamespaces;
}

bool CodeFormatterSettings::attachClasses() const
{
    return mAttachClasses;
}

void CodeFormatterSettings::setAttachClasses(bool newAttachClasses)
{
    mAttachClasses = newAttachClasses;
}

bool CodeFormatterSettings::attachInlines() const
{
    return mAttachInlines;
}

void CodeFormatterSettings::setAttachInlines(bool newAttachInlines)
{
    mAttachInlines = newAttachInlines;
}

bool CodeFormatterSettings::attachExternC() const
{
    return mAttachExternC;
}

void CodeFormatterSettings::setAttachExternC(bool newAttachExternC)
{
    mAttachExternC = newAttachExternC;
}

bool CodeFormatterSettings::attachClosingWhile() const
{
    return mAttachClosingWhile;
}

void CodeFormatterSettings::setAttachClosingWhile(bool newAttachClosingWhile)
{
    mAttachClosingWhile = newAttachClosingWhile;
}

bool CodeFormatterSettings::indentClasses() const
{
    return mIndentClasses;
}

void CodeFormatterSettings::setIndentClasses(bool newIndentClasses)
{
    mIndentClasses = newIndentClasses;
}

bool CodeFormatterSettings::indentModifiers() const
{
    return mIndentModifiers;
}

void CodeFormatterSettings::setIndentModifiers(bool newIndentModifiers)
{
    mIndentModifiers = newIndentModifiers;
}

bool CodeFormatterSettings::indentCases() const
{
    return mIndentCases;
}

void CodeFormatterSettings::setIndentCases(bool newIndentCases)
{
    mIndentCases = newIndentCases;
}

bool CodeFormatterSettings::indentNamespaces() const
{
    return mIndentNamespaces;
}

void CodeFormatterSettings::setIndentNamespaces(bool newIndentNamespaces)
{
    mIndentNamespaces = newIndentNamespaces;
}

int CodeFormatterSettings::indentContinuation() const
{
    return mIndentContinuation;
}

void CodeFormatterSettings::setIndentContinuation(int newIndentContinuation)
{
    mIndentContinuation = newIndentContinuation;
}

bool CodeFormatterSettings::indentLabels() const
{
    return mIndentLabels;
}

void CodeFormatterSettings::setIndentLabels(bool newIndentLabels)
{
    mIndentLabels = newIndentLabels;
}

bool CodeFormatterSettings::indentPreprocBlock() const
{
    return mIndentPreprocBlock;
}

void CodeFormatterSettings::setIndentPreprocBlock(bool newIndentPreprocBlock)
{
    mIndentPreprocBlock = newIndentPreprocBlock;
}

bool CodeFormatterSettings::indentPreprocCond() const
{
    return mIndentPreprocCond;
}

void CodeFormatterSettings::setIndentPreprocCond(bool newIndentPreprocCond)
{
    mIndentPreprocCond = newIndentPreprocCond;
}

bool CodeFormatterSettings::indentPreprocDefine() const
{
    return mIndentPreprocDefine;
}

void CodeFormatterSettings::setIndentPreprocDefine(bool newIndentPreprocDefine)
{
    mIndentPreprocDefine = newIndentPreprocDefine;
}

bool CodeFormatterSettings::indentCol1Comments() const
{
    return mIndentCol1Comments;
}

void CodeFormatterSettings::setIndentCol1Comments(bool newIndentCol1Comments)
{
    mIndentCol1Comments = newIndentCol1Comments;
}

int CodeFormatterSettings::minConditionalIndent() const
{
    return mMinConditionalIndent;
}

void CodeFormatterSettings::setMinConditionalIndent(int newMinConditionalIndent)
{
    mMinConditionalIndent = newMinConditionalIndent;
}

int CodeFormatterSettings::maxContinuationIndent() const
{
    return mMaxContinuationIndent;
}

void CodeFormatterSettings::setMaxContinuationIndent(int newMaxContinuationIndent)
{
    mMaxContinuationIndent = newMaxContinuationIndent;
}

bool CodeFormatterSettings::breakBlocks() const
{
    return mBreakBlocks;
}

void CodeFormatterSettings::setBreakBlocks(bool newBreakBlocks)
{
    mBreakBlocks = newBreakBlocks;
}

bool CodeFormatterSettings::breakBlocksAll() const
{
    return mBreakBlocksAll;
}

void CodeFormatterSettings::setBreakBlocksAll(bool newBreakBlocksAll)
{
    mBreakBlocksAll = newBreakBlocksAll;
}

bool CodeFormatterSettings::padOper() const
{
    return mPadOper;
}

void CodeFormatterSettings::setPadOper(bool newPadOper)
{
    mPadOper = newPadOper;
}

bool CodeFormatterSettings::padComma() const
{
    return mPadComma;
}

void CodeFormatterSettings::setPadComma(bool newPadComma)
{
    mPadComma = newPadComma;
}

bool CodeFormatterSettings::padParen() const
{
    return mPadParen;
}

void CodeFormatterSettings::setPadParen(bool newPadParen)
{
    mPadParen = newPadParen;
}

bool CodeFormatterSettings::padParenOut() const
{
    return mPadParenOut;
}

void CodeFormatterSettings::setPadParenOut(bool newPadParenOut)
{
    mPadParenOut = newPadParenOut;
}

bool CodeFormatterSettings::padFirstParenOut() const
{
    return mPadFirstParenOut;
}

void CodeFormatterSettings::setPadFirstParenOut(bool newPadFirstParenOut)
{
    mPadFirstParenOut = newPadFirstParenOut;
}

bool CodeFormatterSettings::padParenIn() const
{
    return mPadParenIn;
}

void CodeFormatterSettings::setPadParenIn(bool newPadParenIn)
{
    mPadParenIn = newPadParenIn;
}

bool CodeFormatterSettings::padHeader() const
{
    return mPadHeader;
}

void CodeFormatterSettings::setPadHeader(bool newPadHeader)
{
    mPadHeader = newPadHeader;
}

bool CodeFormatterSettings::unpadParen() const
{
    return mUnpadParen;
}

void CodeFormatterSettings::setUnpadParen(bool newUnpadParen)
{
    mUnpadParen = newUnpadParen;
}

bool CodeFormatterSettings::deleteEmptyLines() const
{
    return mDeleteEmptyLines;
}

void CodeFormatterSettings::setDeleteEmptyLines(bool newDeleteEmptyLines)
{
    mDeleteEmptyLines = newDeleteEmptyLines;
}

bool CodeFormatterSettings::fillEmptyLines() const
{
    return mFillEmptyLines;
}

void CodeFormatterSettings::setFillEmptyLines(bool newFillEmptyLines)
{
    mFillEmptyLines = newFillEmptyLines;
}

int CodeFormatterSettings::alignPointerStyle() const
{
    return mAlignPointerStyle;
}

void CodeFormatterSettings::setAlignPointerStyle(int newAlignPointerStyle)
{
    mAlignPointerStyle = newAlignPointerStyle;
}

int CodeFormatterSettings::alignReferenceStyle() const
{
    return mAlignReferenceStyle;
}

void CodeFormatterSettings::setAlignReferenceStyle(int newAlignReferenceStyle)
{
    mAlignReferenceStyle = newAlignReferenceStyle;
}

bool CodeFormatterSettings::breakClosingBraces() const
{
    return mBreakClosingBraces;
}

void CodeFormatterSettings::setBreakClosingBraces(bool newBreakClosingBraces)
{
    mBreakClosingBraces = newBreakClosingBraces;
}

bool CodeFormatterSettings::breakElseIf() const
{
    return mBreakElseIf;
}

void CodeFormatterSettings::setBreakElseIf(bool newBreakElseIf)
{
    mBreakElseIf = newBreakElseIf;
}

bool CodeFormatterSettings::breakOneLineHeaders() const
{
    return mBreakOneLineHeaders;
}

void CodeFormatterSettings::setBreakOneLineHeaders(bool newBreakOneLineHeaders)
{
    mBreakOneLineHeaders = newBreakOneLineHeaders;
}

bool CodeFormatterSettings::addBraces() const
{
    return mAddBraces;
}

void CodeFormatterSettings::setAddBraces(bool newAddBraces)
{
    mAddBraces = newAddBraces;
}

bool CodeFormatterSettings::addOneLineBraces() const
{
    return mAddOneLineBraces;
}

void CodeFormatterSettings::setAddOneLineBraces(bool newAddOneLineBraces)
{
    mAddOneLineBraces = newAddOneLineBraces;
}

bool CodeFormatterSettings::removeBraces() const
{
    return mRemoveBraces;
}

void CodeFormatterSettings::setRemoveBraces(bool newRemoveBraces)
{
    mRemoveBraces = newRemoveBraces;
}

bool CodeFormatterSettings::breakReturnTypeDecl() const
{
    return mBreakReturnTypeDecl;
}

void CodeFormatterSettings::setBreakReturnTypeDecl(bool newBreakReturnTypeDecl)
{
    mBreakReturnTypeDecl = newBreakReturnTypeDecl;
}

bool CodeFormatterSettings::attachReturnType() const
{
    return mAttachReturnType;
}

void CodeFormatterSettings::setAttachReturnType(bool newAttachReturnType)
{
    mAttachReturnType = newAttachReturnType;
}

bool CodeFormatterSettings::attachReturnTypeDecl() const
{
    return mAttachReturnTypeDecl;
}

void CodeFormatterSettings::setAttachReturnTypeDecl(bool newAttachReturnTypeDecl)
{
    mAttachReturnTypeDecl = newAttachReturnTypeDecl;
}

bool CodeFormatterSettings::keepOneLineBlocks() const
{
    return mKeepOneLineBlocks;
}

void CodeFormatterSettings::setKeepOneLineBlocks(bool newKeepOneLineBlocks)
{
    mKeepOneLineBlocks = newKeepOneLineBlocks;
}

bool CodeFormatterSettings::keepOneLineStatements() const
{
    return mKeepOneLineStatements;
}

void CodeFormatterSettings::setKeepOneLineStatements(bool newKeepOneLineStatements)
{
    mKeepOneLineStatements = newKeepOneLineStatements;
}

bool CodeFormatterSettings::convertTabs() const
{
    return mConvertTabs;
}

void CodeFormatterSettings::setConvertTabs(bool newConvertTabs)
{
    mConvertTabs = newConvertTabs;
}

bool CodeFormatterSettings::closeTemplates() const
{
    return mCloseTemplates;
}

void CodeFormatterSettings::setCloseTemplates(bool newCloseTemplates)
{
    mCloseTemplates = newCloseTemplates;
}

bool CodeFormatterSettings::removeCommentPrefix() const
{
    return mRemoveCommentPrefix;
}

void CodeFormatterSettings::setRemoveCommentPrefix(bool newRemoveCommentPrefix)
{
    mRemoveCommentPrefix = newRemoveCommentPrefix;
}

int CodeFormatterSettings::maxCodeLength() const
{
    return mMaxCodeLength;
}

void CodeFormatterSettings::setMaxCodeLength(int newMaxCodeLength)
{
    mMaxCodeLength = newMaxCodeLength;
}

bool CodeFormatterSettings::breakAfterLogical() const
{
    return mBreakAfterLogical;
}

void CodeFormatterSettings::setBreakAfterLogical(bool newBreakAfterLogical)
{
    mBreakAfterLogical = newBreakAfterLogical;
}

bool CodeFormatterSettings::breakReturnType() const
{
    return mBreakReturnType;
}

void CodeFormatterSettings::setBreakReturnType(bool newBreakReturnType)
{
    mBreakReturnType = newBreakReturnType;
}

bool CodeFormatterSettings::breakMaxCodeLength() const
{
    return mBreakMaxCodeLength;
}

void CodeFormatterSettings::setBreakMaxCodeLength(bool newBreakMaxCodeLength)
{
    mBreakMaxCodeLength = newBreakMaxCodeLength;
}

bool CodeFormatterSettings::indentAfterParens() const
{
    return mIndentAfterParens;
}

void CodeFormatterSettings::setIndentAfterParens(bool newIndentAfterParens)
{
    mIndentAfterParens = newIndentAfterParens;
}

bool CodeFormatterSettings::squeezeWhitespace() const
{
    return mSqueezeWhitespace;
}

void CodeFormatterSettings::setSqueezeWhitespace(bool newSqueezeWhitespace)
{
    mSqueezeWhitespace = newSqueezeWhitespace;
}

int CodeFormatterSettings::squeezeLinesNumber() const
{
    return mSqueezeLinesNumber;
}

void CodeFormatterSettings::setSqueezeLinesNumber(int newSqueezeLinesNumber)
{
    mSqueezeLinesNumber = newSqueezeLinesNumber;
}

bool CodeFormatterSettings::squeezeLines() const
{
    return mSqueezeLines;
}

void CodeFormatterSettings::setSqueezeLines(bool newSqueezeLines)
{
    mSqueezeLines = newSqueezeLines;
}

bool CodeFormatterSettings::indentSwitches() const
{
    return mIndentSwitches;
}

void CodeFormatterSettings::setIndentSwitches(bool newIndentSwitches)
{
    mIndentSwitches = newIndentSwitches;
}

void CodeFormatterSettings::doSave()
{
    saveValue("brace_style",mBraceStyle);
    saveValue("indent_style",mIndentStyle);
    saveValue("tab_width",mTabWidth);
    saveValue("attach_namespaces",mAttachNamespaces);
    saveValue("attach_classes",mAttachClasses);
    saveValue("attach_inlines",mAttachInlines);
    saveValue("attach_extern_c",mAttachExternC);
    saveValue("attach_closing_while",mAttachClosingWhile);
    saveValue("indent_classes",mIndentClasses);
    saveValue("indent_modifiers",mIndentModifiers);
    saveValue("indent_switches",mIndentSwitches);
    saveValue("indent_cases",mIndentCases);
    saveValue("indent_namespaces",mIndentNamespaces);
    saveValue("indent_after_parents",mIndentAfterParens);
    saveValue("indent_continuation",mIndentContinuation);
    saveValue("indent_labels",mIndentLabels);
    saveValue("indent_preproc_block",mIndentPreprocBlock);
    saveValue("indent_preproc_cond",mIndentPreprocCond);
    saveValue("indent_preproc_define",mIndentPreprocDefine);
    saveValue("indent_col1_comments",mIndentCol1Comments);
    saveValue("min_conditional_indent",mMinConditionalIndent);
    saveValue("max_continuation_indent",mMaxContinuationIndent);
    saveValue("break_blocks",mBreakBlocks);
    saveValue("break_blocks_all",mBreakBlocksAll);
    saveValue("pad_oper",mPadOper);
    saveValue("pad_comma",mPadComma);
    saveValue("pad_paren",mPadParen);
    saveValue("pad_paren_out",mPadParenOut);
    saveValue("pad_first_paren_out",mPadFirstParenOut);
    saveValue("pad_parent_in",mPadParenIn);
    saveValue("pad_header",mPadHeader);
    saveValue("unpad_paren",mUnpadParen);
    saveValue("delete_empty_lines",mDeleteEmptyLines);
    saveValue("fill_empty_lines",mFillEmptyLines);
    saveValue("squeeze_lines", mSqueezeLines);
    saveValue("squeeze_line_number", mSqueezeLinesNumber);
    saveValue("squeeze_whitespace", mSqueezeWhitespace);

    saveValue("align_pointer_style",mAlignPointerStyle);
    saveValue("align_reference_style",mAlignReferenceStyle);
    saveValue("break_closing_braces",mBreakClosingBraces);
    saveValue("break_else_if",mBreakElseIf);
    saveValue("break_one_line_headers",mBreakOneLineHeaders);
    saveValue("add_braces",mAddBraces);
    saveValue("add_one_line_braces",mAddOneLineBraces);
    saveValue("remove_braces",mRemoveBraces);
    saveValue("break_return_type",mBreakReturnType);
    saveValue("break_return_type_decl",mBreakReturnTypeDecl);
    saveValue("attach_return_type",mAttachReturnType);
    saveValue("attach_return_type_decl",mAttachReturnTypeDecl);
    saveValue("keep_one_line_blocks",mKeepOneLineBlocks);
    saveValue("keep_one_line_statements",mKeepOneLineStatements);
    saveValue("convert_tabs",mConvertTabs);
    saveValue("close_templates",mCloseTemplates);
    saveValue("remove_comment_prefix",mRemoveCommentPrefix);
    saveValue("break_max_code_length",mBreakMaxCodeLength);
    saveValue("max_code_length",mMaxCodeLength);
    saveValue("break_after_logical",mBreakAfterLogical);
}

void CodeFormatterSettings::doLoad()
{
    mBraceStyle = intValue("brace_style", FormatterBraceStyle::fbsJava);
    mIndentStyle = intValue("indent_style",FormatterIndentType::fitTab); // 0 isspaces, 1 is tab
    mTabWidth = intValue("tab_width",4);
    mAttachNamespaces = boolValue("attach_namespaces",false);
    mAttachClasses = boolValue("attach_classes",false);
    mAttachInlines = boolValue("attach_inlines",false);
    mAttachExternC = boolValue("attach_extern_c",false);
    mAttachClosingWhile = boolValue("attach_closing_while",false);
    mIndentClasses = boolValue("indent_classes",false);
    mIndentModifiers = boolValue("indent_modifiers",false);
    mIndentSwitches = boolValue("indent_switches",true);
    mIndentCases = boolValue("indent_cases",false);
    mIndentNamespaces = boolValue("indent_namespaces",true);
    mIndentAfterParens = boolValue("indent_after_parents",false);
    mIndentContinuation = intValue("indent_continuation",1);
    mIndentLabels = boolValue("indent_labels",false);
    mIndentPreprocBlock = boolValue("indent_preproc_block",true);
    mIndentPreprocCond = boolValue("indent_preproc_cond",false);
    mIndentPreprocDefine = boolValue("indent_preproc_define",false);
    mIndentCol1Comments = boolValue("indent_col1_comments",false);
    mMinConditionalIndent = intValue("min_conditional_indent",1);
    mMaxContinuationIndent = intValue("max_continuation_indent",40);
    mBreakBlocks = boolValue("break_blocks",false);
    mBreakBlocksAll = boolValue("break_blocks_all",false);
    mPadOper = boolValue("pad_oper",true);
    mPadComma = boolValue("pad_comma",true);
    mPadParen = boolValue("pad_paren",false);
    mPadParenOut = boolValue("pad_paren_out",false);
    mPadFirstParenOut = boolValue("pad_first_paren_out",false);
    mPadParenIn = boolValue("pad_parent_in",false);
    mPadHeader = boolValue("pad_header",true);
    mUnpadParen = boolValue("unpad_paren",false);
    mDeleteEmptyLines = boolValue("delete_empty_lines",false);
    mFillEmptyLines = boolValue("fill_empty_lines",false);

    mSqueezeLines = boolValue("squeeze_lines", false);
    mSqueezeLinesNumber = intValue("squeeze_line_number", 1);
    mSqueezeWhitespace = boolValue("squeeze_whitespace", false);
    mAlignPointerStyle = intValue("align_pointer_style", FormatterOperatorAlign::foaNone);
    mAlignReferenceStyle = intValue("align_reference_style", FormatterOperatorAlign::foaNone);
    mBreakClosingBraces = boolValue("break_closing_braces",false);
    mBreakElseIf = boolValue("break_else_if",false);
    mBreakOneLineHeaders = boolValue("break_one_line_headers",false);
    mAddBraces = boolValue("add_braces",false);
    mAddOneLineBraces = boolValue("add_one_line_braces",false);
    mRemoveBraces = boolValue("remove_braces",false);
    mBreakReturnType = boolValue("break_return_type",false);
    mBreakReturnTypeDecl = boolValue("break_return_type_decl",false);
    mAttachReturnType = boolValue("attach_return_type",false);
    mAttachReturnTypeDecl = boolValue("attach_return_type_decl",false);
    mKeepOneLineBlocks = boolValue("keep_one_line_blocks",false);
    mKeepOneLineStatements = boolValue("keep_one_line_statements",false);
    mConvertTabs = boolValue("convert_tabs",false);
    mCloseTemplates = boolValue("close_templates",false);
    mRemoveCommentPrefix = boolValue("remove_comment_prefix",false);
    mBreakMaxCodeLength = boolValue("break_max_code_length",false);
    mMaxCodeLength = intValue("max_code_length",80);
    mBreakAfterLogical = boolValue("break_after_logical",false);
}

int CodeFormatterSettings::braceStyle() const
{
    return mBraceStyle;
}

void CodeFormatterSettings::setBraceStyle(int newBraceStyle)
{
    mBraceStyle = newBraceStyle;
}
