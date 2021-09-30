#ifndef CODECOMPLETIONPOPUP_H
#define CODECOMPLETIONPOPUP_H

#include <QListView>
#include <QWidget>
#include "parser/cppparser.h"
#include "codecompletionlistview.h"

class CodeCompletionListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit CodeCompletionListModel(const StatementList* statements,QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    void notifyUpdated();
    const ColorCallback &colorCallback() const;
    void setColorCallback(const ColorCallback &newColorCallback);

private:
    const StatementList* mStatements;
    ColorCallback mColorCallback;
};

class CodeCompletionPopup : public QWidget
{
    Q_OBJECT

public:
    explicit CodeCompletionPopup(QWidget *parent = nullptr);
    ~CodeCompletionPopup();

    void setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback);
    void prepareSearch(const QString& phrase, const QString& filename, int line);
    bool search(const QString& phrase, bool autoHideOnSingleResult);

    PStatement selectedStatement();

    const PCppParser &parser() const;
    void setParser(const PCppParser &newParser);

    int showCount() const;
    void setShowCount(int newShowCount);

    bool onlyGlobals() const;
    void setOnlyGlobals(bool newOnlyGlobals);

    bool recordUsage() const;
    void setRecordUsage(bool newRecordUsage);

    bool showKeywords() const;
    void setShowKeywords(bool newShowKeywords);

    bool showCodeSnippets() const;
    void setShowCodeSnippets(bool newShowCodeIns);

    bool ignoreCase() const;
    void setIgnoreCase(bool newIgnoreCase);

    bool sortByScope() const;
    void setSortByScope(bool newSortByScope);

    bool useCppKeyword() const;
    void setUseCppKeyword(bool newUseCppKeyword);

    const PStatement &currentStatement() const;
    void setCurrentStatement(const PStatement &newCurrentStatement);
    const std::shared_ptr<QHash<StatementKind, QColor> >& colors() const;
    void setColors(const std::shared_ptr<QHash<StatementKind, QColor> > &newColors);

private:
    void addChildren(PStatement scopeStatement, const QString& fileName,
                     int line);
    void addStatement(PStatement statement, const QString& fileName, int line);
    void filterList(const QString& member);
    void getCompletionFor(const QString& fileName,const QString& phrase, int line);
    bool isIncluded(const QString& fileName);
private:
    CodeCompletionListView * mListView;
    CodeCompletionListModel* mModel;
    QList<PCodeSnippet> mCodeSnippets; //(Code template list)
    //QList<PStatement> mCodeInsStatements; //temporary (user code template) statements created when show code suggestion
    StatementList mFullCompletionStatementList;
    StatementList mCompletionStatementList;
    QSet<QString> mIncludedFiles;
    QSet<QString> mUsings;
    QSet<QString> mAddedStatements;
    QString mPhrase;
    QRecursiveMutex mMutex;
    std::shared_ptr<QHash<StatementKind, QColor>> mColors;

    PCppParser mParser;
    PStatement mCurrentStatement;
    int mShowCount;
    bool mOnlyGlobals;
    bool mRecordUsage;
    bool mShowKeywords;
    bool mShowCodeSnippets;
    bool mIgnoreCase;
    bool mSortByScope;
    bool mUseCppKeyword;

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    // QObject interface
public:
    bool event(QEvent *event) override;
    const QString &phrase() const;
    const QList<PCodeSnippet> &codeSnippets() const;
    void setCodeSnippets(const QList<PCodeSnippet> &newCodeSnippets);
};

#endif // CODECOMPLETIONPOPUP_H
