#ifndef CODECOMPLETIONVIEW_H
#define CODECOMPLETIONVIEW_H

#include <QListView>
#include <QWidget>
#include "parser/cppparser.h"

using KeyPressedCallback = std::function<bool (QKeyEvent *)>;

class CodeCompletionListView: public QListView {
    Q_OBJECT
public:
    explicit CodeCompletionListView(QWidget *parent = nullptr);

    // QWidget interface
    const KeyPressedCallback &keypressedCallback() const;
    void setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback);

protected:
    void keyPressEvent(QKeyEvent *event) override;
private:
    KeyPressedCallback mKeypressedCallback;
};

class CodeCompletionListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit CodeCompletionListModel(StatementList* statements,QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    void notifyUpdated();
private:
    const StatementList* mStatements;

};

class CodeCompletionView : public QWidget
{
    Q_OBJECT

public:
    explicit CodeCompletionView(QWidget *parent = nullptr);
    ~CodeCompletionView();

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

    bool showCodeIns() const;
    void setShowCodeIns(bool newShowCodeIns);

    bool ignoreCase() const;
    void setIgnoreCase(bool newIgnoreCase);

    bool sortByScope() const;
    void setSortByScope(bool newSortByScope);

    bool useCppKeyword() const;
    void setUseCppKeyword(bool newUseCppKeyword);

    const PStatement &currentStatement() const;
    void setCurrentStatement(const PStatement &newCurrentStatement);

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
    QList<PCodeIns> mCodeInsList; //(Code template list)
    //QList<PStatement> mCodeInsStatements; //temporary (user code template) statements created when show code suggestion
    StatementList mFullCompletionStatementList;
    StatementList mCompletionStatementList;
    QSet<QString> mIncludedFiles;
    QSet<QString> mUsings;
    QSet<QString> mAddedStatements;
    QString mPhrase;
    QHash<QString,int> mSymbolUsage;
    QRecursiveMutex mMutex;

    PCppParser mParser;
    PStatement mCurrentStatement;
    int mShowCount;
    bool mOnlyGlobals;
    bool mRecordUsage;
    bool mShowKeywords;
    bool mShowCodeIns;
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
};

#endif // CODECOMPLETIONVIEW_H
