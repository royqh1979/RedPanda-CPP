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
#ifndef EDITOR_H
#define EDITOR_H

#include <QObject>
#include "utils/types.h"
#include "utils/parsemacros.h"
#include "utils.h"
#include "qsynedit/qsynedit.h"
#include "colorscheme.h"
#include "common.h"
#include "widgets/codecompletionpopup.h"
#include "widgets/headercompletionpopup.h"
#include "settings/codecompletionsettings.h"
#include "settings/editorsettings.h"
#include "compiler/compilerinfo.h"
#include "reformatter/basereformatter.h"

#define USER_CODE_IN_INSERT_POS "%INSERT%"
#define USER_CODE_IN_REPL_POS_BEGIN "%REPL_BEGIN%"
#define USER_CODE_IN_REPL_POS_END "%REPL_END%"

class Project;
class CppParser;
using PCppParser = std::shared_ptr<CppParser>;
class QTemporaryFile;
class QFileSystemWatcher;
class FunctionTooltipWidget;
class BreakpointModel;
class BookmarkModel;
class Settings;
class CodeSnippetsManager;
struct TabStop {
    int x;
    int endX;
    int y;
};
using PTabStop = std::shared_ptr<TabStop>;
class Editor;

using GetSharedParserrFunc = std::function<PCppParser (ParserLanguage)>;
using GetOpennedEditorFunc = std::function<Editor *(const QString &)>;
using GetFileStreamFunc = std::function<bool (const QString&, QStringList&)>;
using CanShowEvalTipFunc = std::function<bool ()>;
using RequestEvalTipFunc = std::function<bool (Editor *, const QString &)>;
using EvalTipReadyCallback = std::function<void (Editor *)>;
using GetCompilerTypeForEditorFunc = std::function<CompilerType (Editor *)>;
using GetReformatterFunc = std::function<std::unique_ptr<BaseReformatter>(Editor *)>;

class Editor : public QSynedit::QSynEdit
{
    Q_OBJECT
public:
    enum class LastSymbolType {
        Identifier,
        ScopeResolutionOperator, //'::'
        ObjectMemberOperator, //'.'
        PointerMemberOperator, //'->'
        PointerToMemberOfObjectOperator, //'.*'
        PointerToMemberOfPointerOperator, //'->*'
        MatchingBracket,
        BracketMatched,
        MatchingParenthesis,
        ParenthesisMatched,
        TildeSign,    // '~'
        AsteriskSign, // '*'
        AmpersandSign, // '&'
        MatchingAngleQuotation,
        AngleQuotationMatched,
        None
    };

    enum MarginNumber {
        LineNumberMargin = 0,
        MarkerMargin = 1,
        FoldMargin = 2,
    };

    enum MarkerNumber {
        BreakpointMarker,
        ErrorMarker,
        WarningMarker
    };

    enum class QuoteStatus {
        NotQuote,
        SingleQuote,
        SingleQuoteEscape,
        DoubleQuote,
        DoubleQuoteEscape,
        RawString,
        RawStringNoEscape,
        RawStringEnd
    };

    enum class WordPurpose {
        wpCompletion, // walk backwards over words, array, functions, parents, no forward movement
        wpEvaluation, // walk backwards over words, array, functions, parents, forwards over words, array
        wpHeaderCompletion, // walk backwards over path
        wpHeaderCompletionStart, // walk backwards over path, including start '<' or '"'
        wpDirective, // preprocessor
        wpJavadoc, //javadoc
        wpInformation, // walk backwards over words, array, functions, parents, forwards over words
        wpATTASMKeywords,
        wpKeywords
    };

    enum class TipType {
      Include, // cursor hovers above include
      Identifier, // cursor hovers above identifier
      Selection, // cursor hovers above selection
      Keyword,
      Number,
      None, // mouseover not allowed
      Error //Cursor hovers above error line/item;
    };

    struct SyntaxIssue {
        int startChar;
        int endChar;
        CompileIssueType issueType;
        QString token;
        QString hint;
    };

    using PSyntaxIssue = std::shared_ptr<SyntaxIssue>;
    using SyntaxIssueList = QVector<PSyntaxIssue>;
    using PSyntaxIssueList = std::shared_ptr<SyntaxIssueList>;

    explicit Editor(QWidget *parent);

