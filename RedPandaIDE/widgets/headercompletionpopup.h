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
#ifndef HEADERCOMPLETIONPOPUP_H
#define HEADERCOMPLETIONPOPUP_H

#include <QWidget>
#include "codecompletionlistview.h"
#include "../parser/cppparser.h"

class HeaderCompletionListModel: public QAbstractListModel {
    Q_OBJECT
public:
    explicit HeaderCompletionListModel(const QStringList* files,QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    void notifyUpdated();
    void setColor(const QColor &newColor);

private:
    const QStringList* mFiles;
    QColor mColor;
};

class HeaderCompletionPopup : public QWidget
{
    Q_OBJECT
public:
    HeaderCompletionPopup(QWidget* parent=nullptr);
    ~HeaderCompletionPopup();
    void prepareSearch(const QString& phrase, const QString& fileName);
    bool search(const QString& phrase, bool autoHideOnSingleResult);
    void setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback);
    void setSuggestionColor(const QColor& color);
    QString selectedFilename();

private:
    void filterList(const QString& member);
    void getCompletionFor(const QString& phrase);
    void addFilesInPath(const QString& path);
    void addFile(const QString& fileName);
    void addFilesInSubDir(const QString& baseDirPath, const QString& subDirName);
private:

    CodeCompletionListView* mListView;
    HeaderCompletionListModel* mModel;
    QSet<QString> mFullCompletionList;
    QStringList mCompletionList;
    int mShowCount;
    QSet<QString> mAddedFileNames;

    PCppParser mParser;
    QString mPhrase;
    bool mIgnoreCase;
    bool mSearchLocal;
    QString mCurrentFile;

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    // QObject interface
public:
    bool event(QEvent *event) override;
    void setParser(const PCppParser &newParser);
    const QString &phrase() const;
    bool ignoreCase() const;
    void setIgnoreCase(bool newIgnoreCase);
    bool searchLocal() const;
    void setSearchLocal(bool newSearchLocal);
};

#endif // HEADERCOMPLETIONPOPUP_H
