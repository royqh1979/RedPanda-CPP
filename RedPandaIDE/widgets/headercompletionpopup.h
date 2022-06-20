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

#include <QDir>
#include <QWidget>
#include "codecompletionlistview.h"
#include "../parser/cppparser.h"

enum class HeaderCompletionListItemType {
    LocalHeader,
    ProjectHeader,
    SystemHeader
};

struct HeaderCompletionListItem {
    QString filename;
    QString fullpath;
    bool isFolder;
    int usageCount;
    HeaderCompletionListItemType itemType;
};

using PHeaderCompletionListItem=std::shared_ptr<HeaderCompletionListItem>;

class HeaderCompletionListModel: public QAbstractListModel {
    Q_OBJECT
public:
    explicit HeaderCompletionListModel(const QList<PHeaderCompletionListItem>* files,QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    void notifyUpdated();
    void setLocalColor(const QColor &newColor);
    void setSystemColor(const QColor &newColor);
    void setProjectColor(const QColor &newColor);

    void setFolderColor(const QColor &newFolderColor);

private:
    const QList<PHeaderCompletionListItem> *mFiles;
    QColor mLocalColor;
    QColor mSystemColor;
    QColor mProjectColor;
    QColor mFolderColor;
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
    void setSuggestionColor(const QColor& localColor,
                            const QColor& projectColor,
                            const QColor& systemColor,
                            const QColor& folderColor);
    QString selectedFilename(bool updateUsageCount);

private:
    void filterList(const QString& member);
    void getCompletionFor(const QString& phrase);
    void addFilesInPath(const QString& path, HeaderCompletionListItemType type);
    void addFile(const QDir& dir,  const QFileInfo &fileInfo, HeaderCompletionListItemType type);
    void addFilesInSubDir(const QString& baseDirPath, const QString& subDirName, HeaderCompletionListItemType type);
private:

    CodeCompletionListView* mListView;
    HeaderCompletionListModel* mModel;
    QHash<QString, PHeaderCompletionListItem> mFullCompletionList;
    QList<PHeaderCompletionListItem> mCompletionList;
    QHash<QString,int> mHeaderUsageCounts;
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