    ~Editor();

    //tell the compiler to prohibit copy/moving editor objects ( we should only use pointers to the editor object)
    Editor(const Editor&) = delete;
    Editor(const Editor&&) = delete;
    Editor& operator=(const Editor&) = delete;
    Editor& operator=(const Editor&&) = delete;

    const QByteArray& encodingOption() const noexcept;
    void setEditorEncoding(const QByteArray& encoding) noexcept;
    const QByteArray& fileEncoding() const noexcept;
    void convertToEncoding(const QByteArray& encoding);
    const QString& filename() const noexcept;

    bool inProject() const noexcept;
    bool isNew() const noexcept;

    void loadFile(QString filename = "");
    void saveFile(QString filename);
    bool save(bool force=false, bool reparse=true);
    bool saveAs(const QString& name="", bool fromProject = false);
    void setFilename(const QString& newName);

    QString caption();

    void applySettings();
    void applyColorScheme(const QString& schemeName);
    void setAutoIndent(bool indent);
    bool autoIndent();

    void copyToClipboard() override;
    void cutToClipboard() override;
    void copyAsHTML();

    void setCaretPosition(const QSynedit::CharPos & pos);

    void addSyntaxIssues(int line, int startChar, int endChar, CompileIssueType errorType, const QString& hint);
    void clearSyntaxIssues();
    void gotoNextSyntaxIssue();
    void gotoPrevSyntaxIssue();
    bool hasPrevSyntaxIssue() const;
    bool hasNextSyntaxIssue() const;
    PSyntaxIssueList getSyntaxIssuesAtLine(int line);
    PSyntaxIssue getSyntaxIssueAtPosition(const QSynedit::CharPos& pos);
    void toggleBreakpoint(int line);
    void clearBreakpoints();
    bool hasBreakpoint(int line);
    void toggleBookmark(int line);
    void addBookmark(int line);
    void removeBookmark(int line);
    bool hasBookmark(int line) const;
    void clearBookmarks();
    void removeBreakpointFocus();
    void setActiveBreakpointFocus(int Line, bool setFocus=true);
    QString getPreviousWordAtPositionForSuggestion(const QSynedit::CharPos& p,
                                                   QSynedit::TokenType &tokenType);
    QString getPreviousWordAtPositionForCompleteFunctionDefinition(const QSynedit::CharPos& p) const;
    void reformat(bool doReparse=true);
    void replaceContent(const QString &newContent, bool doReparse=true);
    void checkSyntaxInBack();
    void gotoDeclaration(const QSynedit::CharPos& pos);
    void gotoDefinition(const QSynedit::CharPos& pos);
    void reparse(bool resetParser);
    void reparseIfNeeded();
    void resetParserIfNeeded();
    void reparseTodo();
    void insertString(const QString& value, bool moveCursor);
    void insertCodeSnippet(const QString& code);
    void print();
    void exportAsRTF(const QString& rtfFilename);
    void exportAsHTML(const QString& htmlFilename);
    void resetBreakpoints(BreakpointModel *model);
    bool notParsed();
    void breakLine();
    void deleteWord();
    void deleteToWordStart();
    void deleteToWordEnd();
    void deleteLine();
    void duplicate();
    void deleteToEOL();
    void deleteToBOL();
    void gotoBlockStart();
    void gotoBlockEnd();
    void showCodeCompletion();

    QStringList getOwnerExpressionAndMemberAtPositionForCompletion(
            const QSynedit::CharPos& pos,
            QString& memberOperator,
            QStringList& memberExpression);
    QString getWordForCompletionSearch(const QSynedit::CharPos& pos,bool permitTilde);
    QStringList getExpressionAtPosition(
            const QSynedit::CharPos& pos);
    void resetBookmarks(BookmarkModel *model);

    const PCppParser &parser() const;

    void tab() override;

    void pageUp() { processCommand(QSynedit::EditCommand::PageUp); }
    void pageDown() { processCommand(QSynedit::EditCommand::PageDown); }
    void gotoLineStart() { processCommand(QSynedit::EditCommand::LineStart); }
    void gotoLineEnd() { processCommand(QSynedit::EditCommand::LineEnd); }
    void gotoPageStart() { processCommand(QSynedit::EditCommand::PageTop); }
    void gotoPageEnd() { processCommand(QSynedit::EditCommand::PageBottom); }
    void gotoFileStart() { processCommand(QSynedit::EditCommand::FileStart); }
    void gotoFileEnd() { processCommand(QSynedit::EditCommand::FileEnd); }
    void toggleReadonly();

