#ifndef EDITOR_H
#define EDITOR_H

#include <QObject>
#include <utils.h>
#include <QTabWidget>
#include "qsynedit/SynEdit.h"
#include "colorscheme.h"

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

    void loadFile();
    void saveFile(const QString& filename);
    void convertToEncoding(const QByteArray& encoding);
    bool save(bool force=false, bool reparse=true);
    bool saveAs();
    void activate();

    QTabWidget* pageControl() noexcept;

    void updateCaption(const QString& newCaption=QString());
    void applySettings();
    void applyColorScheme(const QString& schemeName);
    void copyToClipboard() override;
    void cutToClipboard() override;
    void copyAsHTML();
signals:


protected slots:
    void onModificationChanged(bool status) ;
    void onStatusChanged(SynStatusChanges changes);

private:
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
    QuoteStatus getQuoteStatus();

private:
    static int newfileCount;
    QByteArray mEncodingOption; // the encoding type set by the user
    QByteArray mFileEncoding; // the real encoding of the file (auto detected)
    QString mFilename;
    QTabWidget* mParentPageControl;
    bool mInProject;
    bool mIsNew;

    // QWidget interface
protected:
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
};

#endif // EDITOR_H
