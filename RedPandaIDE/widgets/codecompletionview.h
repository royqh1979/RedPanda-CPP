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

class CodeCompletionView : public QWidget
{
    Q_OBJECT

public:
    explicit CodeCompletionView(QWidget *parent = nullptr);
    ~CodeCompletionView();

    void setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback);

private:
    void addChildren(PStatement scopeStatement, const QString& fileName,
                     int line);
    void addStatement(PStatement statement, const QString& fileName, int line);
    void filterList(const QString& member);
    void getCompletionFor(const QString& fileName,const QString& phrase, int line);
    bool isIncluded(const QString& fileName);
private:
    CodeCompletionListView * mListView;
    QList<PCodeIns> mCodeInsList; //(Code template list)
    //QList<PStatement> mCodeInsStatements; //temporary (user code template) statements created when show code suggestion
    PCppParser mParser;
    QList<PStatement> mFullCompletionStatementList;
    QList<PStatement> mCompletionStatementList;
    bool mEnabled;
    int mShowCount;
    bool mOnlyGlobals;
    PStatement mCurrentStatement;
    QSet<QString> mIncludedFiles;
    QSet<QString> mUsings;
    QString mIsIncludedCacheFileName;
    bool mIsIncludedCacheResult;
    QSet<QString> mAddedStatements;
    bool mPreparing;
    QString mPhrase;
    QHash<QString,int> mSymbolUsage;
    bool mRecordUsage;
    bool mShowKeywords;
    bool mShowCodeIns;
    bool mIgnoreCase;
    QRecursiveMutex mMutex;
    QString mParserSerialId;
    bool mSortByScope;
    bool mUseCppKeyword;
};

#endif // CODECOMPLETIONVIEW_H