    void pageUpAndSelect() { processCommand(QSynedit::EditCommand::SelPageUp); }
    void pageDownAndSelect() { processCommand(QSynedit::EditCommand::SelPageDown); }
    void selectToLineStart() { processCommand(QSynedit::EditCommand::SelLineStart); }
    void selectToLineEnd() { processCommand(QSynedit::EditCommand::SelLineEnd); }
    void selectToPageStart() { processCommand(QSynedit::EditCommand::SelPageTop); }
    void selectToPageEnd() { processCommand(QSynedit::EditCommand::SelPageBottom); }
    void selectToFileStart() { processCommand(QSynedit::EditCommand::SelFileStart); }
    void selectToFileEnd() { processCommand(QSynedit::EditCommand::SelFileEnd); }

    void setProject(Project* pProject);

    const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &statementColors() const;
    void setStatementColors(const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &newStatementColors);

    const QDateTime &hideTime() const;
    void setHideTime(const QDateTime &newHideTime);

    bool canAutoSave() const;
    void setCanAutoSave(bool newCanAutoSave);

    quint64 lastFocusOutTime() const;

    FileType fileType() const;
    void setFileType(FileType newFileType);
    const QString &contextFile() const;
    void setContextFile(const QString &newContextFile);

    bool autoBackupEnabled() const;
    void setAutoBackupEnabled(bool newEnableAutoBackup);

    FunctionTooltipWidget *functionTooltip() const;
    void setFunctionTooltip(FunctionTooltipWidget *newFunctionTooltip);

    HeaderCompletionPopup *headerCompletionPopup() const;
    void setHeaderCompletionPopup(HeaderCompletionPopup *newHeaderCompletionPopup);

    CodeCompletionPopup *completionPopup() const;
    void setCompletionPopup(CodeCompletionPopup *newCompletionPopup);

    const GetSharedParserrFunc &getSharedParserFunc() const;
    void setGetSharedParserFunc(const GetSharedParserrFunc &newSharedParserProviderCallBack);

    const GetOpennedEditorFunc &getOpennedEditorFunc() const;
    void setGetOpennedFunc(const GetOpennedEditorFunc &newOpennedEditorProviderCallBack);

    const GetFileStreamFunc &getFileStreamCallBack() const;
    void setGetFileStreamCallBack(const GetFileStreamFunc &newGetFileStreamCallBack);

    const RequestEvalTipFunc &requestEvalTipFunc() const;
    void setRequestEvalTipFunc(const RequestEvalTipFunc &newRequestEvalTipFunc);

    const EvalTipReadyCallback &evalTipReadyCallback() const;
    void setEvalTipReadyCallback(const EvalTipReadyCallback &newEvalTipReadyCallback);

    CodeSnippetsManager *codeSnippetsManager() const;
    void setCodeSnippetsManager(CodeSnippetsManager *newCodeSnippetsManager);

    QFileSystemWatcher *fileSystemWatcher() const;
    void setFileSystemWatcher(QFileSystemWatcher *newFileSystemWatcher);

    const CanShowEvalTipFunc &canShowEvalTipFunc() const;
    void setCanShowEvalTipFunc(const CanShowEvalTipFunc &newCanShowEvalTipFunc);

    void setEditorSettings(const EditorSettings *newEditorSettings);

    void setCodeCompletionSettings(const CodeCompletionSettings *newCodeCompletionSettings);
#ifdef ENABLE_SDCC
    const GetCompilerTypeForEditorFunc &getCompilerTypeForEditorFunc() const;
    void setGetCompilerTypeForEditorFunc(const GetCompilerTypeForEditorFunc &newGetCompilerTypeForEditorFunc);
#endif

    const GetReformatterFunc &getReformatterFunc() const;
    void setGetReformatterFunc(const GetReformatterFunc &newGetReformatterFunc);

