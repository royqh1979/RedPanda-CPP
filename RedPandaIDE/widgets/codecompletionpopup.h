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
#ifndef CODECOMPLETIONPOPUP_H
#define CODECOMPLETIONPOPUP_H

#include <QListView>
#include <QWidget>
#include <QStyledItemDelegate>
#include "parser/cppparser.h"
#include "codecompletionlistview.h"

class ColorSchemeItem;
class CodeCompletionListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit CodeCompletionListModel(const StatementList* statements,QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    PStatement statement(const QModelIndex &index) const;
    QPixmap statementIcon(const QModelIndex &index, int size) const;
    void notifyUpdated();

private:
    const StatementList* mStatements;
};

enum class CodeCompletionType {
    Normal,
    ComplexKeyword,
    FunctionWithoutDefinition,
    Namespaces,
    Types,
    Macros,
    KeywordsOnly
};

class CodeCompletionListItemDelegate: public QStyledItemDelegate {
    Q_OBJECT
public:
    CodeCompletionListItemDelegate(CodeCompletionListModel *model=nullptr, QWidget *parent = nullptr);


    // QAbstractItemDelegate interface
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    const QColor &normalColor() const;
    void setNormalColor(const QColor &newNormalColor);

    const QColor &matchedColor() const;
    void setMatchedColor(const QColor &newMatchedColor);

    const QFont &font() const;
    void setFont(const QFont &newFont);

    float lineHeightFactor() const;
    void setLineHeightFactor(float newLineHeightFactor);

    QColor currentSelectionColor() const;
    void setCurrentSelectionColor(const QColor &newCurrentSelectionColor);

private:
    CodeCompletionListModel *mModel;
    QColor mNormalColor;
    QColor mMatchedColor;
    QColor mCurrentSelectionColor;
    QFont mFont;
    float mLineHeightFactor;
};

class CodeCompletionPopup : public QWidget
{
    Q_OBJECT

public:
    explicit CodeCompletionPopup(QWidget *parent = nullptr);
    ~CodeCompletionPopup();

    void setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback);
    void prepareSearch(const QString& preWord,
                       const QStringList & ownerExpression,
                       const QString& memberOperator,
                       const QStringList& memberExpression,
                       const QString& filename,
                       int line,
                       CodeCompletionType completionType,
                       const QSet<QString>& customKeywords);
    bool search(const QString& memberPhrase, bool autoHideOnSingleResult);

    PStatement selectedStatement();

    const PCppParser &parser() const;
    void setParser(const PCppParser &newParser);

    int showCount() const;
    void setShowCount(int newShowCount);

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

    bool hideSymbolsStartWithUnderline() const;
    void setHideSymbolsStartWithUnderline(bool newHideSymbolsStartWithUnderline);
    bool hideSymbolsStartWithTwoUnderline() const;
    void setHideSymbolsStartWithTwoUnderline(bool newHideSymbolsStartWithTwoUnderline);
    void setLineHeightFactor(float factor);

    const PStatement &currentScope() const;
    void setCurrentScope(const PStatement &newCurrentStatement);
    const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > >& colors() const;
    void setColors(const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &newColors);
    const QString &memberPhrase() const;
    const QList<PCodeSnippet> &codeSnippets() const;
    void setCodeSnippets(const QList<PCodeSnippet> &newCodeSnippets);
private:
    void addChildren(const PStatement& scopeStatement, const QString& fileName,
                     int line, bool onlyTypes=false);
    void addFunctionWithoutDefinitionChildren(const PStatement& scopeStatement, const QString& fileName,
                     int line);
    void addStatement(const PStatement& statement, const QString& fileName, int line);
    void filterList(const QString& member);
    void getKeywordCompletionFor(const QSet<QString>& customKeywords);
    void getMacroCompletionList(const QString &fileName, int line);
    void getCompletionFor(
            QStringList ownerExpression,
            const QString& memberOperator,
            const QStringList& memberExpression,
            const QString& fileName,
            int line,
            const QSet<QString>& customKeywords);

    void getCompletionForFunctionWithoutDefinition(
            const QString& preWord,
            QStringList ownerExpression,
            const QString& memberOperator,
            const QStringList& memberExpression,
            const QString& fileName,
            int line);

    void getCompletionListForComplexKeyword(const QString& preWord);
    void getCompletionListForNamespaces(const QString &preWord,
                                        const QString& fileName,
                                        int line);
    void getCompletionListForTypes(const QString &preWord,
                                        const QString& fileName,
                                        int line);
    void addKeyword(const QString& keyword);
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
    QString mMemberPhrase;
    QString mMemberOperator;
    mutable QRecursiveMutex mMutex;
    std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > mColors;
    CodeCompletionListItemDelegate* mDelegate;

    PCppParser mParser;
    PStatement mCurrentScope;
    int mShowCount;
    bool mRecordUsage;
    bool mShowKeywords;
    bool mShowCodeSnippets;
    bool mIgnoreCase;
    bool mSortByScope;
    bool mHideSymbolsStartWithUnderline;
    bool mHideSymbolsStartWithTwoUnderline;

    // QWidget interface
protected:
    void hideEvent(QHideEvent *event) override;

    // QObject interface
public:
    bool event(QEvent *event) override;
    const QString &memberOperator() const;

};

#endif // CODECOMPLETIONPOPUP_H
