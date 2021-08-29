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