    const GetMacroVarsFunc &getMacroVarsFunc() const;
    void setGetMacroVarsFunc(const GetMacroVarsFunc &newGetMacroVarsFunc);

signals:
    void fileSaving(Editor *e, const QString& filename);
    void fileSaveError(Editor *e, const QString& filename, const QString& reason);
    void fileSaved(Editor *e, const QString& filename);
    void fileRenamed(Editor *e, const QString& oldFilename, const QString& newFilename);
    void breakpointAdded(const Editor *e, int line);
    void breakpointRemoved(const Editor *e, int line);
    void breakpointsCleared(const Editor *e);
    void syntaxCheckRequested(Editor *e);
    void parseTodoRequested(const QString& filename, bool inProject);
    void openFileRequested(const QString& filename, FileType fileType, const QString& contextFile , const QSynedit::CharPos& caretPos);
    void symbolChoosed(const QString& filename, int usageCount);
    void fileEncodingChanged(Editor *e);
    void editorEncodingChanged(Editor *e);
    void showOccured(Editor *e);
    void focusInOccured(Editor *e);
    void closeOccured(Editor *e);
    void hideOccured(Editor *e);
    void fontSizeChangedByWheel(int newSize);
public slots:
    void onTipEvalValueReady(const QString& value);

private slots:
    void onStatusChanged(QSynedit::StatusChanges changes);
    void onGutterClicked(Qt::MouseButton button, int x, int y, int line);
    void onFunctionTipsTimer();
    void onAutoBackupTimer();
    void onTooltipTimer();
    void onParseFinished();

private:
    bool completionPopupVisible() const;
    bool headerCompletionPopupVisible() const;
    bool functionTooltipVisible() const;
    void loadContent(const QString& filename);
    void resolveAutoDetectEncodingOption();
    bool isBraceChar(QChar ch) const;
    bool shouldOpenInReadonly();
    QChar getCurrentChar();
    bool handleSymbolCompletion(QChar key);
    bool handleParentheseCompletion();
    bool handleParentheseSkip();
    bool handleBracketCompletion();
    bool handleBracketSkip();
    bool handleMultilineCommentCompletion();
    bool handleBraceCompletion();
    bool handleBraceSkip();
    bool handleSemiColonSkip();
    bool handlePeriodSkip();
    bool handleSingleQuoteCompletion();
    bool handleDoubleQuoteCompletion();
    bool handleGlobalIncludeCompletion();
    bool handleGlobalIncludeSkip();

    bool handleCodeCompletion(QChar key);
    void initParser();
    ParserLanguage calcParserLanguage();
    void undoSymbolCompletion(const QSynedit::CharPos &pos);
    QuoteStatus getQuoteStatus();

    void showCompletion(const QString& preWord, bool autoComplete, CodeCompletionType type);
    void showHeaderCompletion(bool autoComplete, bool forceShow=false);

    void initAutoBackup();
    void saveAutoBackup();
    void cleanAutoBackup();

    bool testInFunc(const QSynedit::CharPos& pos);

    void completionInsert(bool appendFunc=false);

    void headerCompletionInsert();

    bool onCompletionKeyPressed(QKeyEvent* event);
    bool onHeaderCompletionKeyPressed(QKeyEvent* event);
    bool onCompletionInputMethod(QInputMethodEvent *event);

    TipType getTipType(QPoint point, QSynedit::CharPos& pos);
    void cancelHint();
    QString getHeaderFileHint(const QString& s, bool fromNext);
    QString getParserHint(const QStringList& expression, const QSynedit::CharPos& p);
    void showDebugHint(const QString& s,int line);
    QString getErrorHint(const PSyntaxIssue& issue);
    QString getHintForFunction(const PStatement& statement,
                               const QString& filename, int line);

    void updateFunctionTip(bool showTip);
    void clearUserCodeInTabStops();
    void popUserCodeInTabStops();
    void onExportedFormatToken(QSynedit::PSyntaxer syntaxer, int Line, int column, const QString& token,
        QSynedit::PTokenAttribute &attr);
    void onScrollBarValueChanged();
    void updateHoverLink(int line);
    void cancelHoverLink();

    QSize calcCompletionPopupSize();
    void doSetFileType(FileType newFileType);

    void openFileInContext(const QString& filename, const QSynedit::CharPos& caretPos);
    bool needReparse();

    PStatement constructorToClass(PStatement constuctorStatement, const QSynedit::CharPos& p);

