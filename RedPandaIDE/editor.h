#ifndef EDITOR_H
#define EDITOR_H

#include <QObject>
#include <utils.h>
#include <QTabWidget>
#include "qsynedit/SynEdit.h"
#include "colorscheme.h"
#include "common.h"
#include "parser/cppparser.h"
#include "widgets/codecompletionpopup.h"
#include "widgets/headercompletionpopup.h"

#define USER_CODE_IN_INSERT_POS "%INSERT%"
#define USER_CODE_IN_REPL_POS_BEGIN "%REPL_BEGIN%"
#define USER_CODE_IN_REPL_POS_END "%REPL_END%"

struct TabStop {
    int x;
    int endX;
    int y;
};

using PTabStop = std::shared_ptr<TabStop>;

class SaveException: public std::exception {

public:
    explicit SaveException(const QString& reason);
    explicit SaveException(const QString&& reason);
    // exception interface
    const QString& reason() const noexcept;
public:
    const char *what() const noexcept override;
private:
    QString mReason;
    QByteArray mReasonBuffer;
};

class Editor : public SynEdit
{
    Q_OBJECT
public:
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
        RawStringNoEscape
    };

    enum class WordPurpose {
        wpCompletion, // walk backwards over words, array, functions, parents, no forward movement
        wpEvaluation, // walk backwards over words, array, functions, parents, forwards over words, array
        wpHeaderCompletion, // walk backwards over path
        wpHeaderCompletionStart, // walk backwards over path, including start '<' or '"'
        wpDirective, // preprocessor
        wpJavadoc, //javadoc
        wpInformation // walk backwards over words, array, functions, parents, forwards over words
    };

    enum class TipType {
      Preprocessor, // cursor hovers above preprocessor line
      Identifier, // cursor hovers above identifier
      Selection, // cursor hovers above selection
      None, // mouseover not allowed
      Error //Cursor hovers above error line/item;
    };

    struct SyntaxIssue {
        int col;
        int endCol;
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

    explicit Editor(QWidget *parent, const QString& filename,
                    const QByteArray& encoding,
                    bool inProject, bool isNew,QTabWidget* parentPageControl);

    ~Editor();

    //tell the compiler to prohibit copy/moving editor objects ( we should only use pointers to the editor object)
    Editor(const Editor&) = delete;
    Editor(const Editor&&) = delete;
    Editor& operator=(const Editor&) = delete;
    Editor& operator=(const Editor&&) = delete;

    const QByteArray& encodingOption() const noexcept;
    void setEncodingOption(const QByteArray& encoding) noexcept;
    const QByteArray& fileEncoding() const noexcept;
    const QString& filename() const noexcept;
    bool inProject() const noexcept;
    bool isNew() const noexcept;

    void loadFile(QString filename = "");
    void saveFile(QString filename);
    void convertToEncoding(const QByteArray& encoding);
    bool save(bool force=false, bool reparse=true);
    bool saveAs(const QString& name="", bool fromProject = false);
    void activate();

    QTabWidget* pageControl() noexcept;
    void setPageControl(QTabWidget* newPageControl);

    void updateCaption(const QString& newCaption=QString());
    void applySettings();
    void applyColorScheme(const QString& schemeName);
    void copyToClipboard() override;
    void cutToClipboard() override;
    void copyAsHTML();
    void setCaretPosition(int line,int aChar);
    void setCaretPositionAndActivate(int line,int aChar);

    void addSyntaxIssues(int line, int startChar, int endChar, CompileIssueType errorType, const QString& hint);
    void clearSyntaxIssues();
    void gotoNextSyntaxIssue();
    void gotoPrevSyntaxIssue();
    bool hasPrevSyntaxIssue() const;
    bool hasNextSyntaxIssue() const;
    PSyntaxIssueList getSyntaxIssuesAtLine(int line);
    PSyntaxIssue getSyntaxIssueAtPosition(const BufferCoord& pos);
    int gutterClickedLine() const;
    void toggleBreakpoint(int line);
    void clearBreakpoints();
    bool hasBreakpoint(int line);
    void addBookmark(int line,const QString& description);
    void removeBookmark(int line);
    bool hasBookmark(int line);
    void clearBookmarks();
    void removeBreakpointFocus();
    void modifyBreakpointProperty(int line);
    void setActiveBreakpointFocus(int Line, bool setFocus=true);
    QString getPreviousWordAtPositionForSuggestion(const BufferCoord& p);
    void reformat();
    void checkSyntaxInBack();
    void gotoDeclaration(const BufferCoord& pos);
    void gotoDefinition(const BufferCoord& pos);
    void reparse();
    void reparseTodo();
    void insertString(const QString& value, bool moveCursor);
    void insertCodeSnippet(const QString& code);
    void print();
    void exportAsRTF(const QString& rtfFilename);
    void exportAsHTML(const QString& htmlFilename);
    void resetBreakpoints();

    const PCppParser &parser();

    void tab() override;

private slots:
    void onStatusChanged(SynStatusChanges changes);
    void onGutterClicked(Qt::MouseButton button, int x, int y, int line);
    void onTipEvalValueReady(const QString& value);
    void onLinesDeleted(int first,int count);
    void onLinesInserted(int first,int count);

private:
    bool isBraceChar(QChar ch);
    void resetBookmarks();
    QChar getCurrentChar();
    bool handleSymbolCompletion(QChar key);
    bool handleParentheseCompletion();
    bool handleParentheseSkip();
    bool handleBracketCompletion();
    bool handleBracketSkip();
    bool handleMultilineCommentCompletion();
    bool handleBraceCompletion();
    bool handleBraceSkip();
    bool handleSingleQuoteCompletion();
    bool handleDoubleQuoteCompletion();
    bool handleGlobalIncludeCompletion();
    bool handleGlobalIncludeSkip();

    bool handleCodeCompletion(QChar key);
    void initParser();
    void undoSymbolCompletion(int pos);
    QuoteStatus getQuoteStatus();

    void showCompletion(bool autoComplete);
    void showHeaderCompletion(bool autoComplete);

    bool testInFunc(int x,int y);

    void completionInsert(bool appendFunc=false);

    void headerCompletionInsert();

    bool onCompletionKeyPressed(QKeyEvent* event);
    bool onHeaderCompletionKeyPressed(QKeyEvent* event);
    bool onCompletionInputMethod(QInputMethodEvent *event);

    TipType getTipType(QPoint point, BufferCoord& pos);
    void cancelHint();
    QString getFileHint(const QString& s);
    QString getParserHint(const QString& s, int line);
    void showDebugHint(const QString& s,int line);
    QString getErrorHint(const PSyntaxIssue& issue);
    QString getHintForFunction(const PStatement& statement, const PStatement& scope,
                               const QString& filename, int line);

    void updateFunctionTip();
    void clearUserCodeInTabStops();
    void popUserCodeInTabStops();
    void onExportedFormatToken(PSynHighlighter syntaxHighlighter, int Line, int column, const QString& token,
        PSynHighlighterAttribute &attr);
private:
    QByteArray mEncodingOption; // the encoding type set by the user
    QByteArray mFileEncoding; // the real encoding of the file (auto detected)
    QString mFilename;
    QTabWidget* mParentPageControl;
    bool mInProject;
    bool mIsNew;
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
    int mGutterClickedLine;
    QSet<int> mBreakpointLines;
    QSet<int> mBookmarkLines;
    int mActiveBreakpointLine;
    PCppParser mParser;
    std::shared_ptr<CodeCompletionPopup> mCompletionPopup;
    std::shared_ptr<HeaderCompletionPopup> mHeaderCompletionPopup;
    int mLastIdCharPressed;
    bool mUseCppSyntax;
    QString mCurrentWord;
    QString mCurrentDebugTipWord;
    TipType mCurrentTipType;
    QString mOldHighlightedWord;
    QString mCurrentHighlightedWord;

    bool mSaving;
    bool mCurrentLineModified;
    int mXOffsetSince;
    int mTabStopBegin;
    int mTabStopEnd;
    int mTabStopY;
    QString mLineBeforeTabStop;
    QString mLineAfterTabStop;
    QList<PTabStop> mUserCodeInTabStops;
    BufferCoord mHighlightCharPos1;
    BufferCoord mHighlightCharPos2;

    // QWidget interface
protected:
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    // SynEdit interface
protected:
    void onGutterPaint(QPainter &painter, int aLine, int X, int Y) override;
    void onGetEditingAreas(int Line, SynEditingAreaList &areaList) override;

    // SynEdit interface
protected:
    bool onGetSpecialLineColors(int Line, QColor &foreground, QColor &backgroundColor) override;

    // SynEdit interface
protected:
    void onPreparePaintHighlightToken(int line, int aChar, const QString &token, PSynHighlighterAttribute attr, SynFontStyles &style, QColor &foreground, QColor &background) override;

    // QObject interface
public:
    bool event(QEvent *event) override;

    // QWidget interface
    void setInProject(bool newInProject);

    bool useCppSyntax() const;
    void setUseCppSyntax(bool newUseCppSyntax);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

    // QWidget interface
protected:
    void inputMethodEvent(QInputMethodEvent *) override;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

QString getWordAtPosition(SynEdit* editor,
                          const BufferCoord& p,
                          BufferCoord& pWordBegin,
                          BufferCoord& pWordEnd,
                          Editor::WordPurpose purpose);


#endif // EDITOR_H