    int previousIdChars(const QSynedit::CharPos &pos);

private:
    bool mInited;
    QDateTime mBackupTime;
    QFile* mBackupFile;
    QByteArray mEditorEncoding; // the encoding type set by the user
    QByteArray mFileEncoding; // the real encoding of the file (auto detected)
    QString mFilename;
    //QTabWidget* mParentPageControl;
    Project* mProject;
    bool mIsNew;

    bool mCodeCompletionEnabled;


    QMap<int,PSyntaxIssueList> mSyntaxIssues;
    QColor mSyntaxErrorColor;
    QColor mSyntaxWarningColor;
    QColor mActiveBreakpointForegroundColor;
    QColor mActiveBreakpointBackgroundColor;
    QColor mBreakpointForegroundColor;
    QColor mBreakpointBackgroundColor;
    QColor mCurrentHighlighWordForeground;
    QColor mCurrentHighlighWordBackground;
    int mSyntaxErrorLine;
    int mLineCount;
    QSet<int> mBreakpointLines;
    QSet<int> mBookmarkLines;
    int mActiveBreakpointLine;
    PCppParser mParser;
    CodeCompletionPopup *mCompletionPopup;
    HeaderCompletionPopup *mHeaderCompletionPopup;
    FunctionTooltipWidget *mFunctionTooltip;
    QString mCurrentWord;
    QString mCurrentDebugTipWord;
    TipType mCurrentTipType;
    QString mOldHighlightedWord;
    QString mCurrentHighlightedWord;
    QDateTime mHideTime;
    bool mAutoBackupEnabled;

    bool mSaving;
    bool mCurrentLineModified;
    int mXOffsetSince;
    int mTabStopBegin;
    int mTabStopEnd;
    int mTabStopY;
    bool mCanAutoSave;
    QString mLineBeforeTabStop;
    QString mLineAfterTabStop;
    QList<PTabStop> mUserCodeInTabStops;
    QSynedit::CharPos mHighlightCharPos1;
    QSynedit::CharPos mHighlightCharPos2;
    std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > mStatementColors;
    QTimer mFunctionTipTimer;
    QTimer mAutoBackupTimer;
    QTimer mTooltipTimer;
    int mHoverModifiedLine;
    int mWheelAccumulatedDelta;
    bool mCtrlClicking;
    FileType mFileType;
    QString mContextFile;

    QMap<QString,StatementKind> mIdCache;
    qint64 mLastFocusOutTime;

    CodeSnippetsManager *mCodeSnippetsManager;

    GetSharedParserrFunc mGetSharedParserFunc;
    GetOpennedEditorFunc mGetOpennedEditorFunc;
    GetFileStreamFunc mGetFileStreamFunc;
    CanShowEvalTipFunc mCanShowEvalTipFunc;
    RequestEvalTipFunc mRequestEvalTipFunc;
    EvalTipReadyCallback mEvalTipReadyCallback;
    GetReformatterFunc mGetReformatterFunc;
    GetMacroVarsFunc mGetMacroVarsFunc;
#ifdef ENABLE_SDCC
    GetCompilerTypeForEditorFunc mGetCompilerTypeForEditorFunc;
#endif
    QFileSystemWatcher *mFileSystemWatcher;

    const EditorSettings *mEditorSettings;
    const CodeCompletionSettings *mCodeCompletionSettings;

    // SynEdit interface
protected:
    void onGutterPaint(QPainter &painter, int aLine, int X, int Y) override;
    void onGetEditingAreas(int Line, QSynedit::EditingAreaList &areaList) override;
    bool onGetSpecialLineColors(int Line, QColor &foreground, QColor &backgroundColor) override;
    void onPreparePaintHighlightToken(int line, int aChar, const QString &token, QSynedit::PTokenAttribute attr, QSynedit::FontStyles &style, QColor &foreground, QColor &background) override;

    // QObject interface
public:
    bool event(QEvent *event) override;

protected:
    // QWidget interface
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
};

QString getWordAtPosition(QSynedit::QSynEdit* editor,
                          const QSynedit::CharPos& p,
                          QSynedit::CharPos& pWordBegin,
                          QSynedit::CharPos& pWordEnd,
                          Editor::WordPurpose purpose);


#endif // EDITOR_H
